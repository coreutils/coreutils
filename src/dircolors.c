/* FIXME: accept, but ignore EIGHTBIT option
   FIXME: embed contents of default mapping file
   FIXME: add option to print that default mapping?

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
#include "long-options.h"
#include "error.h"

char *xmalloc ();

#define USER_FILE ".dir_colors"	/* Versus user's home directory */
#define SYSTEM_FILE "DIR_COLORS" /* System-wide file in directory SYSTEM_DIR
				     (defined on the cc command line).  */

#define STRINGLEN 2048		/* Max length of a string */

enum modes { SHELLTYPE_BOURNE, SHELLTYPE_C, SHELLTYPE_UNKNOWN };

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
    {"sh", no_argument, NULL, 'b'},
    {"csh", no_argument, NULL, 'c'},
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
  -h, --help        display this help and exit\n\
      --version     output version information and exit\n\
Determine format of output:\n\
  -b, --sh          assume Bourne shell\n\
  -c, --csh         assume C-shell\n"));
    }

  exit (status);
}

/* If SHELL is csh or tcsh, assume C-shell.  Else Bourne shell.  */

static int
figure_mode (void)
{
  char *shell, *shellv;

  shellv = getenv ("SHELL");
  if (shellv == NULL || *shellv == '\0')
    return SHELLTYPE_UNKNOWN;

  shell = strrchr (shellv, '/');
  if (shell != NULL)
    ++shell;
  else
    shell = shellv;

  if (strcmp (shell, "csh") == 0
      || strcmp (shell, "tcsh") == 0)
    return SHELLTYPE_C;

  return SHELLTYPE_BOURNE;
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

int
main (int argc, char *argv[])
{
  char *p;
  int optc;
  int mode = SHELLTYPE_UNKNOWN;
  FILE *fp = NULL;
  char *term;
  int state;

  char line[STRINGLEN];
  char *keywd, *arg;

  char *input_file;

  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  parse_long_options (argc, argv, "dircolors", PACKAGE_VERSION, usage);

  while ((optc = getopt_long (argc, argv, "bc", long_options, NULL))
	 != EOF)
    switch (optc)
      {
      case 'b':	/* Bourne shell mode.  */
	mode = SHELLTYPE_BOURNE;
	break;

      case 'c':	/* Bourne shell mode.  */
	mode = SHELLTYPE_C;
	break;

      default:
	usage (1);
      }

  /* Use shell to determine mode, if not already done. */
  if (mode == SHELLTYPE_UNKNOWN)
    {
      mode = figure_mode ();
      if (mode == SHELLTYPE_UNKNOWN)
	{
	  error (1, 0, _("\
no SHELL environment variable, and no shell type option given"));
	}
    }

  /* Open dir_colors file */
  if (optind == argc)
    {
      p = getenv ("HOME");
      if (p != NULL && *p != '\0')
	{
	  /* Note: deliberate leak.  It's not worth freeing this.  */
	  input_file = xmalloc (strlen (p) + 1 + strlen (USER_FILE) + 1);
	  stpcpy (stpcpy (stpcpy (input_file, p), "/"), USER_FILE);
	  fp = fopen (input_file, "r");
	}

      if (fp == NULL)
	{
	  /* Note: deliberate leak.  It's not worth freeing this.  */
	  input_file = xmalloc (strlen (SHAREDIR) + 1
				+ strlen (USER_FILE) + 1);
	  stpcpy (stpcpy (stpcpy (input_file, SHAREDIR), "/"),
		  SYSTEM_FILE);
	  fp = fopen (input_file, "r");
	}
    }
  else
    {
      input_file = argv[optind];
      fp = fopen (input_file, "r");
    }

  if (fp == NULL)
    error (1, errno, _("while opening input file `%s'"), input_file);

  /* Get terminal type */
  term = getenv ("TERM");
  if (term == NULL || *term == '\0')
    term = "none";

  /* Write out common start */
  switch (mode)
    {
    case SHELLTYPE_C:
      puts ("set noglob;\nsetenv LS_COLORS \':");
      break;

    case SHELLTYPE_BOURNE:
      fputs ("LS_COLORS=\'", stdout);
      break;
    }

  state = ST_GLOBAL;

  /* FIXME: use getline */
  while (fgets (line, STRINGLEN, fp) != NULL)
    {
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
			error (0, 0, _("Unknown keyword %s\n"), keywd);
		    }
		}
	    }
	}
    }

  fclose (fp);

  /* Write it out.  */
  switch (mode)
    {
    case SHELLTYPE_BOURNE:
      printf ("\';\nexport LS_COLORS\n");
      break;

    case SHELLTYPE_C:
      printf ("\'\nunset noglob\n");
      break;

    default:
      abort ();
    }

  exit (0);
}
