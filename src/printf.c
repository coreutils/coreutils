/* printf - format and print data
   Copyright (C) 1990-1998, Free Software Foundation, Inc.

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

/* Usage: printf format [argument...]

   A front end to the printf function that lets it be used from the shell.

   Backslash escapes:

   \" = double quote
   \\ = backslash
   \a = alert (bell)
   \b = backspace
   \c = produce no further output
   \f = form feed
   \n = new line
   \r = carriage return
   \t = horizontal tab
   \v = vertical tab
   \0ooo = octal number (ooo is 0 to 3 digits)
   \xhhh = hexadecimal number (hhh is 1 to 3 digits)

   Additional directive:

   %b = print an argument string, interpreting backslash escapes

   The `format' argument is re-used as many times as necessary
   to convert all of the given arguments.

   David MacKenzie <djm@gnu.ai.mit.edu> */

#include <config.h>
#include <stdio.h>
#include <sys/types.h>
#include <getopt.h>

#include "system.h"
#include "long-options.h"
#include "error.h"

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "printf"

#define AUTHORS "David MacKenzie"

#ifndef STDC_HEADERS
double strtod ();
long int strtol ();
unsigned long int strtoul ();
#endif

#define isodigit(c) ((c) >= '0' && (c) <= '7')
#define hextobin(c) ((c) >= 'a' && (c) <= 'f' ? (c) - 'a' + 10 : \
		     (c) >= 'A' && (c) <= 'F' ? (c) - 'A' + 10 : (c) - '0')
#define octtobin(c) ((c) - '0')

/* The value to return to the calling program.  */
static int exit_status;

/* Non-zero if the POSIXLY_CORRECT environment variable is set.  */
static int posixly_correct;

/* This message appears in N_() here rather than just in _() below because
   the sole use would have been in a #define, and xgettext doesn't look for
   strings in cpp directives.  */
static char *const cfcc_msg =
 N_("warning: %s: character(s) following character constant have been ignored");

/* The name this program was run with. */
char *program_name;

void
usage (int status)
{
  if (status != 0)
    fprintf (stderr, _("Try `%s --help' for more information.\n"),
	     program_name);
  else
    {
      printf (_("\
Usage: %s FORMAT [ARGUMENT]...\n\
  or:  %s OPTION\n\
"),
	      program_name, program_name);
      printf (_("\
Print ARGUMENT(s) according to FORMAT.\n\
\n\
  --help      display this help and exit\n\
  --version   output version information and exit\n\
\n\
FORMAT controls the output as in C printf.  Interpreted sequences are:\n\
\n\
  \\\"      double quote\n\
  \\0NNN   character with octal value NNN (0 to 3 digits)\n\
  \\\\      backslash\n\
  \\a      alert (BEL)\n\
  \\b      backspace\n\
  \\c      produce no further output\n\
  \\f      form feed\n\
  \\n      new line\n\
  \\r      carriage return\n\
  \\t      horizontal tab\n\
  \\v      vertical tab\n\
  \\xNNN   character with hexadecimal value NNN (1 to 3 digits)\n\
\n\
  %%%%      a single %%\n\
  %%b      ARGUMENT as a string with `\\' escapes interpreted\n\
\n\
and all C format specifications ending with one of diouxXfeEgGcs, with\n\
ARGUMENTs converted to proper type first.  Variable widths are handled.\n\
"));
      puts (_("\nReport bugs to <bug-sh-utils@gnu.org>."));
    }
  exit (status);
}

static void
verify (const char *s, const char *end)
{
  if (errno)
    {
      error (0, errno, "%s", s);
      exit_status = 1;
    }
  else if (*end)
    {
      if (s == end)
	error (0, 0, _("%s: expected a numeric value"), s);
      else
	error (0, 0, _("%s: value not completely converted"), s);
      exit_status = 1;
    }
}

#define STRTOX(TYPE, FUNC_NAME, LIB_FUNC_EXPR)				 \
static TYPE								 \
FUNC_NAME (s)								 \
     const char *s;							 \
{									 \
  char *end;								 \
  TYPE val;								 \
									 \
  if (*s == '\"' || *s == '\'')						 \
    {									 \
      val = *(unsigned char *) ++s;					 \
      /* If POSIXLY_CORRECT is not set, then give a warning that there	 \
	 are characters following the character constant and that GNU	 \
	 printf is ignoring those characters.  If POSIXLY_CORRECT *is*	 \
	 set, then don't give the warning.  */				 \
      if (*++s != 0 && !posixly_correct)				 \
	error (0, 0, _(cfcc_msg), s);					 \
    }									 \
  else									 \
    {									 \
      errno = 0;							 \
      val = LIB_FUNC_EXPR;						 \
      verify (s, end);							 \
    }									 \
  return val;								 \
}									 \

STRTOX (unsigned long int, xstrtoul, (strtoul (s, &end, 0)))
STRTOX (long int,          xstrtol,  (strtol  (s, &end, 0)))
STRTOX (double,            xstrtod,  (strtod  (s, &end)))

/* Output a single-character \ escape.  */

static void
print_esc_char (int c)
{
  switch (c)
    {
    case 'a':			/* Alert. */
      putchar (7);
      break;
    case 'b':			/* Backspace. */
      putchar (8);
      break;
    case 'c':			/* Cancel the rest of the output. */
      exit (0);
      break;
    case 'f':			/* Form feed. */
      putchar (12);
      break;
    case 'n':			/* New line. */
      putchar (10);
      break;
    case 'r':			/* Carriage return. */
      putchar (13);
      break;
    case 't':			/* Horizontal tab. */
      putchar (9);
      break;
    case 'v':			/* Vertical tab. */
      putchar (11);
      break;
    default:
      putchar (c);
      break;
    }
}

/* Print a \ escape sequence starting at ESCSTART.
   Return the number of characters in the escape sequence
   besides the backslash. */

static int
print_esc (const char *escstart)
{
  register const char *p = escstart + 1;
  int esc_value = 0;		/* Value of \nnn escape. */
  int esc_length;		/* Length of \nnn escape. */

  /* \0ooo and \xhhh escapes have maximum length of 3 chars. */
  if (*p == 'x')
    {
      for (esc_length = 0, ++p;
	   esc_length < 3 && ISXDIGIT (*p);
	   ++esc_length, ++p)
	esc_value = esc_value * 16 + hextobin (*p);
      if (esc_length == 0)
	error (1, 0, _("missing hexadecimal number in escape"));
      putchar (esc_value);
    }
  else if (*p == '0')
    {
      for (esc_length = 0, ++p;
	   esc_length < 3 && isodigit (*p);
	   ++esc_length, ++p)
	esc_value = esc_value * 8 + octtobin (*p);
      putchar (esc_value);
    }
  else if (strchr ("\"\\abcfnrtv", *p))
    print_esc_char (*p++);
  else
    error (1, 0, _("\\%c: invalid escape"), *p);
  return p - escstart - 1;
}

/* Print string STR, evaluating \ escapes. */

static void
print_esc_string (const char *str)
{
  for (; *str; str++)
    if (*str == '\\')
      str += print_esc (str);
    else
      putchar (*str);
}

/* Output a % directive.  START is the start of the directive,
   LENGTH is its length, and ARGUMENT is its argument.
   If FIELD_WIDTH or PRECISION is non-negative, they are args for
   '*' values in those fields. */

static void
print_direc (const char *start, size_t length, int field_width,
	     int precision, const char *argument)
{
  char *p;		/* Null-terminated copy of % directive. */

  p = xmalloc ((unsigned) (length + 1));
  strncpy (p, start, length);
  p[length] = 0;

  switch (p[length - 1])
    {
    case 'd':
    case 'i':
      if (field_width < 0)
	{
	  if (precision < 0)
	    printf (p, xstrtol (argument));
	  else
	    printf (p, precision, xstrtol (argument));
	}
      else
	{
	  if (precision < 0)
	    printf (p, field_width, xstrtol (argument));
	  else
	    printf (p, field_width, precision, xstrtol (argument));
	}
      break;

    case 'o':
    case 'u':
    case 'x':
    case 'X':
      if (field_width < 0)
	{
	  if (precision < 0)
	    printf (p, xstrtoul (argument));
	  else
	    printf (p, precision, xstrtoul (argument));
	}
      else
	{
	  if (precision < 0)
	    printf (p, field_width, xstrtoul (argument));
	  else
	    printf (p, field_width, precision, xstrtoul (argument));
	}
      break;

    case 'f':
    case 'e':
    case 'E':
    case 'g':
    case 'G':
      if (field_width < 0)
	{
	  if (precision < 0)
	    printf (p, xstrtod (argument));
	  else
	    printf (p, precision, xstrtod (argument));
	}
      else
	{
	  if (precision < 0)
	    printf (p, field_width, xstrtod (argument));
	  else
	    printf (p, field_width, precision, xstrtod (argument));
	}
      break;

    case 'c':
      printf (p, *argument);
      break;

    case 's':
      if (field_width < 0)
	{
	  if (precision < 0)
	    printf (p, argument);
	  else
	    printf (p, precision, argument);
	}
      else
	{
	  if (precision < 0)
	    printf (p, field_width, argument);
	  else
	    printf (p, field_width, precision, argument);
	}
      break;
    }

  free (p);
}

/* Print the text in FORMAT, using ARGV (with ARGC elements) for
   arguments to any `%' directives.
   Return the number of elements of ARGV used.  */

static int
print_formatted (const char *format, int argc, char **argv)
{
  int save_argc = argc;		/* Preserve original value.  */
  const char *f;		/* Pointer into `format'.  */
  const char *direc_start;	/* Start of % directive.  */
  size_t direc_length;		/* Length of % directive.  */
  int field_width;		/* Arg to first '*', or -1 if none.  */
  int precision;		/* Arg to second '*', or -1 if none.  */

  for (f = format; *f; ++f)
    {
      switch (*f)
	{
	case '%':
	  direc_start = f++;
	  direc_length = 1;
	  field_width = precision = -1;
	  if (*f == '%')
	    {
	      putchar ('%');
	      break;
	    }
	  if (*f == 'b')
	    {
	      if (argc > 0)
		{
		  print_esc_string (*argv);
		  ++argv;
		  --argc;
		}
	      break;
	    }
	  if (strchr ("-+ #", *f))
	    {
	      ++f;
	      ++direc_length;
	    }
	  if (*f == '*')
	    {
	      ++f;
	      ++direc_length;
	      if (argc > 0)
		{
		  field_width = xstrtoul (*argv);
		  ++argv;
		  --argc;
		}
	      else
		field_width = 0;
	    }
	  else
	    while (ISDIGIT (*f))
	      {
		++f;
		++direc_length;
	      }
	  if (*f == '.')
	    {
	      ++f;
	      ++direc_length;
	      if (*f == '*')
		{
		  ++f;
		  ++direc_length;
		  if (argc > 0)
		    {
		      precision = xstrtoul (*argv);
		      ++argv;
		      --argc;
		    }
		  else
		    precision = 0;
		}
	      else
		while (ISDIGIT (*f))
		  {
		    ++f;
		    ++direc_length;
		  }
	    }
	  if (*f == 'l' || *f == 'L' || *f == 'h')
	    {
	      ++f;
	      ++direc_length;
	    }
	  if (!strchr ("diouxXfeEgGcs", *f))
	    error (1, 0, _("%%%c: invalid directive"), *f);
	  ++direc_length;
	  if (argc > 0)
	    {
	      print_direc (direc_start, direc_length, field_width,
			   precision, *argv);
	      ++argv;
	      --argc;
	    }
	  else
	    print_direc (direc_start, direc_length, field_width,
			 precision, "");
	  break;

	case '\\':
	  f += print_esc (f);
	  break;

	default:
	  putchar (*f);
	}
    }

  return save_argc - argc;
}

int
main (int argc, char **argv)
{
  char *format;
  int args_used;

  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  exit_status = 0;

  /* Don't recognize --help or --version if POSIXLY_CORRECT is set.  */
  posixly_correct = (getenv ("POSIXLY_CORRECT") != NULL);
  if (!posixly_correct)
    parse_long_options (argc, argv, PROGRAM_NAME, GNU_PACKAGE, VERSION,
			AUTHORS, usage);

  if (argc == 1)
    {
      fprintf (stderr, _("Usage: %s format [argument...]\n"), program_name);
      exit (1);
    }

  format = argv[1];
  argc -= 2;
  argv += 2;

  do
    {
      args_used = print_formatted (format, argc, argv);
      argc -= args_used;
      argv += args_used;
    }
  while (args_used > 0 && argc > 0);

  if (argc > 0)
    error (0, 0, _("warning: excess arguments have been ignored"));

  exit (exit_status);
}
