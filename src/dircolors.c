/* dircolors - output commands to set the LS_COLOR environment variable
   Copyright (C) 1994, 1995, 1997, 1998, 1999, 2000, 2001, 2002 H. Peter Anvin
   Copyright (C) 1996-2000 Free Software Foundation, Inc.

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

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <sys/types.h>
#include <getopt.h>
#include <stdio.h>

#include "system.h"
#include "dircolors.h"
#include "dirname.h"
#include "error.h"
#include "getline.h"
#include "obstack.h"
#include "quote.h"

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "dircolors"

#define AUTHORS "H. Peter Anvin"

#define obstack_chunk_alloc malloc
#define obstack_chunk_free free

enum Shell_syntax
{
  SHELL_SYNTAX_BOURNE,
  SHELL_SYNTAX_C,
  SHELL_SYNTAX_UNKNOWN
};

#define APPEND_CHAR(C) obstack_1grow (&lsc_obstack, C)
#define APPEND_TWO_CHAR_STRING(S)					\
  do									\
    {									\
      APPEND_CHAR (S[0]);						\
      APPEND_CHAR (S[1]);						\
    }									\
  while (0)

/* Accumulate in this obstack the value for the LS_COLORS environment
   variable.  */
static struct obstack lsc_obstack;

/* Nonzero if the input file was the standard input. */
static int have_read_stdin;

/* FIXME: associate with ls_codes? */
static const char *const slack_codes[] =
{
  "NORMAL", "NORM", "FILE", "DIR", "LNK", "LINK",
  "SYMLINK", "ORPHAN", "MISSING", "FIFO", "PIPE", "SOCK", "BLK", "BLOCK",
  "CHR", "CHAR", "DOOR", "EXEC", "LEFT", "LEFTCODE", "RIGHT", "RIGHTCODE",
  "END", "ENDCODE", NULL
};

static const char *const ls_codes[] =
{
  "no", "no", "fi", "di", "ln", "ln", "ln", "or", "mi", "pi", "pi",
  "so", "bd", "bd", "cd", "cd", "do", "ex", "lc", "lc", "rc", "rc", "ec", "ec"
};

static struct option const long_options[] =
  {
    {"bourne-shell", no_argument, NULL, 'b'},
    {"sh", no_argument, NULL, 'b'},
    {"csh", no_argument, NULL, 'c'},
    {"c-shell", no_argument, NULL, 'c'},
    {"print-database", no_argument, NULL, 'p'},
    {GETOPT_HELP_OPTION_DECL},
    {GETOPT_VERSION_OPTION_DECL},
    {NULL, 0, NULL, 0}
  };

char *program_name;

void
usage (int status)
{
  if (status != 0)
    fprintf (stderr, _("Try `%s --help' for more information.\n"),
             program_name);
  else
    {
      printf (_("Usage: %s [OPTION]... [FILE]\n"), program_name);
      fputs (_("\
Output commands to set the LS_COLORS environment variable.\n\
\n\
Determine format of output:\n\
  -b, --sh, --bourne-shell    output Bourne shell code to set LS_COLORS\n\
  -c, --csh, --c-shell        output C shell code to set LS_COLORS\n\
  -p, --print-database        output defaults\n\
"), stdout);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      fputs (_("\
\n\
If FILE is specified, read it to determine which colors to use for which\n\
file types and extensions.  Otherwise, a precompiled database is used.\n\
For details on the format of these files, run `dircolors --print-database'.\n\
"), stdout);
      printf (_("\nReport bugs to <%s>.\n"), PACKAGE_BUGREPORT);
    }

  exit (status == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}

static void *
xstrndup (const char *s, size_t n)
{
  char *new = strndup (s, n);
  if (new == NULL)
    xalloc_die ();
  return new;
}

/* If the SHELL environment variable is set to `csh' or `tcsh,'
   assume C shell.  Else Bourne shell.  */

static enum Shell_syntax
guess_shell_syntax (void)
{
  char *shell;

  shell = getenv ("SHELL");
  if (shell == NULL || *shell == '\0')
    return SHELL_SYNTAX_UNKNOWN;

  shell = base_name (shell);

  if (STREQ (shell, "csh") || STREQ (shell, "tcsh"))
    return SHELL_SYNTAX_C;

  return SHELL_SYNTAX_BOURNE;
}

static void
parse_line (unsigned char const *line, char **keyword, char **arg)
{
  unsigned char const *p;
  unsigned char const *keyword_start;
  unsigned char const *arg_start;

  *keyword = NULL;
  *arg = NULL;

  for (p = line; ISSPACE (*p); ++p)
    ;

  /* Ignore blank lines and shell-style comments.  */
  if (*p == '\0' || *p == '#')
    return;

  keyword_start = p;

  while (!ISSPACE (*p) && *p != '\0')
    {
      ++p;
    }

  *keyword = xstrndup ((const char *) keyword_start, p - keyword_start);
  if (*p  == '\0')
    return;

  do
    {
      ++p;
    }
  while (ISSPACE (*p));

  if (*p == '\0' || *p == '#')
    return;

  arg_start = p;

  while (*p != '\0' && *p != '#')
    ++p;

  for (--p; ISSPACE (*p); --p)
    {
      /* empty */
    }
  ++p;

  *arg = xstrndup ((const char *) arg_start, p - arg_start);
}

/* FIXME: Write a string to standard out, while watching for "dangerous"
   sequences like unescaped : and = characters.  */

static void
append_quoted (const char *str)
{
  int need_backslash = 1;

  while (*str != '\0')
    {
      switch (*str)
	{
	case '\\':
	case '^':
	  need_backslash = !need_backslash;
	  break;

	case ':':
	case '=':
	  if (need_backslash)
	    APPEND_CHAR ('\\');
	  /* Fall through */

	default:
	  need_backslash = 1;
	  break;
	}

      APPEND_CHAR (*str);
      ++str;
    }
}

/* Read the file open on FP (with name FILENAME).  First, look for a
   `TERM name' directive where name matches the current terminal type.
   Once found, translate and accumulate the associated directives onto
   the global obstack LSC_OBSTACK.  Give a diagnostic and return nonzero
   upon failure (unrecognized keyword is the only way to fail here).
   Return zero otherwise.  */

static int
dc_parse_stream (FILE *fp, const char *filename)
{
  size_t line_number = 0;
  char *line = NULL;
  size_t line_chars_allocated = 0;
  int state;
  char *term;
  int err = 0;

  /* State for the parser.  */
  enum states { ST_TERMNO, ST_TERMYES, ST_TERMSURE, ST_GLOBAL };

  state = ST_GLOBAL;

  /* Get terminal type */
  term = getenv ("TERM");
  if (term == NULL || *term == '\0')
    term = "none";

  while (1)
    {
      int line_length;
      char *keywd, *arg;
      int unrecognized;

      ++line_number;

      if (fp)
	{
	  line_length = getline (&line, &line_chars_allocated, fp);
	  if (line_length <= 0)
	    {
	      if (line)
		free (line);
	      break;
	    }
	}
      else
	{
	  line = (char *) (G_line[line_number - 1]);
	  line_length = G_line_length[line_number - 1];
	  if (line_number > G_N_LINES)
	    break;
	}

      parse_line ((unsigned char *) line, &keywd, &arg);

      if (keywd == NULL)
	continue;

      if (arg == NULL)
	{
	  error (0, 0, _("%s:%lu: invalid line;  missing second token"),
		 filename, (long unsigned) line_number);
	  err = 1;
	  free (keywd);
	  continue;
	}

      unrecognized = 0;
      if (strcasecmp (keywd, "TERM") == 0)
	{
	  if (STREQ (arg, term))
	    state = ST_TERMSURE;
	  else if (state != ST_TERMSURE)
	    state = ST_TERMNO;
	}
      else
	{
	  if (state == ST_TERMSURE)
	    state = ST_TERMYES; /* Another TERM can cancel */

	  if (state != ST_TERMNO)
	    {
	      if (keywd[0] == '.')
		{
		  APPEND_CHAR ('*');
		  append_quoted (keywd);
		  APPEND_CHAR ('=');
		  append_quoted (arg);
		  APPEND_CHAR (':');
		}
	      else if (keywd[0] == '*')
		{
		  append_quoted (keywd);
		  APPEND_CHAR ('=');
		  append_quoted (arg);
		  APPEND_CHAR (':');
		}
	      else if (strcasecmp (keywd, "OPTIONS") == 0
		       || strcasecmp (keywd, "COLOR") == 0
		       || strcasecmp (keywd, "EIGHTBIT") == 0)
		{
		  /* Ignore.  */
		}
	      else
		{
		  int i;

		  for (i = 0; slack_codes[i] != NULL; ++i)
		    if (strcasecmp (keywd, slack_codes[i]) == 0)
		      break;

		  if (slack_codes[i] != NULL)
		    {
		      APPEND_TWO_CHAR_STRING (ls_codes[i]);
		      APPEND_CHAR ('=');
		      append_quoted (arg);
		      APPEND_CHAR (':');
		    }
		  else
		    {
		      unrecognized = 1;
		    }
		}
	    }
	  else
	    {
	      unrecognized = 1;
	    }
	}

      if (unrecognized && (state == ST_TERMSURE || state == ST_TERMYES))
	{
	  error (0, 0, _("%s:%lu: unrecognized keyword %s"),
		 (filename ? quote (filename) : _("<internal>")),
		 (long unsigned) line_number, keywd);
	  err = 1;
	}

      free (keywd);
      if (arg)
	free (arg);
    }

  return err;
}

static int
dc_parse_file (const char *filename)
{
  FILE *fp;
  int err;

  if (STREQ (filename, "-"))
    {
      have_read_stdin = 1;
      fp = stdin;
    }
  else
    {
      /* OPENOPTS is a macro.  It varies with the system.
	 Some systems distinguish between internal and
	 external text representations.  */

      fp = fopen (filename, "r");
      if (fp == NULL)
	{
	  error (0, errno, "%s", quote (filename));
	  return 1;
	}
    }

  err = dc_parse_stream (fp, filename);

  if (fp != stdin && fclose (fp) == EOF)
    {
      error (0, errno, "%s", quote (filename));
      return 1;
    }

  return err;
}

int
main (int argc, char **argv)
{
  int err = 0;
  int optc;
  enum Shell_syntax syntax = SHELL_SYNTAX_UNKNOWN;
  int print_database = 0;

  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  while ((optc = getopt_long (argc, argv, "bcp", long_options, NULL)) != -1)
    switch (optc)
      {
      case 'b':	/* Bourne shell syntax.  */
	syntax = SHELL_SYNTAX_BOURNE;
	break;

      case 'c':	/* C shell syntax.  */
	syntax = SHELL_SYNTAX_C;
	break;

      case 'p':
	print_database = 1;
	break;

      case_GETOPT_HELP_CHAR;

      case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);

      default:
	usage (EXIT_FAILURE);
      }

  argc -= optind;
  argv += optind;

  /* It doesn't make sense to use --print with either of
     --bourne or --c-shell.  */
  if (print_database && syntax != SHELL_SYNTAX_UNKNOWN)
    {
      error (0, 0,
	     _("the options to output dircolors' internal database and\n\
to select a shell syntax are mutually exclusive"));
      usage (EXIT_FAILURE);
    }

  if (print_database && argc > 0)
    {
      error (0, 0,
	     _("no FILE arguments may be used with the option to output\n\
dircolors' internal database"));
      usage (EXIT_FAILURE);
    }

  if (!print_database && argc > 1)
    {
      error (0, 0, _("too many arguments"));
      usage (EXIT_FAILURE);
    }

  if (print_database)
    {
      int i;
      for (i = 0; i < G_N_LINES; i++)
	{
	  fwrite (G_line[i], 1, G_line_length[i], stdout);
	  fputc ('\n', stdout);
	}
    }
  else
    {
      /* If shell syntax was not explicitly specified, try to guess it. */
      if (syntax == SHELL_SYNTAX_UNKNOWN)
	{
	  syntax = guess_shell_syntax ();
	  if (syntax == SHELL_SYNTAX_UNKNOWN)
	    {
	      error (EXIT_FAILURE, 0,
	 _("no SHELL environment variable, and no shell type option given"));
	    }
	}

      obstack_init (&lsc_obstack);
      if (argc == 0)
	err = dc_parse_stream (NULL, NULL);
      else
	err = dc_parse_file (argv[0]);

      if (!err)
	{
	  size_t len = obstack_object_size (&lsc_obstack);
	  char *s = obstack_finish (&lsc_obstack);
	  const char *prefix;
	  const char *suffix;

	  if (syntax == SHELL_SYNTAX_BOURNE)
	    {
	      prefix = "LS_COLORS='";
	      suffix = "';\nexport LS_COLORS\n";
	    }
	  else
	    {
	      prefix = "setenv LS_COLORS '";
	      suffix = "'\n";
	    }
	  fputs (prefix, stdout);
	  fwrite (s, 1, len, stdout);
	  fputs (suffix, stdout);
	}
    }


  if (have_read_stdin && fclose (stdin) == EOF)
    error (EXIT_FAILURE, errno, _("standard input"));

  exit (err == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}
