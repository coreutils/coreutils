/* expr -- evaluate expressions.
   Copyright (C) 86, 1991-1997, 1999-2008 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* Author: Mike Parker.
   Modified for arbitrary-precision calculation by James Youngman.

   This program evaluates expressions.  Each token (operator, operand,
   parenthesis) of the expression must be a seperate argument.  The
   parser used is a reasonably general one, though any incarnation of
   it is language-specific.  It is especially nice for expressions.

   No parse tree is needed; a new node is evaluated immediately.
   One function can handle multiple operators all of equal precedence,
   provided they all associate ((x op x) op x).

   Define EVAL_TRACE to print an evaluation trace.  */

#include <config.h>
#include <stdio.h>
#include <sys/types.h>
#include "system.h"

#include <assert.h>
#include <regex.h>
#if HAVE_GMP
#include <gmp.h>
#endif
#include "error.h"
#include "quotearg.h"
#include "strnumcmp.h"
#include "xstrtol.h"

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "expr"

#define AUTHORS proper_name ("Mike Parker"), proper_name ("James Youngman")

/* Exit statuses.  */
enum
  {
    /* Invalid expression: e.g., its form does not conform to the
       grammar for expressions.  Our grammar is an extension of the
       POSIX grammar.  */
    EXPR_INVALID = 2,

    /* An internal error occurred, e.g., arithmetic overflow, storage
       exhaustion.  */
    EXPR_FAILURE
  };

/* The kinds of value we can have.
   In the comments below, a variable is described as "arithmetic" if
   it is either integer or mp_integer.   Variables are of type mp_integer
   only if GNU MP is available, but the type designator is always defined. */
enum valtype
{
  integer,
  mp_integer,
  string
};
typedef enum valtype TYPE;

/* A value is.... */
struct valinfo
{
  TYPE type;			/* Which kind. */
  union
  {				/* The value itself. */
    /* We could use intmax_t but that would integrate less well with GMP,
       since GMP has mpz_set_si but no intmax_t equivalent. */
    signed long int i;
#if HAVE_GMP
    mpz_t z;
#endif
    char *s;
  } u;
};
typedef struct valinfo VALUE;

/* The arguments given to the program, minus the program name.  */
static char **args;

static VALUE *eval (bool);
static bool nomoreargs (void);
static bool null (VALUE *v);
static void printv (VALUE *v);

/* Arithmetic is done in one of three modes.

   The --bignum option forces all arithmetic to use bignums other than
   string indexing (mode==MP_ALWAYS).  The --no-bignum option forces
   all arithmetic to use native types rather than bignums
   (mode==MP_NEVER).

   The default mode is MP_AUTO if GMP is available and MP_NEVER if
   not.  Most functions will process a bignum if one is found, but
   will not convert a native integer to a string if the mode is
   MP_NEVER. */
enum arithmetic_mode
  {
    MP_NEVER,			/* Never use bignums */
#if HAVE_GMP
    MP_ALWAYS,			/* Always use bignums. */
    MP_AUTO,			/* Switch if result would otherwise overflow */
#endif
  };
static enum arithmetic_mode mode =
#if HAVE_GMP
  MP_AUTO
#else
  MP_NEVER
#endif
  ;


void
usage (int status)
{
  if (status != EXIT_SUCCESS)
    fprintf (stderr, _("Try `%s --help' for more information.\n"),
	     program_name);
  else
    {
      printf (_("\
Usage: %s EXPRESSION\n\
  or:  %s OPTION\n\
"),
	      program_name, program_name);
      putchar ('\n');
      fputs (_("\
      --bignum     always use arbitrary-precision arithmetic\n\
      --no-bignum  always use single-precision arithmetic\n"),
	       stdout);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      fputs (_("\
\n\
Print the value of EXPRESSION to standard output.  A blank line below\n\
separates increasing precedence groups.  EXPRESSION may be:\n\
\n\
  ARG1 | ARG2       ARG1 if it is neither null nor 0, otherwise ARG2\n\
\n\
  ARG1 & ARG2       ARG1 if neither argument is null or 0, otherwise 0\n\
"), stdout);
      fputs (_("\
\n\
  ARG1 < ARG2       ARG1 is less than ARG2\n\
  ARG1 <= ARG2      ARG1 is less than or equal to ARG2\n\
  ARG1 = ARG2       ARG1 is equal to ARG2\n\
  ARG1 != ARG2      ARG1 is unequal to ARG2\n\
  ARG1 >= ARG2      ARG1 is greater than or equal to ARG2\n\
  ARG1 > ARG2       ARG1 is greater than ARG2\n\
"), stdout);
      fputs (_("\
\n\
  ARG1 + ARG2       arithmetic sum of ARG1 and ARG2\n\
  ARG1 - ARG2       arithmetic difference of ARG1 and ARG2\n\
"), stdout);
      /* Tell xgettext that the "% A" below is not a printf-style
	 format string:  xgettext:no-c-format */
      fputs (_("\
\n\
  ARG1 * ARG2       arithmetic product of ARG1 and ARG2\n\
  ARG1 / ARG2       arithmetic quotient of ARG1 divided by ARG2\n\
  ARG1 % ARG2       arithmetic remainder of ARG1 divided by ARG2\n\
"), stdout);
      fputs (_("\
\n\
  STRING : REGEXP   anchored pattern match of REGEXP in STRING\n\
\n\
  match STRING REGEXP        same as STRING : REGEXP\n\
  substr STRING POS LENGTH   substring of STRING, POS counted from 1\n\
  index STRING CHARS         index in STRING where any CHARS is found, or 0\n\
  length STRING              length of STRING\n\
"), stdout);
      fputs (_("\
  + TOKEN                    interpret TOKEN as a string, even if it is a\n\
                               keyword like `match' or an operator like `/'\n\
\n\
  ( EXPRESSION )             value of EXPRESSION\n\
"), stdout);
      fputs (_("\
\n\
Beware that many operators need to be escaped or quoted for shells.\n\
Comparisons are arithmetic if both ARGs are numbers, else lexicographical.\n\
Pattern matches return the string matched between \\( and \\) or null; if\n\
\\( and \\) are not used, they return the number of characters matched or 0.\n\
"), stdout);
      fputs (_("\
\n\
Exit status is 0 if EXPRESSION is neither null nor 0, 1 if EXPRESSION is null\n\
or 0, 2 if EXPRESSION is syntactically invalid, and 3 if an error occurred.\n\
"), stdout);
      emit_bug_reporting_address ();
    }
  exit (status);
}

/* Report a syntax error and exit.  */
static void
syntax_error (void)
{
  error (EXPR_INVALID, 0, _("syntax error"));
}

/* Report an integer overflow for operation OP and exit.  */
static void
integer_overflow (char op)
{
  error (EXPR_FAILURE, 0,
	 _("arithmetic operation %c produced an out of range value, "
	   "but arbitrary-precision arithmetic is not available"), op);
}

static void
string_too_long (void)
{
  error (EXPR_FAILURE, ERANGE, _("string too long"));
}

enum
{
  USE_BIGNUM = CHAR_MAX + 1,
  NO_USE_BIGNUM
};

static struct option const long_options[] =
{
  {"bignum", no_argument, NULL, USE_BIGNUM},
  {"no-bignum", no_argument, NULL, NO_USE_BIGNUM},
  {GETOPT_HELP_OPTION_DECL},
  {GETOPT_VERSION_OPTION_DECL},
  {NULL, 0, NULL, 0}
};

int
main (int argc, char **argv)
{
  VALUE *v;
  int c;

  initialize_main (&argc, &argv);
  set_program_name (argv[0]);
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  initialize_exit_failure (EXPR_FAILURE);
  atexit (close_stdout);

  /* The argument -0 should not result in an error message. */
  opterr = 0;

  while ((c = getopt_long (argc, argv, "+", long_options, NULL)) != -1)
    {
      /* "expr -0" should interpret the -0 as an integer argument.
	 arguments like --foo should also be interpreted as a string
	 argument to be "evaluated".
       */
      if ('?' == c)
	{
	  --optind;
	  break;
	}
      else
	switch (c)
	  {
	  case USE_BIGNUM:
#if HAVE_GMP
	    mode = MP_ALWAYS;
#else
	    error (0, 0, _("arbitrary-precision support is not available"));
#endif
	    break;

	  case NO_USE_BIGNUM:
	    mode = MP_NEVER;
	    break;

	    case_GETOPT_HELP_CHAR;

	    case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);
	  }
    }

  if (argc <= optind)
    {
      error (0, 0, _("missing operand"));
      usage (EXPR_INVALID);
    }

  args = argv + optind;

  v = eval (true);
  if (!nomoreargs ())
    syntax_error ();
  printv (v);

  exit (null (v));
}

/* Return a VALUE for I.  */

static VALUE *
int_value (long int i)
{
  VALUE *v = xmalloc (sizeof *v);
#if HAVE_GMP
  if (mode == MP_ALWAYS)
    {
      /* all integer values are handled as bignums. */
      mpz_init_set_si (v->u.z, i);
      v->type = mp_integer;
      return v;
    }
#endif

  v->type = integer;
  v->u.i = i;
  return v;
}

/* Return a VALUE for S.  */

static VALUE *
str_value (char const *s)
{
  VALUE *v = xmalloc (sizeof *v);
  v->type = string;
  v->u.s = xstrdup (s);
  return v;
}


static VALUE *
substr_value (char const *s, size_t len, size_t pos, size_t nchars_wanted)
{
  if (pos >= len)
    return str_value ("");
  else
    {
      VALUE *v = xmalloc (sizeof *v);
      size_t vlen = MIN (nchars_wanted, len - pos + 1);
      char *vlim;
      v->type = string;
      v->u.s = xmalloc (vlen + 1);
      vlim = mempcpy (v->u.s, s + pos, vlen);
      *vlim = '\0';
      return v;
    }
}


/* Free VALUE V, including structure components.  */

static void
freev (VALUE *v)
{
  if (v->type == string)
    {
      free (v->u.s);
    }
  else if (v->type == mp_integer)
    {
      assert (mode != MP_NEVER);
#if HAVE_GMP
      mpz_clear (v->u.z);
#endif
    }
  free (v);
}

/* Print VALUE V.  */

static void
printv (VALUE *v)
{
  switch (v->type)
    {
    case integer:
      printf ("%ld\n", v->u.i);
      break;
    case string:
      puts (v->u.s);
      break;
#if HAVE_GMP
    case mp_integer:
      mpz_out_str (stdout, 10, v->u.z);
      putchar ('\n');
      break;
#endif
    default:
      abort ();
    }

}

/* Return true if V is a null-string or zero-number.  */

static bool
null (VALUE *v)
{
  switch (v->type)
    {
    case integer:
      return v->u.i == 0;
#if HAVE_GMP
    case mp_integer:
      return mpz_sgn (v->u.z) == 0;
#endif
    case string:
      {
	char const *cp = v->u.s;
	if (*cp == '\0')
	  return true;

	cp += (*cp == '-');

	do
	  {
	    if (*cp != '0')
	      return false;
	  }
	while (*++cp);

	return true;
      }
    default:
      abort ();
    }
}

/* Return true if CP takes the form of an integer.  */

static bool
looks_like_integer (char const *cp)
{
  cp += (*cp == '-');

  do
    if (! ISDIGIT (*cp))
      return false;
  while (*++cp);

  return true;
}

/* Coerce V to a string value (can't fail).  */

static void
tostring (VALUE *v)
{
  char buf[INT_BUFSIZE_BOUND (long int)];

  switch (v->type)
    {
    case integer:
      snprintf (buf, sizeof buf, "%ld", v->u.i);
      v->u.s = xstrdup (buf);
      v->type = string;
      break;
#if HAVE_GMP
    case mp_integer:
      {
	char *s = mpz_get_str (NULL, 10, v->u.z);
	if (!s)
	  {
	    xalloc_die ();
	  }
	mpz_clear (v->u.z);
	v->u.s = s;
	v->type = string;
      }
      break;
#endif
    case string:
      break;
    default:
      abort ();
    }
}

/* Coerce V to an arithmetic value.
   Return true on success, false on failure.  */

static bool
toarith (VALUE *v)
{
  switch (v->type)
    {
    case integer:
    case mp_integer:
      return true;

    case string:
      {
	long int value;

	if (! looks_like_integer (v->u.s))
	  return false;
	if (xstrtol (v->u.s, NULL, 10, &value, NULL) != LONGINT_OK)
	  {
#if HAVE_GMP
	    if (mode != MP_NEVER)
	      {
		char *s = v->u.s;
		if (mpz_init_set_str (v->u.z, s, 10))
		  abort ();  /* Bug in looks_like_integer, perhaps. */
		v->type = mp_integer;
		free (s);
	      }
	    else
	      {
		error (EXPR_FAILURE, ERANGE, "%s", v->u.s);
	      }
#else
	    error (EXPR_FAILURE, ERANGE, "%s", v->u.s);
#endif
	  }
	else
	  {
	    free (v->u.s);
	    v->u.i = value;
	    v->type = integer;
	  }
	return true;
      }
    default:
      abort ();
    }
}

/* Convert V into an integer (that is, a non-arbitrary-precision value)
   Return true on success, false on failure.  */
static bool
toint (VALUE *v)
{
  if (!toarith (v))
    return false;
#if HAVE_GMP
  if (v->type == mp_integer)
    {
      if (mpz_fits_slong_p (v->u.z))
	{
	  long value = mpz_get_si (v->u.z);
	  mpz_clear (v->u.z);
	  v->u.i = value;
	  v->type = integer;
	}
      else
	return false;		/* value was too big. */
    }
#else
  if (v->type == mp_integer)
    abort ();			/* should not happen. */
#endif
  return true;
}

/* Extract a size_t value from a positive arithmetic value, V.
   The extracted value is stored in *VAL. */
static bool
getsize (const VALUE *v, size_t *val, bool *negative)
{
  if (v->type == integer)
    {
      if (v->u.i < 0)
	{
	  *negative = true;
	  return false;
	}
      else
	{
	  *negative = false;
	  *val = v->u.i;
	  return true;
	}
    }
  else if (v->type == mp_integer)
    {
#if HAVE_GMP
      if (mpz_sgn (v->u.z) < 0)
	{
	  *negative = true;
	  return false;
	}
      else if (mpz_fits_ulong_p (v->u.z))
	{
	  unsigned long ul;
	  ul = mpz_get_ui (v->u.z);
	  *val = ul;
	  return true;
	}
      else
	{
	  *negative = false;
	  return false;
	}
#else
      abort ();
#endif

    }
  else
    {
      abort ();			/* should not pass a string. */
    }
}



/* Return true and advance if the next token matches STR exactly.
   STR must not be NULL.  */

static bool
nextarg (char const *str)
{
  if (*args == NULL)
    return false;
  else
    {
      bool r = STREQ (*args, str);
      args += r;
      return r;
    }
}

/* Return true if there no more tokens.  */

static bool
nomoreargs (void)
{
  return *args == 0;
}

#ifdef EVAL_TRACE
/* Print evaluation trace and args remaining.  */

static void
trace (fxn)
     char *fxn;
{
  char **a;

  printf ("%s:", fxn);
  for (a = args; *a; a++)
    printf (" %s", *a);
  putchar ('\n');
}
#endif

/* Do the : operator.
   SV is the VALUE for the lhs (the string),
   PV is the VALUE for the rhs (the pattern).  */

static VALUE *
docolon (VALUE *sv, VALUE *pv)
{
  VALUE *v IF_LINT (= NULL);
  const char *errmsg;
  struct re_pattern_buffer re_buffer;
  char fastmap[UCHAR_MAX + 1];
  struct re_registers re_regs;
  regoff_t matchlen;

  tostring (sv);
  tostring (pv);

  re_regs.num_regs = 0;
  re_regs.start = NULL;
  re_regs.end = NULL;

  re_buffer.buffer = NULL;
  re_buffer.allocated = 0;
  re_buffer.fastmap = fastmap;
  re_buffer.translate = NULL;
  re_syntax_options =
    RE_SYNTAX_POSIX_BASIC & ~RE_CONTEXT_INVALID_DUP & ~RE_NO_EMPTY_RANGES;
  errmsg = re_compile_pattern (pv->u.s, strlen (pv->u.s), &re_buffer);
  if (errmsg)
    error (EXPR_INVALID, 0, "%s", errmsg);
  re_buffer.newline_anchor = 0;

  matchlen = re_match (&re_buffer, sv->u.s, strlen (sv->u.s), 0, &re_regs);
  if (0 <= matchlen)
    {
      /* Were \(...\) used? */
      if (re_buffer.re_nsub > 0)
	{
	  sv->u.s[re_regs.end[1]] = '\0';
	  v = str_value (sv->u.s + re_regs.start[1]);
	}
      else
	v = int_value (matchlen);
    }
  else if (matchlen == -1)
    {
      /* Match failed -- return the right kind of null.  */
      if (re_buffer.re_nsub > 0)
	v = str_value ("");
      else
	v = int_value (0);
    }
  else
    error (EXPR_FAILURE,
	   (matchlen == -2 ? errno : EOVERFLOW),
	   _("error in regular expression matcher"));

  if (0 < re_regs.num_regs)
    {
      free (re_regs.start);
      free (re_regs.end);
    }
  re_buffer.fastmap = NULL;
  regfree (&re_buffer);
  return v;
}

/* Handle bare operands and ( expr ) syntax.  */

static VALUE *
eval7 (bool evaluate)
{
  VALUE *v;

#ifdef EVAL_TRACE
  trace ("eval7");
#endif
  if (nomoreargs ())
    syntax_error ();

  if (nextarg ("("))
    {
      v = eval (evaluate);
      if (!nextarg (")"))
	syntax_error ();
      return v;
    }

  if (nextarg (")"))
    syntax_error ();

  return str_value (*args++);
}

/* Handle match, substr, index, and length keywords, and quoting "+".  */

static VALUE *
eval6 (bool evaluate)
{
  VALUE *l;
  VALUE *r;
  VALUE *v;
  VALUE *i1;
  VALUE *i2;

#ifdef EVAL_TRACE
  trace ("eval6");
#endif
  if (nextarg ("+"))
    {
      if (nomoreargs ())
	syntax_error ();
      return str_value (*args++);
    }
  else if (nextarg ("length"))
    {
      r = eval6 (evaluate);
      tostring (r);
      v = int_value (strlen (r->u.s));
      freev (r);
      return v;
    }
  else if (nextarg ("match"))
    {
      l = eval6 (evaluate);
      r = eval6 (evaluate);
      if (evaluate)
	{
	  v = docolon (l, r);
	  freev (l);
	}
      else
	v = l;
      freev (r);
      return v;
    }
  else if (nextarg ("index"))
    {
      size_t pos, len;

      l = eval6 (evaluate);
      r = eval6 (evaluate);
      tostring (l);
      tostring (r);
      pos = strcspn (l->u.s, r->u.s);
      len = strlen (l->u.s);
      if (pos == len)
	{
	  v = int_value (0);
	}
      else
	{
	  if (pos < LONG_MAX)
	    {
	      v = int_value (pos + 1);
	    }
	  else
	    {
#if HAVE_GMP
	      if (mode != MP_NEVER
		  && pos < ULONG_MAX)
		{
		  v = xmalloc (sizeof *v);
		  mpz_init_set_ui (v->u.z, pos+1);
		  v->type = mp_integer;
		}
	      else
		string_too_long ();
#else
	      string_too_long ();
#endif
	    }
	}
      freev (l);
      freev (r);
      return v;
    }
  else if (nextarg ("substr"))
    {
      size_t llen;
      l = eval6 (evaluate);
      i1 = eval6 (evaluate);
      i2 = eval6 (evaluate);
      tostring (l);
      llen = strlen (l->u.s);

      if (!toarith (i1) || !toarith (i2))
	v = str_value ("");
      else
	{
	  size_t pos, len;
	  bool negative = false;

	  if (getsize (i1, &pos, &negative))
	    if (getsize (i2, &len, &negative))
	      if (pos == 0 || len == 0)
		v = str_value ("");
	      else
		v = substr_value (l->u.s, llen, pos-1, len);
	    else
	      if (negative)
		v = str_value ("");
	      else
		error (EXPR_FAILURE, ERANGE,
		       _("string offset is too large"));
	  else
	    if (negative)
	      v = str_value ("");
	    else
	      error (EXPR_FAILURE, ERANGE,
		     _("substring length too large"));
	}
      freev (l);
      freev (i1);
      freev (i2);
      return v;
    }
  else
    return eval7 (evaluate);
}

/* Handle : operator (pattern matching).
   Calls docolon to do the real work.  */

static VALUE *
eval5 (bool evaluate)
{
  VALUE *l;
  VALUE *r;
  VALUE *v;

#ifdef EVAL_TRACE
  trace ("eval5");
#endif
  l = eval6 (evaluate);
  while (1)
    {
      if (nextarg (":"))
	{
	  r = eval6 (evaluate);
	  if (evaluate)
	    {
	      v = docolon (l, r);
	      freev (l);
	      l = v;
	    }
	  freev (r);
	}
      else
	return l;
    }
}


#if HAVE_GMP
static void
promote (VALUE *x)
{
  if (x->type == integer)
    mpz_init_set_si (x->u.z, x->u.i);
}
#endif

/* L = L * R.  Both L and R are arithmetic. */
static void
domult (VALUE *l, VALUE *r)
{
  if (l->type == integer && r->type == integer)
    {
      long int val = 0;
      val = l->u.i * r->u.i;
      if (! (l->u.i == 0 || r->u.i == 0
	     || ((val < 0) == ((l->u.i < 0) ^ (r->u.i < 0))
		 && val / l->u.i == r->u.i)))
	{
	  /* Result would (did) overflow.  Handle with MP if available. */
	  if (mode != MP_NEVER)
	    {
#if HAVE_GMP
	      mpz_init_set_si (l->u.z, l->u.i);
	      mpz_mul_si (l->u.z, l->u.z, r->u.i); /* L*=R */
	      l->type = mp_integer;
#endif
	    }
	  else
	    {
	      integer_overflow ('*');
	    }
	}
      else
	{
	  l->u.i = val;
	}
    }
  else
    {
      /* At least one operand is already mp_integer, so promote the other. */
#if HAVE_GMP
      /* We could use mpz_mul_si here if R is not already mp_integer,
	 but for the moment we'll try to minimise code paths. */
      if (l->type == integer)
	mpz_init_set_si (l->u.z, l->u.i);
      if (r->type == integer)
	mpz_init_set_si (r->u.z, r->u.i);
      l->type = r->type = mp_integer;
      mpz_mul (l->u.z, l->u.z, r->u.z); /* L*=R */
#else
      abort ();
#endif
    }
}

/* L = L / R or (if WANT_MODULUS) L = L % R */
static void
dodivide (VALUE *l, VALUE *r, bool want_modulus)
{
  if (r->type == integer && r->u.i == 0)
    error (EXPR_INVALID, 0, _("division by zero"));
#if HAVE_GMP
  if (r->type == mp_integer && mpz_sgn (r->u.z) == 0)
    error (EXPR_INVALID, 0, _("division by zero"));
#endif
  if (l->type == integer && r->type == integer)
    {
      if (l->u.i < - INT_MAX && r->u.i == -1)
	{
	  /* Some x86-style hosts raise an exception for
	     INT_MIN / -1 and INT_MIN % -1, so handle these
	     problematic cases specially.  */
	  if (want_modulus)
	    {
	      /* X mod -1 is zero for all negative X.
		 Although strictly this is implementation-defined,
		 we don't want to coredump, so we avoid the calculation. */
	      l->u.i = 0;
	      return;
	    }
	  else
	    {
	      if (mode != MP_NEVER)
		{
#if HAVE_GMP
		  /* Handle the case by promoting. */
		  mpz_init_set_si (l->u.z, l->u.i);
		  l->type = mp_integer;
#endif
		}
	      else
		{
		  integer_overflow ('/');
		}
	    }
	}
      else
	{
	  l->u.i = want_modulus ? l->u.i % r->u.i : l->u.i / r->u.i;
	  return;
	}
    }
  /* If we get to here, at least one operand is mp_integer
     and R is not 0. */
#if HAVE_GMP
  {
    bool round_up = false;	/* do we round up? */
    int sign_l, sign_r;
    promote (l);
    promote (r);
    sign_l = mpz_sgn (l->u.z);
    sign_r = mpz_sgn (r->u.z);

    if (!want_modulus)
      {
	if (!sign_l)
	  {
	    mpz_set_si (l->u.z, 0);
	  }
	else if (sign_l < 0 || sign_r < 0)
	  {
	    /* At least one operand is negative.  For integer arithmetic,
	       it's platform-dependent if the operation rounds up or down.
	       We mirror what the implementation does. */
	    switch ((3*sign_l) / (2*sign_r))
	      {
	      case  2:		/* round toward +inf. */
	      case -1:		/* round toward +inf. */
		mpz_cdiv_q (l->u.z, l->u.z, r->u.z);
		break;
	      case -2:		/* round toward -inf. */
	      case  1:		/* round toward -inf */
		mpz_fdiv_q (l->u.z, l->u.z, r->u.z);
		break;
	      default:
		abort ();
	      }
	  }
	else
	  {
	    /* Both operands positive.  Round toward -inf. */
	    mpz_fdiv_q (l->u.z, l->u.z, r->u.z);
	  }
      }
    else
      {
	mpz_mod (l->u.z, l->u.z, r->u.z); /* L = L % R */

	/* If either operand is negative, it's platform-dependent if
	   the remainer is positive or negative.  We mirror what the
	   implementation does. */
	if (sign_l % sign_r < 0)
	  mpz_neg (l->u.z, l->u.z); /* L = (-L) */
      }
  }
#else
  abort ();
#endif
}


/* Handle *, /, % operators.  */

static VALUE *
eval4 (bool evaluate)
{
  VALUE *l;
  VALUE *r;
  enum { multiply, divide, mod } fxn;

#ifdef EVAL_TRACE
  trace ("eval4");
#endif
  l = eval5 (evaluate);
  while (1)
    {
      if (nextarg ("*"))
	fxn = multiply;
      else if (nextarg ("/"))
	fxn = divide;
      else if (nextarg ("%"))
	fxn = mod;
      else
	return l;
      r = eval5 (evaluate);
      if (evaluate)
	{
	  if (!toarith (l) || !toarith (r))
	    error (EXPR_INVALID, 0, _("non-numeric argument"));
	  switch (fxn)
	    {
	    case multiply:
	      domult (l, r);
	      break;
	    case divide:
	    case mod:
	      dodivide (l, r, fxn==mod);
	      break;
	    }
	}
      freev (r);
    }
}

/* L = L + R, or L = L - R */
static void
doadd (VALUE *l, VALUE *r, bool add)
{
  long int val = 0;

  if (!toarith (l) || !toarith (r))
    error (EXPR_INVALID, 0, _("non-numeric argument"));
  if (l->type == integer && r->type == integer)
    {
      if (add)
	{
	  val = l->u.i + r->u.i;
	  if ((val < l->u.i) == (r->u.i < 0))
	    {
	      l->u.i = val;
	      return;
	    }
	}
      else
	{
	  val = l->u.i - r->u.i;
	  if ((l->u.i < val) == (r->u.i < 0))
	    {
	      l->u.i = val;
	      return;
	    }
	}
    }
  /* If we get to here, either the operation overflowed or at least
     one operand is an mp_integer. */
  if (mode != MP_NEVER)
    {
#if HAVE_GMP
      promote (l);
      promote (r);
      if (add)
	mpz_add (l->u.z, l->u.z, r->u.z);
      else
	mpz_sub (l->u.z, l->u.z, r->u.z);
#endif
    }
  else
    {
      integer_overflow ('-');
    }
}



/* Handle +, - operators.  */

static VALUE *
eval3 (bool evaluate)
{
  VALUE *l;
  VALUE *r;
  bool add;

#ifdef EVAL_TRACE
  trace ("eval3");
#endif
  l = eval4 (evaluate);
  while (1)
    {
      if (nextarg ("+"))
	add = true;
      else if (nextarg ("-"))
	add = false;
      else
	return l;
      r = eval4 (evaluate);
      if (evaluate)
	{
	  doadd (l, r, add);
	}
      freev (r);
    }
}

/* Handle comparisons.  */

static VALUE *
eval2 (bool evaluate)
{
  VALUE *l;

#ifdef EVAL_TRACE
  trace ("eval2");
#endif
  l = eval3 (evaluate);
  while (1)
    {
      VALUE *r;
      enum
	{
	  less_than, less_equal, equal, not_equal, greater_equal, greater_than
	} fxn;
      bool val = false;

      if (nextarg ("<"))
	fxn = less_than;
      else if (nextarg ("<="))
	fxn = less_equal;
      else if (nextarg ("=") || nextarg ("=="))
	fxn = equal;
      else if (nextarg ("!="))
	fxn = not_equal;
      else if (nextarg (">="))
	fxn = greater_equal;
      else if (nextarg (">"))
	fxn = greater_than;
      else
	return l;
      r = eval3 (evaluate);

      if (evaluate)
	{
	  int cmp;
	  tostring (l);
	  tostring (r);

	  if (looks_like_integer (l->u.s) && looks_like_integer (r->u.s))
	    cmp = strintcmp (l->u.s, r->u.s);
	  else
	    {
	      errno = 0;
	      cmp = strcoll (l->u.s, r->u.s);

	      if (errno)
		{
		  error (0, errno, _("string comparison failed"));
		  error (0, 0, _("set LC_ALL='C' to work around the problem"));
		  error (EXPR_INVALID, 0,
			 _("the strings compared were %s and %s"),
			 quotearg_n_style (0, locale_quoting_style, l->u.s),
			 quotearg_n_style (1, locale_quoting_style, r->u.s));
		}
	    }

	  switch (fxn)
	    {
	    case less_than:     val = (cmp <  0); break;
	    case less_equal:    val = (cmp <= 0); break;
	    case equal:         val = (cmp == 0); break;
	    case not_equal:     val = (cmp != 0); break;
	    case greater_equal: val = (cmp >= 0); break;
	    case greater_than:  val = (cmp >  0); break;
	    default: abort ();
	    }
	}

      freev (l);
      freev (r);
      l = int_value (val);
    }
}

/* Handle &.  */

static VALUE *
eval1 (bool evaluate)
{
  VALUE *l;
  VALUE *r;

#ifdef EVAL_TRACE
  trace ("eval1");
#endif
  l = eval2 (evaluate);
  while (1)
    {
      if (nextarg ("&"))
	{
	  r = eval2 (evaluate & ~ null (l));
	  if (null (l) || null (r))
	    {
	      freev (l);
	      freev (r);
	      l = int_value (0);
	    }
	  else
	    freev (r);
	}
      else
	return l;
    }
}

/* Handle |.  */

static VALUE *
eval (bool evaluate)
{
  VALUE *l;
  VALUE *r;

#ifdef EVAL_TRACE
  trace ("eval");
#endif
  l = eval1 (evaluate);
  while (1)
    {
      if (nextarg ("|"))
	{
	  r = eval1 (evaluate & null (l));
	  if (null (l))
	    {
	      freev (l);
	      l = r;
	      if (null (l))
		{
		  freev (l);
		  l = int_value (0);
		}
	    }
	  else
	    freev (r);
	}
      else
	return l;
    }
}
