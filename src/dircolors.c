/* dircolors - output commands to set the LS_COLOR environment variable
   Copyright (C) 1996-2025 Free Software Foundation, Inc.
   Copyright (C) 1994, 1995, 1997, 1998, 1999, 2000 H. Peter Anvin

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

#include <config.h>

#include <sys/types.h>
#include <fnmatch.h>
#include <getopt.h>

#include "system.h"
#include "dircolors.h"
#include "c-ctype.h"
#include "c-strcase.h"
#include "obstack.h"
#include "quote.h"
#include "stdio--.h"

/* The official name of this program (e.g., no 'g' prefix).  */
#define PROGRAM_NAME "dircolors"

#define AUTHORS proper_name ("H. Peter Anvin")

#define obstack_chunk_alloc malloc
#define obstack_chunk_free free

enum Shell_syntax
{
  SHELL_SYNTAX_BOURNE,
  SHELL_SYNTAX_C,
  SHELL_SYNTAX_UNKNOWN
};

#define APPEND_CHAR(C) obstack_1grow (&lsc_obstack, C)

/* Accumulate in this obstack the value for the LS_COLORS environment
   variable.  */
static struct obstack lsc_obstack;

static char const *const slack_codes[] =
{
  "NORMAL", "NORM", "FILE", "RESET", "DIR", "LNK", "LINK",
  "SYMLINK", "ORPHAN", "MISSING", "FIFO", "PIPE", "SOCK", "BLK", "BLOCK",
  "CHR", "CHAR", "DOOR", "EXEC", "LEFT", "LEFTCODE", "RIGHT", "RIGHTCODE",
  "END", "ENDCODE", "SUID", "SETUID", "SGID", "SETGID", "STICKY",
  "OTHER_WRITABLE", "OWR", "STICKY_OTHER_WRITABLE", "OWT", "CAPABILITY",
  "MULTIHARDLINK", "CLRTOEOL", nullptr
};

static char const *const ls_codes[] =
{
  "no", "no", "fi", "rs", "di", "ln", "ln", "ln", "or", "mi", "pi", "pi",
  "so", "bd", "bd", "cd", "cd", "do", "ex", "lc", "lc", "rc", "rc", "ec", "ec",
  "su", "su", "sg", "sg", "st", "ow", "ow", "tw", "tw", "ca", "mh", "cl",
  nullptr
};
static_assert (countof (slack_codes) == countof (ls_codes));

/* Whether to output escaped ls color codes for display.  */
static bool print_ls_colors;

/* For long options that have no equivalent short option, use a
   non-character as a pseudo short option, starting with CHAR_MAX + 1.  */
enum
{
  PRINT_LS_COLORS_OPTION = CHAR_MAX + 1,
};

static struct option const long_options[] =
  {
    {"bourne-shell", no_argument, nullptr, 'b'},
    {"sh", no_argument, nullptr, 'b'},
    {"csh", no_argument, nullptr, 'c'},
    {"c-shell", no_argument, nullptr, 'c'},
    {"print-database", no_argument, nullptr, 'p'},
    {"print-ls-colors", no_argument, nullptr, PRINT_LS_COLORS_OPTION},
    {GETOPT_HELP_OPTION_DECL},
    {GETOPT_VERSION_OPTION_DECL},
    {nullptr, 0, nullptr, 0}
  };

void
usage (int status)
{
  if (status != EXIT_SUCCESS)
    emit_try_help ();
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
      --print-ls-colors       output fully escaped colors for display\n\
"), stdout);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      fputs (_("\
\n\
If FILE is specified, read it to determine which colors to use for which\n\
file types and extensions.  Otherwise, a precompiled database is used.\n\
For details on the format of these files, run 'dircolors --print-database'.\n\
"), stdout);
      emit_ancillary_info (PROGRAM_NAME);
    }

  exit (status);
}

/* If the SHELL environment variable is set to 'csh' or 'tcsh,'
   assume C shell.  Else Bourne shell.  */

static enum Shell_syntax
guess_shell_syntax (void)
{
  char *shell;

  shell = getenv ("SHELL");
  if (shell == nullptr || *shell == '\0')
    return SHELL_SYNTAX_UNKNOWN;

  shell = last_component (shell);

  if (streq (shell, "csh") || streq (shell, "tcsh"))
    return SHELL_SYNTAX_C;

  return SHELL_SYNTAX_BOURNE;
}

static void
parse_line (char const *line, char **keyword, char **arg)
{
  char const *p;
  char const *keyword_start;
  char const *arg_start;

  *keyword = nullptr;
  *arg = nullptr;

  for (p = line; c_isspace (*p); ++p)
    continue;

  /* Ignore blank lines and shell-style comments.  */
  if (*p == '\0' || *p == '#')
    return;

  keyword_start = p;

  while (!c_isspace (*p) && *p != '\0')
    {
      ++p;
    }

  *keyword = ximemdup0 (keyword_start, p - keyword_start);
  if (*p  == '\0')
    return;

  do
    {
      ++p;
    }
  while (c_isspace (*p));

  if (*p == '\0' || *p == '#')
    return;

  arg_start = p;

  while (*p != '\0' && *p != '#')
    ++p;

  for (--p; c_isspace (*p); --p)
    continue;
  ++p;

  *arg = ximemdup0 (arg_start, p - arg_start);
}

/* Accumulate STR to LS_COLORS data.
   If outputting shell syntax, then escape appropriately.  */

static void
append_quoted (char const *str)
{
  bool need_backslash = true;

  while (*str != '\0')
    {
      if (! print_ls_colors)
        switch (*str)
          {
          case '\'':
            APPEND_CHAR ('\'');
            APPEND_CHAR ('\\');
            APPEND_CHAR ('\'');
            need_backslash = true;
            break;

          case '\\':
          case '^':
            need_backslash = !need_backslash;
            break;

          case ':':
          case '=':
            if (need_backslash)
              APPEND_CHAR ('\\');
            FALLTHROUGH;

          default:
            need_backslash = true;
            break;
          }

      APPEND_CHAR (*str);
      ++str;
    }
}

/* Accumulate entry to LS_COLORS data.
   Use shell syntax unless PRINT_LS_COLORS is set.  */

static void
append_entry (char prefix, char const *item, char const *arg)
{
  if (print_ls_colors)
    {
      append_quoted ("\x1B[");
      append_quoted (arg);
      APPEND_CHAR ('m');
    }
  if (prefix)
    APPEND_CHAR (prefix);
  append_quoted (item);
  APPEND_CHAR (print_ls_colors ? '\t' : '=');
  append_quoted (arg);
  if (print_ls_colors)
    append_quoted ("\x1B[0m");
  APPEND_CHAR (print_ls_colors ? '\n' : ':');
}

/* Read the file open on FP (with name FILENAME).  First, look for a
   'TERM name' directive where name matches the current terminal type.
   Once found, translate and accumulate the associated directives onto
   the global obstack LSC_OBSTACK.  Give a diagnostic
   upon failure (unrecognized keyword is the only way to fail here).
   Return true if successful.  */

static bool
dc_parse_stream (FILE *fp, char const *filename)
{
  idx_t line_number = 0;
  char const *next_G_line = G_line;
  char *input_line = nullptr;
  size_t input_line_size = 0;
  char const *line;
  char const *term;
  char const *colorterm;
  bool ok = true;

  /* State for the parser.  */
  enum { ST_TERMNO, ST_TERMYES, ST_TERMSURE, ST_GLOBAL } state = ST_GLOBAL;

  /* Get terminal type */
  term = getenv ("TERM");
  if (term == nullptr || *term == '\0')
    term = "none";

  /* Also match $COLORTERM.  */
  colorterm = getenv ("COLORTERM");
  if (colorterm == nullptr)
    colorterm = "";  /* Doesn't match default "?*"  */

  while (true)
    {
      char *keywd, *arg;
      bool unrecognized;

      ++line_number;

      if (fp)
        {
          if (getline (&input_line, &input_line_size, fp) <= 0)
            {
              if (ferror (fp))
                {
                  error (0, errno, _("%s: read error"), quotef (filename));
                  ok = false;
                }
              free (input_line);
              break;
            }
          line = input_line;
        }
      else
        {
          if (next_G_line == G_line + sizeof G_line)
            break;
          line = next_G_line;
          next_G_line += strlen (next_G_line) + 1;
        }

      parse_line (line, &keywd, &arg);

      if (keywd == nullptr)
        continue;

      if (arg == nullptr)
        {
          error (0, 0, _("%s:%td: invalid line;  missing second token"),
                 quotef (filename), line_number);
          ok = false;
          free (keywd);
          continue;
        }

      unrecognized = false;
      if (c_strcasecmp (keywd, "TERM") == 0)
        {
          if (state != ST_TERMSURE)
            state = fnmatch (arg, term, 0) == 0 ? ST_TERMSURE : ST_TERMNO;
        }
      else if (c_strcasecmp (keywd, "COLORTERM") == 0)
        {
          if (state != ST_TERMSURE)
            state = fnmatch (arg, colorterm, 0) == 0 ? ST_TERMSURE : ST_TERMNO;
        }
      else
        {
          if (state == ST_TERMSURE)
            state = ST_TERMYES;  /* Another {COLOR,}TERM can cancel.  */

          if (state != ST_TERMNO)
            {
              if (keywd[0] == '.')
                append_entry ('*', keywd, arg);
              else if (keywd[0] == '*')
                append_entry (0, keywd, arg);
              else if (c_strcasecmp (keywd, "OPTIONS") == 0
                       || c_strcasecmp (keywd, "COLOR") == 0
                       || c_strcasecmp (keywd, "EIGHTBIT") == 0)
                {
                  /* Ignore.  */
                }
              else
                {
                  int i;

                  for (i = 0; slack_codes[i] != nullptr; ++i)
                    if (c_strcasecmp (keywd, slack_codes[i]) == 0)
                      break;

                  if (slack_codes[i] != nullptr)
                    append_entry (0, ls_codes[i], arg);
                  else
                    unrecognized = true;
                }
            }
          else
            unrecognized = true;
        }

      if (unrecognized && (state == ST_TERMSURE || state == ST_TERMYES))
        {
          error (0, 0, _("%s:%td: unrecognized keyword %s"),
                 (filename ? quotef (filename) : _("<internal>")),
                 line_number, keywd);
          ok = false;
        }

      free (keywd);
      free (arg);
    }

  return ok;
}

static bool
dc_parse_file (char const *filename)
{
  bool ok;

  if (! streq (filename, "-") && freopen (filename, "r", stdin) == nullptr)
    {
      error (0, errno, "%s", quotef (filename));
      return false;
    }

  ok = dc_parse_stream (stdin, filename);

  if (fclose (stdin) != 0)
    {
      error (0, errno, "%s", quotef (filename));
      return false;
    }

  return ok;
}

int
main (int argc, char **argv)
{
  bool ok = true;
  int optc;
  enum Shell_syntax syntax = SHELL_SYNTAX_UNKNOWN;
  bool print_database = false;

  initialize_main (&argc, &argv);
  set_program_name (argv[0]);
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  while ((optc = getopt_long (argc, argv, "bcp", long_options, nullptr)) != -1)
    switch (optc)
      {
      case 'b':	/* Bourne shell syntax.  */
        syntax = SHELL_SYNTAX_BOURNE;
        break;

      case 'c':	/* C shell syntax.  */
        syntax = SHELL_SYNTAX_C;
        break;

      case 'p':
        print_database = true;
        break;

      case PRINT_LS_COLORS_OPTION:
        print_ls_colors = true;
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
  if ((print_database | print_ls_colors) && syntax != SHELL_SYNTAX_UNKNOWN)
    {
      error (0, 0,
             _("the options to output non shell syntax,\n"
               "and to select a shell syntax are mutually exclusive"));
      usage (EXIT_FAILURE);
    }

  if (print_database && print_ls_colors)
    {
      error (0, 0,
             _("options --print-database and --print-ls-colors "
               "are mutually exclusive"));
      usage (EXIT_FAILURE);
    }

  if ((!print_database) < argc)
    {
      error (0, 0, _("extra operand %s"),
             quote (argv[!print_database]));
      if (print_database)
        fprintf (stderr, "%s\n",
                 _("file operands cannot be combined with "
                   "--print-database (-p)"));
      usage (EXIT_FAILURE);
    }

  if (print_database)
    {
      char const *p = G_line;
      while (p - G_line < sizeof G_line)
        {
          puts (p);
          p += strlen (p) + 1;
        }
    }
  else
    {
      /* If shell syntax was not explicitly specified, try to guess it. */
      if (syntax == SHELL_SYNTAX_UNKNOWN && ! print_ls_colors)
        {
          syntax = guess_shell_syntax ();
          if (syntax == SHELL_SYNTAX_UNKNOWN)
            error (EXIT_FAILURE, 0,
                   _("no SHELL environment variable,"
                     " and no shell type option given"));
        }

      obstack_init (&lsc_obstack);
      if (argc == 0)
        ok = dc_parse_stream (nullptr, nullptr);
      else
        ok = dc_parse_file (argv[0]);

      if (ok)
        {
          size_t len = obstack_object_size (&lsc_obstack);
          char *s = obstack_finish (&lsc_obstack);
          char const *prefix;
          char const *suffix;

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
          if (! print_ls_colors)
            fputs (prefix, stdout);
          fwrite (s, 1, len, stdout);
          if (! print_ls_colors)
            fputs (suffix, stdout);
        }
    }

  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
