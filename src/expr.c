/* expr -- evaluate expressions.
   Copyright (C) 1986-2025 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* Author: Mike Parker.
   Modified for arbitrary-precision calculation by James Youngman.

   This program evaluates expressions.  Each token (operator, operand,
   parenthesis) of the expression must be a separate argument.  The
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

#include <gmp.h>
#include <regex.h>
#include "c-ctype.h"
#include "long-options.h"
#include "mcel.h"
#include "strnumcmp.h"
#include "xstrtol.h"

/* Various parts of this code assume size_t fits into unsigned long
   int, the widest unsigned type that GMP supports.  */
static_assert (SIZE_MAX <= ULONG_MAX);

/* The official name of this program (e.g., no 'g' prefix).  */
#define PROGRAM_NAME "expr"

#define AUTHORS \
  proper_name ("Mike Parker"), \
  proper_name ("James Youngman"), \
  proper_name ("Paul Eggert")

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
    mpz_t i;
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


/*
   Find the first occurrence in the character string STRING of any character
   in the character string ACCEPT.

   Copied from gnulib's mbscspn, with two differences:
   1. Returns 1-based position of first found character, or zero if not found.
   2. Returned value is the logical character index, NOT byte offset.

   Examples:
     mbs_logical_cspn ('hello','a')  => 0
     mbs_logical_cspn ('hello','h')  => 1
     mbs_logical_cspn ('hello','oe') => 1
     mbs_logical_cspn ('hello','lo') => 3

   In UTF-8 \xCE\xB1 is a single character (greek alpha):
     mbs_logical_cspn ('\xCE\xB1bc','\xCE\xB1') => 1
     mbs_logical_cspn ('\xCE\xB1bc','c') => 3 */
static size_t
mbs_logical_cspn (char const *s, char const *accept)
{
  size_t idx = 0;

  if (accept[0] == '\0')
    return 0;

  /* General case.  */
  if (MB_CUR_MAX > 1)
    {
      for (char const *p = s; *p; )
        {
          ++idx;
          mcel_t g = mcel_scanz (p);
          if (g.len == 1)
            {
              if (mbschr (accept, *p))
                return idx;
            }
          else
            for (char const *a = accept; *a; )
              {
                mcel_t h = mcel_scanz (a);
                if (mcel_cmp (g, h) == 0)
                  return idx;
                a += h.len;
              }
          p += g.len;
        }
    }
  else
    {
      /* single-byte locale,
         convert returned byte offset to 1-based index or zero if not found. */
      size_t i = strcspn (s, accept);
      if (s[i])
        return i + 1;
    }

  /* not found */
  return 0;
}

/* Extract the substring of S, from logical character
   position POS and LEN characters.
   first character position is 1.
   POS and LEN refer to logical characters, not octets.

   Upon exit, sets v->s to the new string.
   The new string might be empty if POS/LEN are invalid. */
static char *
mbs_logical_substr (char const *s, size_t pos, size_t len)
{
  size_t mb_cur_max = MB_CUR_MAX;
  idx_t llen = mb_cur_max <= 1 ? strlen (s) : mbslen (s); /* logical length */

  /* characters to copy */
  size_t vlen = MIN (len, pos <= llen ? llen - pos + 1 : 0);

  char const *substart = s;
  idx_t sublen = 0;
  if (pos == 0 || len == SIZE_MAX)
    {
      /* The request is invalid.  Silently yield an empty string.  */
    }
  else if (mb_cur_max <= 1)
    {
      substart += pos - 1;
      sublen = vlen;
    }
  else
    for (idx_t idx = 1; *s && vlen; idx++)
      {
        idx_t char_bytes = mcel_scanz (s).len;

        /* Skip until we reach the starting position.  */
        if (pos <= idx)
          {
            if (pos == idx)
              substart = s;

            /* Add one character's length in bytes.  */
            vlen--;
            sublen += char_bytes;
          }

        s += char_bytes;
      }

  return ximemdup0 (substart, sublen);
}

/* Return the number of logical characters (possibly multibyte)
   that are in string S in the first OFS octets.

   Example in UTF-8:
   "\xE2\x9D\xA7" is "U+2767 ROTATED FLORAL HEART BULLET".
   In the string below, there are only two characters
   up to the first 4 bytes (The U+2767 which occupies 3 bytes and 'x'):
      mbs_count_to_offset ("\xE2\x9D\xA7xyz", 4) => 2  */
static size_t
mbs_offset_to_chars (char const *s, size_t ofs)
{
  size_t c = 0;
  for (size_t d = 0; d < ofs && s[d]; d += mcel_scanz (s + d).len)
    c++;
  return c;
}



void
usage (int status)
{
  if (status != EXIT_SUCCESS)
    emit_try_help ();
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
                               keyword like 'match' or an operator like '/'\n\
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
      emit_ancillary_info (PROGRAM_NAME);
    }
  exit (status);
}


int
main (int argc, char **argv)
{
  VALUE *v;

  initialize_main (&argc, &argv);
  set_program_name (argv[0]);
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  initialize_exit_failure (EXPR_FAILURE);
  atexit (close_stdout);

  parse_long_options (argc, argv, PROGRAM_NAME, PACKAGE_NAME, VERSION,
                      usage, AUTHORS, (char const *) nullptr);

  /* The above handles --help and --version.
     Since there is no other invocation of getopt, handle '--' here.  */
  if (1 < argc && streq (argv[1], "--"))
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
    error (EXPR_INVALID, 0, _("syntax error: unexpected argument %s"),
           quotearg_n_style (0, locale_quoting_style, *args));

  printv (v);

  main_exit (null (v));
}

/* Return a VALUE for I.  */

static VALUE *
int_value (unsigned long int i)
{
  VALUE *v = xmalloc (sizeof *v);
  v->type = integer;
  mpz_init_set_ui (v->u.i, i);
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

/* Free VALUE V, including structure components.  */

static void
freev (VALUE *v)
{
  if (v->type == string)
    free (v->u.s);
  else
    mpz_clear (v->u.i);
  free (v);
}

/* Print VALUE V.  */

static void
printv (VALUE *v)
{
  switch (v->type)
    {
    case integer:
      mpz_out_str (stdout, 10, v->u.i);
      putchar ('\n');
      break;
    case string:
      puts (v->u.s);
      break;
    default:
      unreachable ();
    }
}

/* Return true if V is a null-string or zero-number.  */

ATTRIBUTE_PURE
static bool
null (VALUE *v)
{
  switch (v->type)
    {
    case integer:
      return mpz_sgn (v->u.i) == 0;
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
      unreachable ();
    }
}

/* Return true if CP takes the form of an integer.  */

ATTRIBUTE_PURE
static bool
looks_like_integer (char const *cp)
{
  cp += (*cp == '-');

  do
    if (! c_isdigit (*cp))
      return false;
  while (*++cp);

  return true;
}

/* Coerce V to a string value (can't fail).  */

static void
tostring (VALUE *v)
{
  switch (v->type)
    {
    case integer:
      {
        char *s = mpz_get_str (nullptr, 10, v->u.i);
        mpz_clear (v->u.i);
        v->u.s = s;
        v->type = string;
      }
      break;
    case string:
      break;
    default:
      unreachable ();
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
        char *s = v->u.s;

        if (! looks_like_integer (s))
          return false;
        if (mpz_init_set_str (v->u.i, s, 10) != 0)
          error (EXPR_FAILURE, ERANGE, "%s", (s));
        free (s);
        v->type = integer;
        return true;
      }
    default:
      unreachable ();
    }
}

/* Extract a size_t value from an integer value I.
   If the value is negative, return SIZE_MAX.
   If the value is too large, return SIZE_MAX - 1.  */
static size_t
getsize (mpz_t i)
{
  if (mpz_sgn (i) < 0)
    return SIZE_MAX;
  if (mpz_fits_ulong_p (i))
    {
      unsigned long int ul = mpz_get_ui (i);
      if (ul < SIZE_MAX)
        return ul;
    }
  return SIZE_MAX - 1;
}

/* Return true and advance if the next token matches STR exactly.
   STR must not be null.  */

static bool
nextarg (char const *str)
{
  if (*args == nullptr)
    return false;
  else
    {
      bool r = streq (*args, str);
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

/* Report missing operand.
   There is an implicit assumption that there was a previous argument,
   and (args-1) is valid. */
static void
require_more_args (void)
{
  if (nomoreargs ())
    error (EXPR_INVALID, 0, _("syntax error: missing argument after %s"),
           quotearg_n_style (0, locale_quoting_style, *(args - 1)));
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
  char const *errmsg;
  struct re_pattern_buffer re_buffer;
  char fastmap[UCHAR_MAX + 1];
  struct re_registers re_regs;
  regoff_t matchlen;

  tostring (sv);
  tostring (pv);

  re_regs.num_regs = 0;
  re_regs.start = nullptr;
  re_regs.end = nullptr;

  re_buffer.buffer = nullptr;
  re_buffer.allocated = 0;
  re_buffer.fastmap = fastmap;
  re_buffer.translate = nullptr;
  re_syntax_options =
    RE_SYNTAX_POSIX_BASIC & ~RE_CONTEXT_INVALID_DUP & ~RE_NO_EMPTY_RANGES;
  errmsg = re_compile_pattern (pv->u.s, strlen (pv->u.s), &re_buffer);
  if (errmsg)
    error (EXPR_INVALID, 0, "%s", (errmsg));
  re_buffer.newline_anchor = 0;

  matchlen = re_match (&re_buffer, sv->u.s, strlen (sv->u.s), 0, &re_regs);
  if (0 <= matchlen)
    {
      /* Were \(...\) used? */
      if (re_buffer.re_nsub > 0)
        {
          if (re_regs.end[1] < 0)
            v = str_value ("");
          else
            {
              sv->u.s[re_regs.end[1]] = '\0';
              v = str_value (sv->u.s + re_regs.start[1]);
            }
        }
      else
        {
          /* In multibyte locales, convert the matched offset (=number of bytes)
             to the number of matched characters. */
          size_t i = (MB_CUR_MAX == 1
                      ? matchlen
                      : mbs_offset_to_chars (sv->u.s, matchlen));
          v = int_value (i);
        }
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
           matchlen == -2 ? errno : EOVERFLOW,
           _("error in regular expression matcher"));

  if (0 < re_regs.num_regs)
    {
      free (re_regs.start);
      free (re_regs.end);
    }
  re_buffer.fastmap = nullptr;
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
  require_more_args ();

  if (nextarg ("("))
    {
      v = eval (evaluate);
      if (nomoreargs ())
        error (EXPR_INVALID, 0, _("syntax error: expecting ')' after %s"),
               quotearg_n_style (0, locale_quoting_style, *(args - 1)));
      if (!nextarg (")"))
        error (EXPR_INVALID, 0, _("syntax error: expecting ')' instead of %s"),
               quotearg_n_style (0, locale_quoting_style, *args));
      return v;
    }

  if (nextarg (")"))
    error (EXPR_INVALID, 0, _("syntax error: unexpected ')'"));

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
      require_more_args ();
      return str_value (*args++);
    }
  else if (nextarg ("length"))
    {
      r = eval6 (evaluate);
      tostring (r);
      v = int_value (mbslen (r->u.s));
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
      size_t pos;

      l = eval6 (evaluate);
      r = eval6 (evaluate);
      tostring (l);
      tostring (r);
      pos = mbs_logical_cspn (l->u.s, r->u.s);
      v = int_value (pos);
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

      if (!toarith (i1) || !toarith (i2))
        v = str_value ("");
      else
        {
          size_t pos = getsize (i1->u.i);
          size_t len = getsize (i2->u.i);

          char *s = mbs_logical_substr (l->u.s, pos, len);
          v = str_value (s);
          free (s);
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
  while (true)
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

#ifdef EVAL_TRACE
  trace ("eval4");
#endif
  l = eval5 (evaluate);
  while (true)
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
            error (EXPR_INVALID, 0, _("non-integer argument"));
          if (fxn != multiply && mpz_sgn (r->u.i) == 0)
            error (EXPR_INVALID, 0, _("division by zero"));
          ((fxn == multiply ? mpz_mul
            : fxn == divide ? mpz_tdiv_q
            : mpz_tdiv_r)
           (l->u.i, l->u.i, r->u.i));
        }
      freev (r);
    }
}

/* Handle +, - operators.  */

static VALUE *
eval3 (bool evaluate)
{
  VALUE *l;
  VALUE *r;
  enum { plus, minus } fxn;

#ifdef EVAL_TRACE
  trace ("eval3");
#endif
  l = eval4 (evaluate);
  while (true)
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
            error (EXPR_INVALID, 0, _("non-integer argument"));
          (fxn == plus ? mpz_add : mpz_sub) (l->u.i, l->u.i, r->u.i);
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
  while (true)
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
            default: unreachable ();
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
  while (true)
    {
      if (nextarg ("&"))
        {
          r = eval2 (evaluate && !null (l));
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
  while (true)
    {
      if (nextarg ("|"))
        {
          r = eval1 (evaluate && null (l));
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
