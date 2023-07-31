/* env - run a program in a modified environment
   Copyright (C) 1986-2023 Free Software Foundation, Inc.

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
#include <signal.h>

#include "system.h"
#include "operand2sig.h"
#include "quote.h"
#include "sig2str.h"

/* The official name of this program (e.g., no 'g' prefix).  */
#define PROGRAM_NAME "env"

#define AUTHORS \
  proper_name ("Richard Mlynarik"), \
  proper_name ("David MacKenzie"), \
  proper_name ("Assaf Gordon")

/* Array of envvars to unset.  */
static char const **usvars;
static size_t usvars_alloc;
static idx_t usvars_used;

/* Annotate the output with extra info to aid the user.  */
static bool dev_debug;

/* Buffer and length of extracted envvars in -S strings.  */
static char *varname;
static idx_t vnlen;

/* Possible actions on each signal.  */
enum SIGNAL_MODE {
  UNCHANGED = 0,
  DEFAULT,       /* Set to default handler (SIG_DFL).  */
  DEFAULT_NOERR, /* Ditto, but ignore sigaction(2) errors.  */
  IGNORE,        /* Set to ignore (SIG_IGN).  */
  IGNORE_NOERR   /* Ditto, but ignore sigaction(2) errors.  */
};
static enum SIGNAL_MODE *signals;

/* Set of signals to block.  */
static sigset_t block_signals;

/* Set of signals to unblock.  */
static sigset_t unblock_signals;

/* Whether signal mask adjustment requested.  */
static bool sig_mask_changed;

/* Whether to list non default handling.  */
static bool report_signal_handling;

/* The isspace characters in the C locale.  */
#define C_ISSPACE_CHARS " \t\n\v\f\r"

static char const shortopts[] = "+C:iS:u:v0" C_ISSPACE_CHARS;

/* For long options that have no equivalent short option, use a
   non-character as a pseudo short option, starting with CHAR_MAX + 1.  */
enum
{
  DEFAULT_SIGNAL_OPTION = CHAR_MAX + 1,
  IGNORE_SIGNAL_OPTION,
  BLOCK_SIGNAL_OPTION,
  LIST_SIGNAL_HANDLING_OPTION,
};

static struct option const longopts[] =
{
  {"ignore-environment", no_argument, nullptr, 'i'},
  {"null", no_argument, nullptr, '0'},
  {"unset", required_argument, nullptr, 'u'},
  {"chdir", required_argument, nullptr, 'C'},
  {"default-signal", optional_argument, nullptr, DEFAULT_SIGNAL_OPTION},
  {"ignore-signal",  optional_argument, nullptr, IGNORE_SIGNAL_OPTION},
  {"block-signal",   optional_argument, nullptr, BLOCK_SIGNAL_OPTION},
  {"list-signal-handling", no_argument, nullptr,  LIST_SIGNAL_HANDLING_OPTION},
  {"debug", no_argument, nullptr, 'v'},
  {"split-string", required_argument, nullptr, 'S'},
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
"), stdout);
      fputs (_("\
      --block-signal[=SIG]    block delivery of SIG signal(s) to COMMAND\n\
"), stdout);
      fputs (_("\
      --default-signal[=SIG]  reset handling of SIG signal(s) to the default\n\
"), stdout);
      fputs (_("\
      --ignore-signal[=SIG]   set handling of SIG signal(s) to do nothing\n\
"), stdout);
      fputs (_("\
      --list-signal-handling  list non default signal handling to stderr\n\
"), stdout);
      fputs (_("\
  -v, --debug          print verbose information for each processing step\n\
"), stdout);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      fputs (_("\
\n\
A mere - implies -i.  If no COMMAND, print the resulting environment.\n\
"), stdout);
      fputs (_("\
\n\
SIG may be a signal name like 'PIPE', or a signal number like '13'.\n\
Without SIG, all known signals are included.  Multiple signals can be\n\
comma-separated.  An empty SIG argument is a no-op.\n\
"), stdout);
      emit_exec_status (PROGRAM_NAME);
      emit_ancillary_info (PROGRAM_NAME);
    }
  exit (status);
}

static void
append_unset_var (char const *var)
{
  if (usvars_used == usvars_alloc)
    usvars = x2nrealloc (usvars, &usvars_alloc, sizeof *usvars);
  usvars[usvars_used++] = var;
}

static void
unset_envvars (void)
{
  for (idx_t i = 0; i < usvars_used; ++i)
    {
      devmsg ("unset:    %s\n", usvars[i]);

      if (unsetenv (usvars[i]))
        error (EXIT_CANCELED, errno, _("cannot unset %s"),
               quote (usvars[i]));
    }
}

/* Return a pointer to the end of a valid ${VARNAME} string, or nullptr.
   'str' should point to the '$' character.
   First letter in VARNAME must be alpha or underscore,
   rest of letters are alnum or underscore.
   Any other character is an error.  */
ATTRIBUTE_PURE
static char const *
scan_varname (char const *str)
{
  if (str[1] == '{' && (c_isalpha (str[2]) || str[2] == '_'))
    {
      char const *end = str + 3;
      while (c_isalnum (*end) || *end == '_')
        ++end;
      if (*end == '}')
        return end;
    }

  return nullptr;
}

/* Return a pointer to a static buffer containing the VARNAME as
   extracted from a '${VARNAME}' string.
   The returned string will be NUL terminated.
   The returned pointer should not be freed.
   Return nullptr if not a valid ${VARNAME} syntax.  */
static char *
extract_varname (char const *str)
{
  idx_t i;
  char const *p;

  p = scan_varname (str);
  if (!p)
    return nullptr;

  /* -2 and +2 (below) account for the '${' prefix.  */
  i = p - str - 2;

  if (i >= vnlen)
    {
      vnlen = i + 1;
      varname = xrealloc (varname, vnlen);
    }

  memcpy (varname, str + 2, i);
  varname[i] = 0;

  return varname;
}

/* Temporary buffer used by --split-string processing.  */
struct splitbuf
{
  /* Buffer address, arg count, and half the number of elements in the buffer.
     ARGC and ARGV are as in 'main', and ARGC + 1 <= HALF_ALLOC so
     that the upper half of ARGV can be used for string contents.
     This may waste up to half the space but keeps the code simple,
     which is better for this rarely-used but security-sensitive code.

     ARGV[0] is not initialized; that is the caller's responsibility
     after finalization.

     During assembly, ARGV[I] (where 0 < I < ARGC) contains the offset
     of the Ith string (relative to ARGV + HALF_ALLOC), so that
     reallocating ARGV does not change the validity of its contents.
     The integer offset is cast to char * during assembly, and is
     converted to a true char * pointer on finalization.

     During assembly, ARGV[ARGC] contains the offset of the first
     unused string byte (relative to ARGV + HALF_ALLOC).  */
  char **argv;
  int argc;
  idx_t half_alloc;

  /* The number of extra argv slots to keep room for.  */
  int extra_argc;

  /* Whether processing should act as if the most recent character
     seen was a separator.  */
  bool sep;
};

/* Expand SS so that it has at least one more argv slot and at least
   one more string byte.  */
static void
splitbuf_grow (struct splitbuf *ss)
{
  idx_t old_half_alloc = ss->half_alloc;
  idx_t string_bytes = (intptr_t) ss->argv[ss->argc];
  ss->argv = xpalloc (ss->argv, &ss->half_alloc, 1,
                      MIN (INT_MAX, IDX_MAX), 2 * sizeof *ss->argv);
  memmove (ss->argv + ss->half_alloc, ss->argv + old_half_alloc, string_bytes);
}

/* In SS, append C to the last string.  */
static void
splitbuf_append_byte (struct splitbuf *ss, char c)
{
  idx_t string_bytes = (intptr_t) ss->argv[ss->argc];
  if (ss->half_alloc * sizeof *ss->argv <= string_bytes)
    splitbuf_grow (ss);
  ((char *) (ss->argv + ss->half_alloc))[string_bytes] = c;
  ss->argv[ss->argc] = (char *) (intptr_t) (string_bytes + 1);
}

/* If SS's most recent character was a separator, finish off its
   previous argument and start a new one.  */
static void
check_start_new_arg (struct splitbuf *ss)
{
  if (ss->sep)
    {
      splitbuf_append_byte (ss, '\0');
      int argc = ss->argc;
      if (ss->half_alloc <= argc + ss->extra_argc + 1)
        splitbuf_grow (ss);
      ss->argv[argc + 1] = ss->argv[argc];
      ss->argc = argc + 1;
      ss->sep = false;
    }
}

/* All additions to SS have been made.  Convert its offsets to pointers,
   and return the resulting argument vector.  */
static char **
splitbuf_finishup (struct splitbuf *ss)
{
  int argc = ss->argc;
  char **argv = ss->argv;
  char *stringbase = (char *) (ss->argv + ss->half_alloc);
  for (int i = 1; i < argc; i++)
    argv[i] = stringbase + (intptr_t) argv[i];
  return argv;
}

/* Return a newly-allocated argv-like array,
   by parsing and splitting the input 'str'.

   'extra_argc' is the number of additional elements to allocate
   in the array (on top of the number of args required to split 'str').

   Store into *argc the number of arguments found (plus 1 for
   the program name).

   Example:
     int argc;
     char **argv = build_argv ("A=B uname -k', 3, &argc);
   Results in:
     argc = 4
     argv[0] = [not initialized]
     argv[1] = "A=B"
     argv[2] = "uname"
     argv[3] = "-k"
     argv[4,5,6,7] = [allocated due to extra_argc + 1, but not initialized]

   To free allocated memory:
     free (argv);
   However, 'env' does not free since it's about to exec or exit anyway
   and the complexity of keeping track of the storage that may have been
   allocated via multiple calls to build_argv is not worth the hassle.  */
static char **
build_argv (char const *str, int extra_argc, int *argc)
{
  bool dq = false, sq = false;
  struct splitbuf ss;
  ss.argv = xnmalloc (extra_argc + 2, 2 * sizeof *ss.argv);
  ss.argc = 1;
  ss.half_alloc = extra_argc + 2;
  ss.extra_argc = extra_argc;
  ss.sep = true;
  ss.argv[ss.argc] = 0;

  /* In the following loop,
     'break' causes the character 'newc' to be added to *dest,
     'continue' skips the character.  */
  while (*str)
    {
      char newc = *str; /* Default: add the next character.  */

      switch (*str)
        {
        case '\'':
          if (dq)
            break;
          sq = !sq;
          check_start_new_arg (&ss);
          ++str;
          continue;

        case '"':
          if (sq)
            break;
          dq = !dq;
          check_start_new_arg (&ss);
          ++str;
          continue;

        case ' ': case '\t': case '\n': case '\v': case '\f': case '\r':
          /* Start a new argument if outside quotes.  */
          if (sq || dq)
            break;
          ss.sep = true;
          str += strspn (str, C_ISSPACE_CHARS);
          continue;

        case '#':
          if (!ss.sep)
            break;
          goto eos; /* '#' as first char terminates the string.  */

        case '\\':
          /* Backslash inside single-quotes is not special, except \\
             and \'.  */
          if (sq && str[1] != '\\' && str[1] != '\'')
            break;

          /* Skip the backslash and examine the next character.  */
          newc = *++str;
          switch (newc)
            {
            case '"': case '#': case '$': case '\'': case '\\':
              /* Pass escaped character as-is.  */
              break;

            case '_':
              if (!dq)
                {
                  ++str;  /* '\_' outside double-quotes is arg separator.  */
                  ss.sep = true;
                  continue;
                }
              newc = ' ';  /* '\_' inside double-quotes is space.  */
              break;

            case 'c':
              if (dq)
                error (EXIT_CANCELED, 0,
                       _("'\\c' must not appear in double-quoted -S string"));
              goto eos; /* '\c' terminates the string.  */

            case 'f': newc = '\f'; break;
            case 'n': newc = '\n'; break;
            case 'r': newc = '\r'; break;
            case 't': newc = '\t'; break;
            case 'v': newc = '\v'; break;

            case '\0':
              error (EXIT_CANCELED, 0,
                     _("invalid backslash at end of string in -S"));

            default:
              error (EXIT_CANCELED, 0,
                     _("invalid sequence '\\%c' in -S"), newc);
            }
          break;

        case '$':
          /* ${VARNAME} are not expanded inside single-quotes.  */
          if (sq)
            break;

          /* Store the ${VARNAME} value.  */
          {
            char *n = extract_varname (str);
            if (!n)
              error (EXIT_CANCELED, 0,
                     _("only ${VARNAME} expansion is supported, error at: %s"),
                     str);

            char *v = getenv (n);
            if (v)
              {
                check_start_new_arg (&ss);
                devmsg ("expanding ${%s} into %s\n", n, quote (v));
                for (; *v; v++)
                  splitbuf_append_byte (&ss, *v);
              }
            else
              devmsg ("replacing ${%s} with null string\n", n);

            str = strchr (str, '}') + 1;
            continue;
          }
        }

      check_start_new_arg (&ss);
      splitbuf_append_byte (&ss, newc);
      ++str;
    }

  if (dq || sq)
    error (EXIT_CANCELED, 0, _("no terminating quote in -S string"));

 eos:
  splitbuf_append_byte (&ss, '\0');
  *argc = ss.argc;
  return splitbuf_finishup (&ss);
}

/* Process an "-S" string and create the corresponding argv array.
   Update the given argc/argv parameters with the new argv.

   Example: if executed as:
      $ env -S"-i -C/tmp A=B" foo bar
   The input argv is:
      argv[0] = "env"
      argv[1] = "-S-i -C/tmp A=B"
      argv[2] = "foo"
      argv[3] = "bar"
      argv[4] = nullptr
   This function will modify argv to be:
      argv[0] = "env"
      argv[1] = "-i"
      argv[2] = "-C/tmp"
      argv[3] = "A=B"
      argv[4] = "foo"
      argv[5] = "bar"
      argv[6] = nullptr
   argc will be updated from 4 to 6.
   optind will be reset to 0 to force getopt_long to rescan all arguments.  */
static void
parse_split_string (char const *str, int *orig_optind,
                    int *orig_argc, char ***orig_argv)
{
  int extra_argc = *orig_argc - *orig_optind, newargc;
  char **newargv = build_argv (str, extra_argc, &newargc);

  /* Restore argv[0] - the 'env' executable name.  */
  *newargv = (*orig_argv)[0];

  /* Print parsed arguments.  */
  if (dev_debug && 1 < newargc)
    {
      devmsg ("split -S:  %s\n", quote (str));
      devmsg (" into:    %s\n", quote (newargv[1]));
      for (int i = 2; i < newargc; i++)
        devmsg ("     &    %s\n", quote (newargv[i]));
    }

  /* Add remaining arguments and terminating null from the original
     command line.  */
  memcpy (newargv + newargc, *orig_argv + *orig_optind,
          (extra_argc + 1) * sizeof *newargv);

  /* Set new values for original getopt variables.  */
  *orig_argc = newargc + extra_argc;
  *orig_argv = newargv;
  *orig_optind = 0; /* Tell getopt to restart from first argument.  */
}

static void
parse_signal_action_params (char const *optarg, bool set_default)
{
  char signame[SIG2STR_MAX];
  char *opt_sig;
  char *optarg_writable;

  if (! optarg)
    {
      /* Without an argument, reset all signals.
         Some signals cannot be set to ignore or default (e.g., SIGKILL,
         SIGSTOP on most OSes, and SIGCONT on AIX.) - so ignore errors.  */
      for (int i = 1 ; i <= SIGNUM_BOUND; i++)
        if (sig2str (i, signame) == 0)
          signals[i] = set_default ? DEFAULT_NOERR : IGNORE_NOERR;
      return;
    }

  optarg_writable = xstrdup (optarg);

  opt_sig = strtok (optarg_writable, ",");
  while (opt_sig)
    {
      int signum = operand2sig (opt_sig, signame);
      /* operand2sig accepts signal 0 (EXIT) - but we reject it.  */
      if (signum == 0)
        error (0, 0, _("%s: invalid signal"), quote (opt_sig));
      if (signum <= 0)
        usage (exit_failure);

      signals[signum] = set_default ? DEFAULT : IGNORE;

      opt_sig = strtok (nullptr, ",");
    }

  free (optarg_writable);
}

static void
reset_signal_handlers (void)
{
  for (int i = 1; i <= SIGNUM_BOUND; i++)
    {
      struct sigaction act;

      if (signals[i] == UNCHANGED)
        continue;

      bool ignore_errors = (signals[i] == DEFAULT_NOERR
                            || signals[i] == IGNORE_NOERR);

      bool set_to_default = (signals[i] == DEFAULT
                             || signals[i] == DEFAULT_NOERR);

      int sig_err = sigaction (i, nullptr, &act);

      if (sig_err && !ignore_errors)
        error (EXIT_CANCELED, errno,
               _("failed to get signal action for signal %d"), i);

      if (! sig_err)
        {
          act.sa_handler = set_to_default ? SIG_DFL : SIG_IGN;
          sig_err = sigaction (i, &act, nullptr);
          if (sig_err && !ignore_errors)
            error (EXIT_CANCELED, errno,
                   _("failed to set signal action for signal %d"), i);
        }

      if (dev_debug)
        {
          char signame[SIG2STR_MAX];
          sig2str (i, signame);
          devmsg ("Reset signal %s (%d) to %s%s\n",
                  signame, i,
                  set_to_default ? "DEFAULT" : "IGNORE",
                  sig_err ? " (failure ignored)" : "");
        }
    }
}


static void
parse_block_signal_params (char const *optarg, bool block)
{
  char signame[SIG2STR_MAX];
  char *opt_sig;
  char *optarg_writable;

  if (! optarg)
    {
      /* Without an argument, reset all signals.  */
      sigfillset (block ? &block_signals : &unblock_signals);
      sigemptyset (block ? &unblock_signals : &block_signals);
    }
  else if (! sig_mask_changed)
    {
      /* Initialize the sets.  */
      sigemptyset (&block_signals);
      sigemptyset (&unblock_signals);
    }

  sig_mask_changed = true;

  if (! optarg)
    return;

  optarg_writable = xstrdup (optarg);

  opt_sig = strtok (optarg_writable, ",");
  while (opt_sig)
    {
      int signum = operand2sig (opt_sig, signame);
      /* operand2sig accepts signal 0 (EXIT) - but we reject it.  */
      if (signum == 0)
        error (0, 0, _("%s: invalid signal"), quote (opt_sig));
      if (signum <= 0)
        usage (exit_failure);

      sigaddset (block ? &block_signals : &unblock_signals, signum);
      sigdelset (block ? &unblock_signals : &block_signals, signum);

      opt_sig = strtok (nullptr, ",");
    }

  free (optarg_writable);
}

static void
set_signal_proc_mask (void)
{
  /* Get the existing signal mask */
  sigset_t set;
  char const *debug_act;

  sigemptyset (&set);

  if (sigprocmask (0, nullptr, &set))
    error (EXIT_CANCELED, errno, _("failed to get signal process mask"));

  for (int i = 1; i <= SIGNUM_BOUND; i++)
    {
      if (sigismember (&block_signals, i))
        {
          sigaddset (&set, i);
          debug_act = "BLOCK";
        }
      else if (sigismember (&unblock_signals, i))
        {
          sigdelset (&set, i);
          debug_act = "UNBLOCK";
        }
      else
        {
          debug_act = nullptr;
        }

      if (dev_debug && debug_act)
        {
          char signame[SIG2STR_MAX];
          sig2str (i, signame);
          devmsg ("signal %s (%d) mask set to %s\n",
                  signame, i, debug_act);
        }
    }

  if (sigprocmask (SIG_SETMASK, &set, nullptr))
    error (EXIT_CANCELED, errno, _("failed to set signal process mask"));
}

static void
list_signal_handling (void)
{
  sigset_t set;
  char signame[SIG2STR_MAX];

  sigemptyset (&set);
  if (sigprocmask (0, nullptr, &set))
    error (EXIT_CANCELED, errno, _("failed to get signal process mask"));

  for (int i = 1; i <= SIGNUM_BOUND; i++)
    {
      struct sigaction act;
      if (sigaction (i, nullptr, &act))
        continue;

      char const *ignored = act.sa_handler == SIG_IGN ? "IGNORE" : "";
      char const *blocked = sigismember (&set, i) ? "BLOCK" : "";
      char const *connect = *ignored && *blocked ? "," : "";

      if (! *ignored && ! *blocked)
        continue;

      sig2str (i, signame);
      fprintf (stderr, "%-10s (%2d): %s%s%s\n", signame, i,
               blocked, connect, ignored);
    }
}

static void
initialize_signals (void)
{
  signals = xmalloc ((sizeof *signals) * (SIGNUM_BOUND + 1));

  for (int i = 0 ; i <= SIGNUM_BOUND; i++)
    signals[i] = UNCHANGED;

  return;
}

int
main (int argc, char **argv)
{
  int optc;
  bool ignore_environment = false;
  bool opt_nul_terminate_output = false;
  char const *newdir = nullptr;

  initialize_main (&argc, &argv);
  set_program_name (argv[0]);
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  initialize_exit_failure (EXIT_CANCELED);
  atexit (close_stdout);

  initialize_signals ();

  while ((optc = getopt_long (argc, argv, shortopts, longopts, nullptr)) != -1)
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
        case DEFAULT_SIGNAL_OPTION:
          parse_signal_action_params (optarg, true);
          parse_block_signal_params (optarg, false);
          break;
        case IGNORE_SIGNAL_OPTION:
          parse_signal_action_params (optarg, false);
          break;
        case BLOCK_SIGNAL_OPTION:
          parse_block_signal_params (optarg, true);
          break;
        case LIST_SIGNAL_HANDLING_OPTION:
          report_signal_handling = true;
          break;
        case 'C':
          newdir = optarg;
          break;
        case 'S':
          parse_split_string (optarg, &optind, &argc, &argv);
          break;
        case ' ': case '\t': case '\n': case '\v': case '\f': case '\r':
          /* These are undocumented options.  Attempt to detect
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
      static char *dummy_environ[] = { nullptr };
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
          error (EXIT_CANCELED, errno, _("cannot set %s"),
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
      /* Print the environment and exit.  */
      char *const *e = environ;
      while (*e)
        printf ("%s%c", *e++, opt_nul_terminate_output ? '\0' : '\n');
      return EXIT_SUCCESS;
    }

  reset_signal_handlers ();
  if (sig_mask_changed)
    set_signal_proc_mask ();

  if (report_signal_handling)
    list_signal_handling ();

  if (newdir)
    {
      devmsg ("chdir:    %s\n", quoteaf (newdir));

      if (chdir (newdir) != 0)
        error (EXIT_CANCELED, errno, _("cannot change directory to %s"),
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

  if (exit_status == EXIT_ENOENT && strpbrk (argv[optind], C_ISSPACE_CHARS))
    error (0, 0, _("use -[v]S to pass options in shebang lines"));

  main_exit (exit_status);
}
