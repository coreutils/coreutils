/* expr -- evaluate expressions.
   Copyright (C) 86, 1991-1997, 1999-2005 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

/* Author: Mike Parker.

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

#include <regex.h>
#include "long-options.h"
#include "error.h"
#include "inttostr.h"
#include "quote.h"
#include "quotearg.h"
#include "strnumcmp.h"
#include "xstrtol.h"

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "expr"

#define AUTHORS "Mike Parker"

/* Exit statuses.  */
enum
  {
    /* Invalid expression: i.e., its form does not conform to the
       grammar for expressions.  Our grammar is an extension of the
       POSIX grammar.  */
    EXPR_INVALID = 2,

    /* Some other error occurred.  */
    EXPR_FAILURE
  };

/* The kinds of value we can have.  */
enum valtype
{
  integer,
  string
};
typedef enum valtype TYPE;

/* A value is.... */
struct valinfo
{
  TYPE type;			/* Which kind. */
  union
  {				/* The value itself. */
    intmax_t i;
    char *s;
  } u;
};
typedef struct valinfo VALUE;

/* The arguments given to the program, minus the program name.  */
static char **args;

/* The name this program was run with. */
char *program_name;

static VALUE *eval (bool);
static bool nomoreargs (void);
static bool null (VALUE *v);
static void printv (VALUE *v);

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
      printf (_("\nReport bugs to <%s>.\n"), PACKAGE_BUGREPORT);
    }
  exit (status);
}

/* Report a syntax error and exit.  */
static void
syntax_error (void)
{
  error (EXPR_INVALID, 0, _("syntax error"));
}

int
main (int argc, char **argv)
{
  VALUE *v;

  initialize_main (&argc, &argv);
  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  initialize_exit_failure (EXPR_FAILURE);
  atexit (close_stdout);

  parse_long_options (argc, argv, PROGRAM_NAME, GNU_PACKAGE, VERSION,
		      usage, AUTHORS, (char const *) NULL);
  /* The above handles --help and --version.
     Since there is no other invocation of getopt, handle `--' here.  */
  if (argc > 1 && STREQ (argv[1], "--"))
    {
      --argc;
      ++argv;
    }

  if (argc <= 1)
    {
      error (0, 0, _("missing operand"));
      usage (EXPR_INVALID);
    }

  args = argv + 1;

  v = eval (true);
  if (!nomoreargs ())
    syntax_error ();
  printv (v);

  exit (null (v));
}

/* Return a VALUE for I.  */

static VALUE *
int_value (intmax_t i)
{
  VALUE *v = xmalloc (sizeof *v);
  v->type = integer;
  v->u.i = i;
  return v;
}

/* Return a VALUE for S.  */

static VALUE *
str_value (char *s)
{
  VALUE *v = xmalloc (sizeof *v);
  v->type = string;
  v->u.s = xstrdup (s);
  return v;
}

/* Free VALUE V, including structure components.  */

static void
freev (VALUE *v)
{
  if (v->type == string)
    free (v->u.s);
  free (v);
}

/* Print VALUE V.  */

static void
printv (VALUE *v)
{
  char *p;
  char buf[INT_BUFSIZE_BOUND (intmax_t)];

  switch (v->type)
    {
    case integer:
      p = imaxtostr (v->u.i, buf);
      break;
    case string:
      p = v->u.s;
      break;
    default:
      abort ();
    }

  puts (p);
}

/* Return true if V is a null-string or zero-number.  */

static bool
null (VALUE *v)
{
  switch (v->type)
    {
    case integer:
      return v->u.i == 0;
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
  char buf[INT_BUFSIZE_BOUND (intmax_t)];

  switch (v->type)
    {
    case integer:
      v->u.s = xstrdup (imaxtostr (v->u.i, buf));
      v->type = string;
      break;
    case string:
      break;
    default:
      abort ();
    }
}

/* Coerce V to an integer value.  Return true on success, false on failure.  */

static bool
toarith (VALUE *v)
{
  switch (v->type)
    {
    case integer:
      return true;
    case string:
      {
	intmax_t value;

	if (! looks_like_integer (v->u.s))
	  return false;
	if (xstrtoimax (v->u.s, NULL, 10, &value, NULL) != LONGINT_OK)
	  error (EXPR_FAILURE, ERANGE, "%s", v->u.s);
	free (v->u.s);
	v->u.i = value;
	v->type = integer;
	return true;
      }
    default:
      abort ();
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
  struct re_registers re_regs;
  size_t len;
  regoff_t matchlen;

  tostring (sv);
  tostring (pv);

  if (pv->u.s[0] == '^')
    {
      error (0, 0, _("\
warning: unportable BRE: %s: using `^' as the first character\n\
of the basic regular expression is not portable; it is being ignored"),
	     quote (pv->u.s));
    }

  len = strlen (pv->u.s);
  memset (&re_buffer, 0, sizeof (re_buffer));
  memset (&re_regs, 0, sizeof (re_regs));
  re_buffer.buffer = xnmalloc (len, 2);
  re_buffer.allocated = 2 * len;
  re_buffer.translate = NULL;
  re_syntax_options = RE_SYNTAX_POSIX_BASIC;
  errmsg = re_compile_pattern (pv->u.s, len, &re_buffer);
  if (errmsg)
    error (EXPR_FAILURE, 0, "%s", errmsg);

  matchlen = re_match (&re_buffer, sv->u.s, strlen (sv->u.s), 0, &re_regs);
  if (0 <= matchlen)
    {
      /* Were \(...\) used? */
      if (re_buffer.re_nsub > 0)/* was (re_regs.start[1] >= 0) */
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

  free (re_buffer.buffer);
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
      l = eval6 (evaluate);
      r = eval6 (evaluate);
      tostring (l);
      tostring (r);
      v = int_value (strcspn (l->u.s, r->u.s) + 1);
      if (v->u.i == strlen (l->u.s) + 1)
	v->u.i = 0;
      freev (l);
      freev (r);
      return v;
    }
  else if (nextarg ("substr"))
    {
      l = eval6 (evaluate);
      i1 = eval6 (evaluate);
      i2 = eval6 (evaluate);
      tostring (l);
      if (!toarith (i1) || !toarith (i2)
	  || strlen (l->u.s) < i1->u.i
	  || i1->u.i <= 0 || i2->u.i <= 0)
	v = str_value ("");
      else
	{
	  v = xmalloc (sizeof *v);
	  v->type = string;
	  v->u.s = strncpy (xmalloc (i2->u.i + 1),
			    l->u.s + i1->u.i - 1, i2->u.i);
	  v->u.s[i2->u.i] = 0;
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

/* Handle *, /, % operators.  */

static VALUE *
eval4 (bool evaluate)
{
  VALUE *l;
  VALUE *r;
  enum { multiply, divide, mod } fxn;
  intmax_t val = 0;

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
	    error (EXPR_FAILURE, 0, _("non-numeric argument"));
	  if (fxn == multiply)
	    val = l->u.i * r->u.i;
	  else
	    {
	      if (r->u.i == 0)
		error (EXPR_FAILURE, 0, _("division by zero"));
	      val = fxn == divide ? l->u.i / r->u.i : l->u.i % r->u.i;
	    }
	}
      freev (l);
      freev (r);
      l = int_value (val);
    }
}

/* Handle +, - operators.  */

static VALUE *
eval3 (bool evaluate)
{
  VALUE *l;
  VALUE *r;
  enum { plus, minus } fxn;
  intmax_t val = 0;

#ifdef EVAL_TRACE
  trace ("eval3");
#endif
  l = eval4 (evaluate);
  while (1)
    {
      if (nextarg ("+"))
	fxn = plus;
      else if (nextarg ("-"))
	fxn = minus;
      else
	return l;
      r = eval4 (evaluate);
      if (evaluate)
	{
	  if (!toarith (l) || !toarith (r))
	    error (EXPR_FAILURE, 0, _("non-numeric argument"));
	  val = fxn == plus ? l->u.i + r->u.i : l->u.i - r->u.i;
	}
      freev (l);
      freev (r);
      l = int_value (val);
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
		  error (0, 0, _("Set LC_ALL='C' to work around the problem."));
		  error (EXPR_FAILURE, 0,
			 _("The strings compared were %s and %s."),
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
