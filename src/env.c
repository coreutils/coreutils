/* env - run a program in a modified environment
   Copyright (C) 1986-2018 Free Software Foundation, Inc.

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

/* Richard Mlynarik and David MacKenzie */

#include <config.h>
#include <stdio.h>
#include <sys/types.h>
#include <getopt.h>
#include <c-ctype.h>

#include <assert.h>
#include "system.h"
#include "die.h"
#include "error.h"
#include "quote.h"

/* The official name of this program (e.g., no 'g' prefix).  */
#define PROGRAM_NAME "env"

#define AUTHORS \
  proper_name ("Richard Mlynarik"), \
  proper_name ("David MacKenzie"), \
  proper_name ("Assaf Gordon")

/* array of envvars to unset. */
static const char** usvars;
static size_t usvars_alloc;
static size_t usvars_used;

/* Annotate the output with extra info to aid the user.  */
static bool dev_debug;

/* buffer and length of extracted envvars in -S strings. */
static char *varname;
static size_t vnlen;

static char const shortopts[] = "+C:iS:u:v0 \t";

static struct option const longopts[] =
{
  {"ignore-environment", no_argument, NULL, 'i'},
  {"null", no_argument, NULL, '0'},
  {"unset", required_argument, NULL, 'u'},
  {"chdir", required_argument, NULL, 'C'},
  {"debug", no_argument, NULL, 'v'},
  {"split-string", required_argument, NULL, 'S'},
  {GETOPT_HELP_OPTION_DECL},
  {GETOPT_VERSION_OPTION_DECL},
  {NULL, 0, NULL, 0}
};

void
usage (int status)
{
  if (status != EXIT_SUCCESS)
    emit_try_help ();
  else
    {
      printf (_("\
Usage: %s [OPTION]... [-] [NAME=VALUE]... [COMMAND [ARG]...]\n"),
              program_name);
      fputs (_("\
Set each NAME to VALUE in the environment and run COMMAND.\n\
"), stdout);

      emit_mandatory_arg_note ();

      fputs (_("\
  -i, --ignore-environment  start with an empty environment\n\
  -0, --null           end each output line with NUL, not newline\n\
  -u, --unset=NAME     remove variable from the environment\n\
"), stdout);
      fputs (_("\
  -C, --chdir=DIR      change working directory to DIR\n\
"), stdout);
      fputs (_("\
  -S, --split-string=S  process and split S into separate arguments;\n\
                        used to pass multiple arguments on shebang lines\n\
  -v, --debug          print verbose information for each processing step\n\
"), stdout);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      fputs (_("\
\n\
A mere - implies -i.  If no COMMAND, print the resulting environment.\n\
"), stdout);
      emit_ancillary_info (PROGRAM_NAME);
    }
  exit (status);
}

static void
append_unset_var (const char *var)
{
  if (usvars_used == usvars_alloc)
    usvars = x2nrealloc (usvars, &usvars_alloc, sizeof *usvars);
  usvars[usvars_used++] = var;
}

static void
unset_envvars (void)
{
  for (size_t i = 0; i < usvars_used; ++i)
    {
      devmsg ("unset:    %s\n", usvars[i]);

      if (unsetenv (usvars[i]))
        die (EXIT_CANCELED, errno, _("cannot unset %s"),
             quote (usvars[i]));
    }

  IF_LINT (free (usvars));
  IF_LINT (usvars = NULL);
  IF_LINT (usvars_used = 0);
  IF_LINT (usvars_alloc = 0);
}

static bool _GL_ATTRIBUTE_PURE
valid_escape_sequence (const char c)
{
  return (c == 'c' || c == 'f' || c == 'n' || c == 'r' || c == 't' || c == 'v' \
          || c == '#' || c == '$' || c == '_' || c == '"' || c == '\'' \
          || c == '\\');
}

static char _GL_ATTRIBUTE_PURE
escape_char (const char c)
{
  switch (c)
    {
    /* \a,\b not supported by FreeBSD's env. */
    case 'f': return '\f';
    case 'n': return '\n';
    case 'r': return '\r';
    case 't': return '\t';
    case 'v': return '\v';
    default:  assert (0);                           /* LCOV_EXCL_LINE */
    }
}

/* Return a pointer to the end of a valid ${VARNAME} string, or NULL.
   'str' should point to the '$' character.
   First letter in VARNAME must be alpha or underscore,
   rest of letters are alnum or underscore. Any other character is an error. */
static const char* _GL_ATTRIBUTE_PURE
scan_varname (const char* str)
{
  assert (str && *str == '$');                      /* LCOV_EXCL_LINE */
  if ( *(str+1) == '{' && (c_isalpha (*(str+2)) || *(str+2) == '_'))
    {
      const char* end = str+3;
      while (c_isalnum (*end) || *end == '_')
        ++end;
      if (*end == '}')
        return end;
    }

  return NULL;
}

/* Return a pointer to a static buffer containing the VARNAME as
   extracted from a '${VARNAME}' string.
   The returned string will be NUL terminated.
   The returned pointer should not be freed.
   Return NULL if not a valid ${VARNAME} syntax. */
static char*
extract_varname (const char* str)
{
  ptrdiff_t i;
  const char* p;

  p = scan_varname (str);
  if (!p)
    return NULL;

  /* -2 and +2 (below) account for the '${' prefix. */
  i = p - str - 2;

  if (i >= vnlen)
    {
      vnlen = i + 1;
      varname = xrealloc (varname, vnlen);
    }

  memcpy (varname, str+2, i);
  varname[i]=0;

  return varname;
}

/* Validate the "-S" parameter, according to the syntax defined by FreeBSD's
   env(1). Terminate with an error message if not valid.

   Calculate and set two values:
   bufsize - the size (in bytes) required to hold the resulting string
             after ENVVAR expansion (the value is overestimated).
   maxargc - the maximum number of arguments (the size of the new argv). */
static void
validate_split_str (const char* str, size_t* /*out*/ bufsize,
                    int* /*out*/ maxargc)
{
  bool dq, sq, sp;
  const char *pch;
  size_t buflen;
  int cnt = 1;

  assert (str && str[0] && !isspace (str[0]));     /* LCOV_EXCL_LINE */

  dq = sq = sp = false;
  buflen = strlen (str)+1;

  while (*str)
    {
      const char next = *(str+1);

      if (isspace (*str) && !dq && !sq)
        {
          sp = true;
        }
      else
        {
          if (sp)
            ++cnt;
          sp = false;
        }

      switch (*str)
        {
        case '\'':
          assert (!(sq && dq));                           /* LCOV_EXCL_LINE */
          sq = !sq && !dq;
          break;

        case '"':
          assert (!(sq && dq));                           /* LCOV_EXCL_LINE */
          dq = !sq && !dq;
          break;

        case '\\':
          if (dq && next == 'c')
            die (EXIT_CANCELED, 0,
                 _("'\\c' must not appear in double-quoted -S string"));

          if (next == '\0')
            die (EXIT_CANCELED, 0,
                 _("invalid backslash at end of string in -S"));

          if (!valid_escape_sequence (next))
            die (EXIT_CANCELED, 0, _("invalid sequence '\\%c' in -S"), next);

          if (next == '_')
            ++cnt;

          ++str;
          break;


        case '$':
          if (sq)
            break;

          if (!(pch = extract_varname (str)))
            die (EXIT_CANCELED, 0, _("only ${VARNAME} expansion is supported,"\
                                     " error at: %s"), str);

          if ((pch = getenv (pch)))
            buflen += strlen (pch);
          break;
        }
      ++str;
    }

  if (dq || sq)
    die (EXIT_CANCELED, 0, _("no terminating quote in -S string"));

  *maxargc = cnt;
  *bufsize = buflen;
}

/* Return a newly-allocated *arg[]-like array,
   by parsing and splitting the input 'str'.
   'extra_argc' is the number of additional elements to allocate
   in the array (on top of the number of args required to split 'str').

   Example:
     char **argv = build_argv ("A=B uname -k', 3)
   Results in:
     argv[0] = "DUMMY" - dummy executable name, can be replaced later.
     argv[1] = "A=B"
     argv[2] = "uname"
     argv[3] = "-k"
     argv[4] = NULL
     argv[5,6,7] = [allocated due to extra_argc, but not initialized]

   The strings are stored in an allocated buffer, pointed by argv[0].
   To free allocated memory:
     free (argv[0]);
     free (argv); */
static char**
build_argv (const char* str, int extra_argc)
{
  bool dq = false, sq = false, sep = true;
  char *dest;    /* buffer to hold the new argv values. allocated as one buffer,
                    but will contain multiple NUL-terminate strings. */
  char **newargv, **nextargv;
  int newargc = 0;
  size_t buflen = 0;

  /* This macro is called before inserting any characters to the output
     buffer. It checks if the previous character was a separator
     and if so starts a new argv element. */
#define CHECK_START_NEW_ARG                     \
  do {                                          \
    if (sep)                                    \
      {                                         \
        *dest++ = '\0';                         \
        *nextargv++ = dest;                     \
        sep = false;                            \
      }                                         \
  } while (0)

  assert (str && str[0] && !isspace (str[0]));             /* LCOV_EXCL_LINE */

  validate_split_str (str, &buflen, &newargc);

  /* allocate buffer. +6 for the "DUMMY\0" executable name, +1 for NUL. */
  dest = xmalloc (buflen + 6 + 1);

  /* allocate the argv array.
     +2 for the program name (argv[0]) and the last NULL pointer. */
  nextargv = newargv = xmalloc ((newargc + extra_argc + 2) * sizeof (char *));

  /* argv[0] = executable's name  - will be replaced later. */
  strcpy (dest, "DUMMY");
  *nextargv++ = dest;
  dest += 6;

  /* In the following loop,
     'break' causes the character 'newc' to be added to *dest,
     'continue' skips the character. */
  while (*str)
    {
      char newc = *str; /* default: add the next character. */

      switch (*str)
        {
        case '\'':
          if (dq)
            break;
          sq = !sq;
          CHECK_START_NEW_ARG;
          ++str;
          continue;

        case '"':
          if (sq)
            break;
          dq = !dq;
          CHECK_START_NEW_ARG;
          ++str;
          continue;

        case ' ':
        case '\t':
          /* space/tab outside quotes starts a new argument. */
          if (sq || dq)
            break;
          sep = true;
          str += strspn (str, " \t"); /* skip whitespace. */
          continue;

        case '#':
          if (!sep)
            break;
          goto eos; /* '#' as first char terminates the string. */

        case '\\':
          /* backslash inside single-quotes is not special, except \\ and \'. */
          if (sq && *(str+1) != '\\' && *(str+1) != '\'')
            break;

          /* skip the backslash and examine the next character. */
          newc = *(++str);
          if ((newc == '\\' || newc == '\'')
              || (!sq && (newc == '#' || newc == '$' || newc == '"')))
            {
              /* Pass escaped character as-is. */
            }
          else if (newc == '_')
            {
              if (!dq)
                {
                  ++str;  /* '\_' outside double-quotes is arg separator. */
                  sep = true;
                  continue;
                }
              else
                  newc = ' ';  /* '\_' inside double-quotes is space. */
            }
          else if (newc == 'c')
              goto eos; /* '\c' terminates the string. */
          else
              newc = escape_char (newc); /* other characters (e.g. '\n'). */
          break;

        case '$':
          /* ${VARNAME} are not expanded inside single-quotes. */
          if (sq)
            break;

          /* Store the ${VARNAME} value. Error checking omitted as
             the ${VARNAME} was already validated. */
          {
            char *n = extract_varname (str);
            char *v = getenv (n);
            if (v)
              {
                CHECK_START_NEW_ARG;
                devmsg ("expanding ${%s} into %s\n", n, quote (v));
                dest = stpcpy (dest, v);
              }
            else
              devmsg ("replacing ${%s} with null string\n", n);

            str = strchr (str, '}') + 1;
            continue;
          }

        }

      CHECK_START_NEW_ARG;
      *dest++ = newc;
      ++str;
    }

 eos:
  *dest = '\0';
  *nextargv = NULL; /* mark the last element in argv as NULL. */

  return newargv;
}

/* Process an "-S" string and create the corresponding argv array.
   Update the given argc/argv parameters with the new argv.

   Example: if executed as:
      $ env -S"-i -C/tmp A=B" foo bar
   The input argv is:
      argv[0] = 'env'
      argv[1] = "-S-i -C/tmp A=B"
      argv[2] = foo
      argv[3] = bar
   This function will modify argv to be:
      argv[0] = 'env'
      argv[1] = "-i"
      argv[2] = "-C/tmp"
      argv[3] =  A=B"
      argv[4] = foo
      argv[5] = bar
   argc will be updated from 4 to 6.
   optind will be reset to 0 to force getopt_long to rescan all arguments. */
static void
parse_split_string (const char* str, int /*out*/ *orig_optind,
                    int /*out*/ *orig_argc, char*** /*out*/ orig_argv)
{
  int i, newargc;
  char **newargv, **nextargv;


  while (isspace (*str))
    str++;
  if (*str == '\0')
    return;

  newargv = build_argv (str, *orig_argc - *orig_optind);

  /* restore argv[0] - the 'env' executable name */
  *newargv = (*orig_argv)[0];

  /* Start from argv[1] */
  nextargv = newargv + 1;

  /* Print parsed arguments */
  if (dev_debug && *nextargv)
    {
      devmsg ("split -S:  %s\n", quote (str));
      devmsg (" into:    %s\n", quote (*nextargv++));
      while (*nextargv)
        devmsg ("     &    %s\n", quote (*nextargv++));
    }
  else
    {
      /* Ensure nextargv points to the last argument */
      while (*nextargv)
        ++nextargv;
    }

  /* Add remaining arguments from original command line */
  for (i = *orig_optind; i < *orig_argc; ++i)
    *nextargv++ = (*orig_argv)[i];
  *nextargv = NULL;

  /* Count how many new arguments we have */
  newargc = 0;
  for (nextargv = newargv; *nextargv; ++nextargv)
    ++newargc;

  /* set new values for original getopt variables */
  *orig_argc = newargc;
  *orig_argv = newargv;
  *orig_optind = 0; /* tell getopt to restart from first argument */
}

int
main (int argc, char **argv)
{
  int optc;
  bool ignore_environment = false;
  bool opt_nul_terminate_output = false;
  char const *newdir = NULL;

  initialize_main (&argc, &argv);
  set_program_name (argv[0]);
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  initialize_exit_failure (EXIT_CANCELED);
  atexit (close_stdout);

  while ((optc = getopt_long (argc, argv, shortopts, longopts, NULL)) != -1)
    {
      switch (optc)
        {
        case 'i':
          ignore_environment = true;
          break;
        case 'u':
          append_unset_var (optarg);
          break;
        case 'v':
          dev_debug = true;
          break;
        case '0':
          opt_nul_terminate_output = true;
          break;
        case 'C':
          newdir = optarg;
          break;
        case 'S':
          parse_split_string (optarg, &optind, &argc, &argv);
          break;
        case ' ':
        case '\t':
          /* These are undocumented options. Attempt to detect
             incorrect shebang usage with extraneous space, e.g.:
                #!/usr/bin/env -i command
             In which case argv[1] == "-i command".  */
          error (0, 0, _("invalid option -- '%c'"), optc);
          error (0, 0, _("use -[v]S to pass options in shebang lines"));
          usage (EXIT_CANCELED);

        case_GETOPT_HELP_CHAR;
        case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);
        default:
          usage (EXIT_CANCELED);
        }
    }

  if (optind < argc && STREQ (argv[optind], "-"))
    {
      ignore_environment = true;
      ++optind;
    }

  if (ignore_environment)
    {
      devmsg ("cleaning environ\n");
      static char *dummy_environ[] = { NULL };
      environ = dummy_environ;
    }
  else
    unset_envvars ();

  char *eq;
  while (optind < argc && (eq = strchr (argv[optind], '=')))
    {
      devmsg ("setenv:   %s\n", argv[optind]);

      if (putenv (argv[optind]))
        {
          *eq = '\0';
          die (EXIT_CANCELED, errno, _("cannot set %s"),
               quote (argv[optind]));
        }
      optind++;
    }

  bool program_specified = optind < argc;

  if (opt_nul_terminate_output && program_specified)
    {
      error (0, 0, _("cannot specify --null (-0) with command"));
      usage (EXIT_CANCELED);
    }

  if (newdir && ! program_specified)
    {
      error (0, 0, _("must specify command with --chdir (-C)"));
      usage (EXIT_CANCELED);
    }

  if (! program_specified)
    {
      /* Print the environment and exit. */
      char *const *e = environ;
      while (*e)
        printf ("%s%c", *e++, opt_nul_terminate_output ? '\0' : '\n');
      return EXIT_SUCCESS;
    }

  if (newdir)
    {
      devmsg ("chdir:    %s\n", quoteaf (newdir));

      if (chdir (newdir) != 0)
        die (EXIT_CANCELED, errno, _("cannot change directory to %s"),
             quoteaf (newdir));
    }

  if (dev_debug)
    {
      devmsg ("executing: %s\n", argv[optind]);
      for (int i=optind; i<argc; ++i)
        devmsg ("   arg[%d]= %s\n", i-optind, quote (argv[i]));
    }

  execvp (argv[optind], &argv[optind]);

  int exit_status = errno == ENOENT ? EXIT_ENOENT : EXIT_CANNOT_INVOKE;
  error (0, errno, "%s", quote (argv[optind]));

  if (exit_status == EXIT_ENOENT && strchr (argv[optind], ' '))
    error (0, 0, _("use -[v]S to pass options in shebang lines"));

  return exit_status;
}
