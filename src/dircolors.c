/* FIXME: accept, but ignore EIGHTBIT option
   FIXME: add option to print that default mapping?

   dircolors: use compiled-in defaults, output a string setting LS_COLORS
   dircolors -p: output the compiled-in defaults
   dircolors FILE: use FILE, output a string setting LS_COLORS

   Only distinction necessary: Bourne vs. C-shell
   Use obstack to accumulate rhs of setenv
 */
/* dircolors - parse a Slackware-style DIR_COLORS file.
   Copyright (C) 1994, 1995 H. Peter Anvin
   Copyright (C) 1996 Free Software Foundation, Inc.

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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <ctype.h>
#include <getopt.h>
#include <stdio.h>

#include "system.h"
#include "getline.h"
#include "long-options.h"
#include "error.h"
#include "dircolors.h"

char *xmalloc ();
char *basename ();

/* Nonzero if any of the files read were the standard input. */
static int have_read_stdin;

enum Shell_syntax
{
  SHELL_SYNTAX_BOURNE,
  SHELL_SYNTAX_C,
  SHELL_SYNTAX_UNKNOWN
};

/* Parser needs these state variables.  */
enum states { ST_TERMNO, ST_TERMYES, ST_TERMSURE, ST_GLOBAL };

/* FIXME: associate with ls_codes? */
static const char *const slack_codes[] =
{
  "NORMAL", "NORM", "FILE", "DIR", "LNK", "LINK",
  "SYMLINK", "ORPHAN", "MISSING", "FIFO", "PIPE", "SOCK", "BLK", "BLOCK",
  "CHR", "CHAR", "EXEC", "LEFT", "LEFTCODE", "RIGHT", "RIGHTCODE", "END",
  "ENDCODE", NULL
};

static const char *const ls_codes[] =
{
  "no", "no", "fi", "di", "ln", "ln", "ln", "or", "mi", "pi", "pi",
  "so", "bd", "bd", "cd", "cd", "ex", "lc", "lc", "rc", "rc", "ec", "ec"
};

static struct option const long_options[] =
  {
    {"bourne-shell", no_argument, NULL, 'b'},
    {"sh", no_argument, NULL, 'b'},
    {"csh", no_argument, NULL, 'c'},
    {"c-shell", no_argument, NULL, 'c'},
    {"help", no_argument, NULL, 'h'},
    {"version", no_argument, NULL, 'v'},
  };

char *program_name;

static void
usage (int status)
{
  if (status != 0)
    fprintf (stderr, _("Try `%s --help' for more information.\n"),
             program_name);
  else
    {
      printf (_("Usage: %s [OPTION]... [FILE]\n"), program_name);
      printf (_("\
  -h, --help                  display this help and exit\n\
      --version               output version information and exit\n\
Determine format of output:\n\
  -p, --print                 output defaults\n\
  -b, --sh, --bourne-shell    output Bourne shell code to set LS_COLOR\n\
  -c, --csh, --c-shell        output C-shell code to set LS_COLOR\n"));
    }

  exit (status);
}

/* If the SHELL environment variable is set to `csh' or `tcsh,'
   assume C-shell.  Else Bourne shell.  */

static enum Shell_syntax
guess_shell_syntax (void)
{
  char *shell;

  shell = getenv ("SHELL");
  if (shell == NULL || *shell == '\0')
    return SHELL_SYNTAX_UNKNOWN;

  shell = basename (shell);

  if (STREQ (shell, "csh") || STREQ (shell, "tcsh"))
    return SHELL_SYNTAX_C;

  return SHELL_SYNTAX_BOURNE;
}

static void
parse_line (char **keyword, char **arg, char *line)
{
  char *p;

  *keyword = *arg = "";

  for (p = line; isspace (*p); ++p)
    ;

  if (*p == '\0' || *p == '#')
    return;

  *keyword = p;

  while (!isspace (*p))
    if (*p++ == '\0')
      return;

  *p++ = '\0';

  while (isspace (*p))
    ++p;

  if (*p == '\0' || *p == '#')
    return;

  *arg = p;

  while (*p != '\0' && *p != '#')
    ++p;
  for (--p; isspace (*p); --p)
    ;
  ++p;

  *p = '\0';
}

/* Write a string to standard out, while watching for "dangerous"
   sequences like unescaped : and = characters.  */

static void
put_seq (const char *str, char follow)
{
  int danger = 1;

  while (*str != '\0')
    {
      switch (*str)
	{
	case '\\':
	case '^':
	  danger = !danger;
	  break;

	case ':':
	case '=':
	  if (danger)
	    putchar ('\\');
	  /* Fall through */

	default:
	  danger = 1;
	  break;
	}

      putchar (*str++);
    }

  putchar (follow);		/* The character that ends the sequence.  */
}

/* FIXME: Accumulate settings in obstack and convert to string in *RESULT.  */

static int
dc_parse_stream (FILE *fp, const char *filename, char **result)
{
  size_t line_number = 0;
  char *line = NULL;
  size_t line_chars_allocated = 0;
  int state;
  char *term;
  int err = 0;

  state = ST_GLOBAL;

  /* Get terminal type */
  term = getenv ("TERM");
  if (term == NULL || *term == '\0')
    term = "none";

  while (1)
    {
      int line_length;
      char *keywd, *arg;

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

      parse_line (&keywd, &arg, line);
      if (*keywd != '\0')
	{
	  if (strcasecmp (keywd, "TERM") == 0)
	    {
	      if (strcmp (arg, term) == 0)
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
		      putchar ('*');
		      put_seq (keywd, '=');
		      put_seq (arg, ':');
		    }
		  else if (keywd[0] == '*')
		    {
		      put_seq (keywd, '=');
		      put_seq (arg, ':');
		    }
		  else if (strcasecmp (keywd, "OPTIONS") == 0)
		    {
		      /* Ignore.  */
		    }
		  else if (strcasecmp (keywd, "COLOR") == 0)
		    {
		       /* FIXME: ignored now.  */
		    }
		  else
		    {
		      int i;

		      for (i = 0; slack_codes[i] != NULL; ++i)
			if (strcasecmp (keywd, slack_codes[i]) == 0)
			  break;

		      if (slack_codes[i] != NULL)
			{
			  printf ("%s=", ls_codes[i]);
			  put_seq (arg, ':');
			}
		      else
			{
			  error (0, 0, _("%s:%lu: unrecognized keyword %s\n"),
				 filename, (long unsigned) line_number, keywd);
			  err = 1;
			}
		    }
		}
	    }
	}
    }

  return err;
}

static int
dc_parse_file (const char *filename, char **ls_color_string)
{
  FILE *fp;
  int err;

  if (strcmp (filename, "-") == 0)
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
	  error (0, errno, "%s", filename);
	  return 1;
	}
    }

  err = dc_parse_stream (fp, filename, ls_color_string);
  if (err)
    {
      error (0, errno, "%s", filename);
      if (fp != stdin)
	fclose (fp);
      return 1;
    }

  if (fp != stdin && fclose (fp) == EOF)
    {
      error (0, errno, "%s", filename);
      return 1;
    }

  return 0;
}

int
main (int argc, char *argv[])
{
  int err;
  int optc;
  enum Shell_syntax syntax = SHELL_SYNTAX_UNKNOWN;
  char *ls_color_string;
  char *file;
  int print_defaults = 0;

  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  parse_long_options (argc, argv, "dircolors", PACKAGE_VERSION, usage);

  while ((optc = getopt_long (argc, argv, "bcp", long_options, NULL))
	 != EOF)
    switch (optc)
      {
      case 'b':	/* Bourne shell syntax.  */
	syntax = SHELL_SYNTAX_BOURNE;
	break;

      case 'c':	/* C-shell syntax.  */
	syntax = SHELL_SYNTAX_C;
	break;

      case 'p':
	print_defaults = 1;
	break;

      default:
	usage (1);
      }

  /* FIXME: don't allow -b with -c
     or -p with anything else.
     No file args allowed with -p.
     No more than one allowed in any case.
     */

  if (print_defaults)
    {
      int i;
      for (i = 0; i < G_N_LINES; i++)
	{
	  fwrite (G_line[i], 1, G_line_length[i], stdout);
	  fputc ('\n', stdout);
	}
    }

  /* Use shell to determine mode, if not already done. */
  if (syntax == SHELL_SYNTAX_UNKNOWN)
    {
      syntax = guess_shell_syntax ();
      if (syntax == SHELL_SYNTAX_UNKNOWN)
	{
	  error (EXIT_FAILURE, 0,
	   _("no SHELL environment variable, and no shell type option given"));
	}
    }

  file = argv[optind];
  err = dc_parse_file (file, &ls_color_string);

  if (fclose (stdout) == EOF)
    error (EXIT_FAILURE, errno, _("write error"));

  if (have_read_stdin && fclose (stdin) == EOF)
    error (EXIT_FAILURE, errno, _("standard input"));

  exit (err == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}
