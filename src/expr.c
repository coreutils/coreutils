/* expr -- evaluate expressions.
   Copyright (C) 1986, 1991 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* Author: Mike Parker.

   This program evaluates expressions.  Each token (operator, operand,
   parenthesis) of the expression must be a seperate argument.  The
   parser used is a reasonably general one, though any incarnation of
   it is language-specific.  It is especially nice for expressions.

   No parse tree is needed; a new node is evaluated immediately.
   One function can handle multiple operators all of equal precedence,
   provided they all associate ((x op x) op x).

   Define EVAL_TRACE to print an evaluation trace.  */

#ifdef HAVE_CONFIG_H
#if defined (CONFIG_BROKETS)
/* We use <config.h> instead of "config.h" so that a compilation
   using -I. -I$srcdir will use ./config.h rather than $srcdir/config.h
   (which it would do because it found this file in $srcdir).  */
#include <config.h>
#else
#include "config.h"
#endif
#endif

#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <regex.h>

#include "system.h"
#include "version.h"
#include "long-options.h"

#if defined (STDC_HEADERS) || (!defined (isascii) && !defined (HAVE_ISASCII))
#define ISASCII(c) 1
#else
#define ISASCII(c) isascii(c)
#endif

#define ISDIGIT(c) (ISASCII (c) && isdigit (c))

#define NEW(type) ((type *) xmalloc (sizeof (type)))
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
    int i;
    char *s;
  } u;
};
typedef struct valinfo VALUE;

/* The arguments given to the program, minus the program name.  */
static char **args;

/* The name this program was run with. */
char *program_name;

void error ();
char *xstrdup ();
char *strstr ();
char *xmalloc ();

static VALUE *docolon ();
static VALUE *eval ();
static VALUE *int_value ();
static VALUE *str_value ();
static int isstring ();
static int nextarg ();
static int nomoreargs ();
static int null ();
static int toarith ();
static void freev ();
static void printv ();
static void tostring ();

#ifdef EVAL_TRACE
static void trace ();
#endif

static void
usage (status)
     int status;
{
  if (status != 0)
    fprintf (stderr, "Try `%s --help' for more information.\n",
	     program_name);
  else
    {
      printf ("\
Usage: %s EXPRESSION\n\
  or:  %s OPTION\n\
",
	      program_name, program_name);
      printf ("\
\n\
  --help      display this help and exit\n\
  --version   output version information and exit\n\
\n\
");
      printf ("\
EXPRESSION value is written on standard output.  A white line\n\
separates increasing precedence groups.  EXPRESSION may be:\n\
\n\
  ARG1 | ARG2       ARG1 if it is neither null nor 0, otherwise ARG2\n\
\n\
  ARG1 & ARG2       ARG1 if neither argument is null or 0, otherwise 0\n\
\n\
  ARG1 < ARG2       ARG1 is less than ARG2\n\
  ARG1 <= ARG2      ARG1 is less than or equal to ARG2\n\
  ARG1 = ARG2       ARG1 is equal to ARG2\n\
  ARG1 != ARG2      ARG1 is unequal to ARG2\n\
  ARG1 >= ARG2      ARG1 is greater than or equal to ARG2\n\
  ARG1 > ARG2       ARG1 is greater than ARG2\n\
\n\
  ARG1 + ARG2       arithmetic sum of ARG1 and ARG2\n\
  ARG1 - ARG2       arithmetic difference of ARG1 and ARG2\n\
\n\
  ARG1 * ARG2       arithmetic product of ARG1 and ARG2\n\
  ARG1 / ARG2       arithmetic quotient of ARG1 divided by ARG2\n\
  ARG1 %% ARG2       arithmetic remainder of ARG1 divided by ARG2\n\
\n\
  STRING : REGEXP   anchored pattern match of REGEXP in STRING\n\
\n\
  match STRING REGEXP        same as STRING : REGEXP\n\
  substr STRING POS LENGTH   substring of STRING, POS counted from 1\n\
  index STRING CHARS         index in STRING where any CHARS is found, or 0\n\
  length STRING              length of STRING\n\
\n\
  ( EXPRESSION )             value of EXPRESSION\n\
");
      printf ("\
\n\
Beware that some operators need to be escaped by backslashes for shells.\n\
Comparisons are arithmetic if both ARGs are numbers, else lexicographical.\n\
Pattern matches return the string matched between \\( and \\) or null; if\n\
\\( and \\) are not used, they return the number of characters matched or 0.\n\
");
    }
  exit (status);
}

void
main (argc, argv)
     int argc;
     char **argv;
{
  VALUE *v;

  program_name = argv[0];

  parse_long_options (argc, argv, usage);

  if (argc == 1)
    usage (1);
  args = argv + 1;

  v = eval ();
  if (!nomoreargs ())
    error (2, 0, "syntax error");
  printv (v);

  exit (null (v));
}

/* Return a VALUE for I.  */

static VALUE *
int_value (i)
     int i;
{
  VALUE *v;

  v = NEW (VALUE);
  v->type = integer;
  v->u.i = i;
  return v;
}

/* Return a VALUE for S.  */

static VALUE *
str_value (s)
     char *s;
{
  VALUE *v;

  v = NEW (VALUE);
  v->type = string;
  v->u.s = xstrdup (s);
  return v;
}

/* Free VALUE V, including structure components.  */

static void
freev (v)
     VALUE *v;
{
  if (v->type == string)
    free (v->u.s);
  OLD (v);
}

/* Print VALUE V.  */

static void
printv (v)
     VALUE *v;
{
  switch (v->type)
    {
    case integer:
      printf ("%d\n", v->u.i);
      break;
    case string:
      printf ("%s\n", v->u.s);
      break;
    default:
      abort ();
    }
}

/* Return nonzero if V is a null-string or zero-number.  */

static int
null (v)
     VALUE *v;
{
  switch (v->type)
    {
    case integer:
      return v->u.i == 0;
    case string:
      return v->u.s[0] == '\0' || strcmp(v->u.s, "0") == 0;
    default:
      abort ();
    }
}

/* Return nonzero if V is a string value.  */

static int
isstring (v)
     VALUE *v;
{
  return v->type == string;
}

/* Coerce V to a string value (can't fail).  */

static void
tostring (v)
     VALUE *v;
{
  char *temp;

  switch (v->type)
    {
    case integer:
      temp = xmalloc (4 * (sizeof (int) / sizeof (char)));
      sprintf (temp, "%d", v->u.i);
      v->u.s = temp;
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
toarith (v)
     VALUE *v;
{
  int i;
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
      for (; *cp; cp++)
	{
	  if (ISDIGIT (*cp))
	    i = i * 10 + *cp - '0';
	  else
	    return 0;
	}
      free (v->u.s);
      v->u.i = i * (neg ? -1 : 1);
      v->type = integer;
      return 1;
    default:
      abort ();
    }
}

/* Return nonzero if the next token matches STR exactly.
   STR must not be NULL.  */

static int
nextarg (str)
     char *str;
{
  if (*args == NULL)
    return 0;
  return strcmp (*args, str) == 0;
}

/* Return nonzero if there no more tokens.  */

static int
nomoreargs ()
{
  return *args == 0;
}

/* The comparison operator handling functions.  */

#define cmpf(name, rel)				\
static						\
int name (l, r) VALUE *l; VALUE *r;		\
{						\
  if (isstring (l) || isstring (r))		\
    {						\
       tostring (l);				\
       tostring (r);				\
       return strcmp (l->u.s, r->u.s) rel 0;	\
    }						\
 else						\
   return l->u.i rel r->u.i;			\
}
cmpf (less_than, <)
cmpf (less_equal, <=)
cmpf (equal, ==)
cmpf (not_equal, !=)
cmpf (greater_equal, >=)
cmpf (greater_than, >)

#undef cmpf

/* The arithmetic operator handling functions.  */

#define arithf(name, op)			\
static						\
int name (l, r) VALUE *l; VALUE *r;		\
{						\
  if (!toarith (l) || !toarith (r))		\
    error (2, 0, "non-numeric argument");	\
  return l->u.i op r->u.i;			\
}

#define arithdivf(name, op)			\
int name (l, r) VALUE *l; VALUE *r;		\
{						\
  if (!toarith (l) || !toarith (r))		\
    error (2, 0, "non-numeric argument");	\
  if (r->u.i == 0)				\
    error (2, 0, "division by zero");		\
  return l->u.i op r->u.i;			\
}

arithf (plus, +)
arithf (minus, -)
arithf (multiply, *)
arithdivf (divide, /)
arithdivf (mod, %)

#undef arithf
#undef arithdivf

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
docolon (sv, pv)
     VALUE *sv;
     VALUE *pv;
{
  VALUE *v;
  const char *errmsg;
  struct re_pattern_buffer re_buffer;
  struct re_registers re_regs;
  int len;

  tostring (sv);
  tostring (pv);

  len = strlen (pv->u.s);
  re_buffer.allocated = 2 * len;
  re_buffer.buffer = (unsigned char *) xmalloc (re_buffer.allocated);
  re_buffer.translate = 0;
  errmsg = re_compile_pattern (pv->u.s, len, &re_buffer);
  if (errmsg)
    error (2, 0, "%s", errmsg);

  len = re_match (&re_buffer, sv->u.s, strlen (sv->u.s), 0, &re_regs);
  if (len >= 0)
    {
      /* Were \(...\) used? */
      if (re_buffer.re_nsub > 0)/* was (re_regs.start[1] >= 0) */
	{
	  sv->u.s[re_regs.end[1]] = '\0';
	  v = str_value (sv->u.s + re_regs.start[1]);
	}
      else
	v = int_value (len);
    }
  else
    {
      /* Match failed -- return the right kind of null.  */
      if (strstr (pv->u.s, "\\("))
	v = str_value ("");
      else
	v = int_value (0);
    }
  free (re_buffer.buffer);
  return v;
}

/* Handle bare operands and ( expr ) syntax.  */

static VALUE *
eval7 ()
{
  VALUE *v;

#ifdef EVAL_TRACE
  trace ("eval7");
#endif
  if (nomoreargs ())
    error (2, 0, "syntax error");

  if (nextarg ("("))
    {
      args++;
      v = eval ();
      if (!nextarg (")"))
	error (2, 0, "syntax error");
      args++;
      return v;
    }

  if (nextarg (")"))
    error (2, 0, "syntax error");

  return str_value (*args++);
}

/* Handle match, substr, index, and length keywords.  */

static VALUE *
eval6 ()
{
  VALUE *l;
  VALUE *r;
  VALUE *v;
  VALUE *i1;
  VALUE *i2;

#ifdef EVAL_TRACE
  trace ("eval6");
#endif
  if (nextarg ("length"))
    {
      args++;
      r = eval6 ();
      tostring (r);
      v = int_value (strlen (r->u.s));
      freev (r);
      return v;
    }
  else if (nextarg ("match"))
    {
      args++;
      l = eval6 ();
      r = eval6 ();
      v = docolon (l, r);
      freev (l);
      freev (r);
      return v;
    }
  else if (nextarg ("index"))
    {
      args++;
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
      args++;
      l = eval6 ();
      i1 = eval6 ();
      i2 = eval6 ();
      tostring (l);
      if (!toarith (i1) || !toarith (i2)
	  || i1->u.i > strlen (l->u.s)
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
eval5 ()
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
	  args++;
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
eval4 ()
{
  VALUE *l;
  VALUE *r;
  int (*fxn) ();
  int val;

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
      args++;
      r = eval5 ();
      val = (*fxn) (l, r);
      freev (l);
      freev (r);
      l = int_value (val);
    }
}

/* Handle +, - operators.  */

static VALUE *
eval3 ()
{
  VALUE *l;
  VALUE *r;
  int (*fxn) ();
  int val;

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
      args++;
      r = eval4 ();
      val = (*fxn) (l, r);
      freev (l);
      freev (r);
      l = int_value (val);
    }
}

/* Handle comparisons.  */

static VALUE *
eval2 ()
{
  VALUE *l;
  VALUE *r;
  int (*fxn) ();
  int val;

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
      args++;
      r = eval3 ();
      toarith (l);
      toarith (r);
      val = (*fxn) (l, r);
      freev (l);
      freev (r);
      l = int_value (val);
    }
}

/* Handle &.  */

static VALUE *
eval1 ()
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
	  args++;
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
eval ()
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
	  args++;
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
