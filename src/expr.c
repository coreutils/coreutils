/* expr -- evaluate expressions.
   Copyright (C) 86, 1991-1997, 1999-2002 Free Software Foundation, Inc.

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
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

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
#include "closeout.h"
#include "inttostr.h"

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "expr"

#define AUTHORS "Mike Parker"

#undef NEW
#define NEW(Type) XMALLOC (Type, 1)
#define OLD(x) free ((char *) x)

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

static VALUE *eval (void);
static int nomoreargs (void);
static int null (VALUE *v);
static void printv (VALUE *v);

void
usage (int status)
{
  if (status != 0)
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
      printf (_("\nReport bugs to <%s>.\n"), PACKAGE_BUGREPORT);
    }
  exit (status);
}

int
main (int argc, char **argv)
{
  VALUE *v;

  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  parse_long_options (argc, argv, PROGRAM_NAME, GNU_PACKAGE, VERSION,
		      AUTHORS, usage);
  /* The above handles --help and --version.
     Since there is no other invocation of getopt, handle `--' here.  */
  if (argc > 1 && STREQ (argv[1], "--"))
    {
      --argc;
      ++argv;
    }

  if (argc == 1)
    {
      error (0, 0, _("too few arguments"));
      usage (EXIT_FAILURE);
    }

  args = argv + 1;

  v = eval ();
  if (!nomoreargs ())
    error (2, 0, _("syntax error"));
  printv (v);

  exit (null (v));
}

/* Return a VALUE for I.  */

static VALUE *
int_value (intmax_t i)
{
  VALUE *v;

  v = NEW (VALUE);
  v->type = integer;
  v->u.i = i;
  return v;
}

/* Return a VALUE for S.  */

static VALUE *
str_value (char *s)
{
  VALUE *v;

  v = NEW (VALUE);
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
  OLD (v);
}

/* Print VALUE V.  */

static void
printv (VALUE *v)
{
  char *p;
  char buf[INT_STRLEN_BOUND (intmax_t) + 1];

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

/* Return nonzero if V is a null-string or zero-number.  */

static int
null (VALUE *v)
{
  switch (v->type)
    {
    case integer:
      return v->u.i == 0;
    case string:
      return v->u.s[0] == '\0' || strcmp (v->u.s, "0") == 0;
    default:
      abort ();
    }
}

/* Coerce V to a string value (can't fail).  */

static void
tostring (VALUE *v)
{
  char buf[INT_STRLEN_BOUND (intmax_t) + 1];

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

/* Coerce V to an integer value.  Return 1 on success, 0 on failure.  */

static int
toarith (VALUE *v)
{
  intmax_t i;
  int neg;
  char *cp;

  switch (v->type)
    {
    case integer:
      return 1;
    case string:
      i = 0;
      cp = v->u.s;
      neg = (*cp == '-');
      if (neg)
	cp++;

      do
	{
	  if (ISDIGIT (*cp))
	    i = i * 10 + *cp - '0';
	  else
	    return 0;
	}
      while (*++cp);

      free (v->u.s);
      v->u.i = i * (neg ? -1 : 1);
      v->type = integer;
      return 1;
    default:
      abort ();
    }
}

/* Return nonzero and advance if the next token matches STR exactly.
   STR must not be NULL.  */

static int
nextarg (char *str)
{
  if (*args == NULL)
    return 0;
  else
    {
      int r = strcoll (*args, str) == 0;
      args += r;
      return r;
    }
}

/* Return nonzero if there no more tokens.  */

static int
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
  VALUE *v;
  const char *errmsg;
  struct re_pattern_buffer re_buffer;
  struct re_registers re_regs;
  size_t len;
  int matchlen;

  tostring (sv);
  tostring (pv);

  if (pv->u.s[0] == '^')
    {
      error (0, 0, _("\
warning: unportable BRE: `%s': using `^' as the first character\n\
of the basic regular expression is not portable; it is being ignored"),
	     pv->u.s);
    }

  len = strlen (pv->u.s);
  memset (&re_buffer, 0, sizeof (re_buffer));
  memset (&re_regs, 0, sizeof (re_regs));
  re_buffer.allocated = 2 * len;
  if (re_buffer.allocated < len)
    xalloc_die ();
  re_buffer.buffer = (unsigned char *) xmalloc (re_buffer.allocated);
  re_buffer.translate = 0;
  re_syntax_options = RE_SYNTAX_POSIX_BASIC;
  errmsg = re_compile_pattern (pv->u.s, len, &re_buffer);
  if (errmsg)
    error (2, 0, "%s", errmsg);

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
  else
    {
      /* Match failed -- return the right kind of null.  */
      if (re_buffer.re_nsub > 0)
	v = str_value ("");
      else
	v = int_value (0);
    }
  free (re_buffer.buffer);
  return v;
}

/* Handle bare operands and ( expr ) syntax.  */

static VALUE *
eval7 (void)
{
  VALUE *v;

#ifdef EVAL_TRACE
  trace ("eval7");
#endif
  if (nomoreargs ())
    error (2, 0, _("syntax error"));

  if (nextarg ("("))
    {
      v = eval ();
      if (!nextarg (")"))
	error (2, 0, _("syntax error"));
      return v;
    }

  if (nextarg (")"))
    error (2, 0, _("syntax error"));

  return str_value (*args++);
}

/* Handle match, substr, index, and length keywords, and quoting "+".  */

static VALUE *
eval6 (void)
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
	error (2, 0, _("syntax error"));
      return str_value (*args++);
    }
  else if (nextarg ("length"))
    {
      r = eval6 ();
      tostring (r);
      v = int_value (strlen (r->u.s));
      freev (r);
      return v;
    }
  else if (nextarg ("match"))
    {
      l = eval6 ();
      r = eval6 ();
      v = docolon (l, r);
      freev (l);
      freev (r);
      return v;
    }
  else if (nextarg ("index"))
    {
      l = eval6 ();
      r = eval6 ();
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
      l = eval6 ();
      i1 = eval6 ();
      i2 = eval6 ();
      tostring (l);
      if (!toarith (i1) || !toarith (i2)
	  || strlen (l->u.s) < i1->u.i
	  || i1->u.i <= 0 || i2->u.i <= 0)
	v = str_value ("");
      else
	{
	  v = NEW (VALUE);
	  v->type = string;
	  v->u.s = strncpy ((char *) xmalloc (i2->u.i + 1),
			    l->u.s + i1->u.i - 1, i2->u.i);
	  v->u.s[i2->u.i] = 0;
	}
      freev (l);
      freev (i1);
      freev (i2);
      return v;
    }
  else
    return eval7 ();
}

/* Handle : operator (pattern matching).
   Calls docolon to do the real work.  */

static VALUE *
eval5 (void)
{
  VALUE *l;
  VALUE *r;
  VALUE *v;

#ifdef EVAL_TRACE
  trace ("eval5");
#endif
  l = eval6 ();
  while (1)
    {
      if (nextarg (":"))
	{
	  r = eval6 ();
	  v = docolon (l, r);
	  freev (l);
	  freev (r);
	  l = v;
	}
      else
	return l;
    }
}

/* Handle *, /, % operators.  */

static VALUE *
eval4 (void)
{
  VALUE *l;
  VALUE *r;
  enum { multiply, divide, mod } fxn;
  intmax_t val;

#ifdef EVAL_TRACE
  trace ("eval4");
#endif
  l = eval5 ();
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
      r = eval5 ();
      if (!toarith (l) || !toarith (r))
	error (2, 0, _("non-numeric argument"));
      if (fxn == multiply)
	val = l->u.i * r->u.i;
      else
	{
	  if (r->u.i == 0)
	    error (2, 0, _("division by zero"));
	  val = fxn == divide ? l->u.i / r->u.i : l->u.i % r->u.i;
	}
      freev (l);
      freev (r);
      l = int_value (val);
    }
}

/* Handle +, - operators.  */

static VALUE *
eval3 (void)
{
  VALUE *l;
  VALUE *r;
  enum { plus, minus } fxn;
  intmax_t val;

#ifdef EVAL_TRACE
  trace ("eval3");
#endif
  l = eval4 ();
  while (1)
    {
      if (nextarg ("+"))
	fxn = plus;
      else if (nextarg ("-"))
	fxn = minus;
      else
	return l;
      r = eval4 ();
      if (!toarith (l) || !toarith (r))
	error (2, 0, _("non-numeric argument"));
      val = fxn == plus ? l->u.i + r->u.i : l->u.i - r->u.i;
      freev (l);
      freev (r);
      l = int_value (val);
    }
}

/* Handle comparisons.  */

static VALUE *
eval2 (void)
{
  VALUE *l;
  VALUE *r;
  enum
  {
    less_than, less_equal, equal, not_equal, greater_equal, greater_than
  } fxn;
  int val;
  intmax_t lval;
  intmax_t rval;

#ifdef EVAL_TRACE
  trace ("eval2");
#endif
  l = eval3 ();
  while (1)
    {
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
      r = eval3 ();
      tostring (l);
      tostring (r);
      lval = strcoll (l->u.s, r->u.s);
      rval = 0;
      if (toarith (l) && toarith (r))
	{
	  lval = l->u.i;
	  rval = r->u.i;
	}
      switch (fxn)
	{
	case less_than:     val = (lval <  rval); break;
	case less_equal:    val = (lval <= rval); break;
	case equal:         val = (lval == rval); break;
	case not_equal:     val = (lval != rval); break;
	case greater_equal: val = (lval >= rval); break;
	case greater_than:  val = (lval >  rval); break;
	default: abort ();
	}
      freev (l);
      freev (r);
      l = int_value (val);
    }
}

/* Handle &.  */

static VALUE *
eval1 (void)
{
  VALUE *l;
  VALUE *r;

#ifdef EVAL_TRACE
  trace ("eval1");
#endif
  l = eval2 ();
  while (1)
    {
      if (nextarg ("&"))
	{
	  r = eval2 ();
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
eval (void)
{
  VALUE *l;
  VALUE *r;

#ifdef EVAL_TRACE
  trace ("eval");
#endif
  l = eval1 ();
  while (1)
    {
      if (nextarg ("|"))
	{
	  r = eval1 ();
	  if (null (l))
	    {
	      freev (l);
	      l = r;
	    }
	  else
	    freev (r);
	}
      else
	return l;
    }
}
