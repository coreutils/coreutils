/* timeout -- run a command with bounded time
   Copyright (C) 2008-2009 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */


/* timeout - Start a command, and kill it if the specified timeout expires

   We try to behave like a shell starting a single (foreground) job,
   and will kill the job if we receive the alarm signal we setup.
   The exit status of the job is returned, or one of these errors:
     EXIT_TIMEDOUT      124      job timed out
     EXIT_CANCELED      125      internal error
     EXIT_CANNOT_INVOKE 126      error executing job
     EXIT_ENOENT        127      couldn't find job to exec

   Caveats:
     If user specifies the KILL (9) signal is to be sent on timeout,
     the monitor is killed and so exits with 128+9 rather than 124.

     If you start a command in the background, which reads from the tty
     and so is immediately sent SIGTTIN to stop, then the timeout
     process will ignore this so it can timeout the command as expected.
     This can be seen with `timeout 10 dd&` for example.
     However if one brings this group to the foreground with the `fg`
     command before the timer expires, the command will remain
     in the sTop state as the shell doesn't send a SIGCONT
     because the timeout process (group leader) is already running.
     To get the command running again one can Ctrl-Z, and do fg again.
     Note one can Ctrl-C the whole job when in this state.
     I think this could be fixed but I'm not sure the extra
     complication is justified for this scenario.

   Written by Pádraig Brady.  */

#include <config.h>
#include <getopt.h>
#include <stdio.h>
#include <sys/types.h>
#include <signal.h>

#if HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif
#ifndef WIFSIGNALED
# define WIFSIGNALED(s) (((s) & 0xFFFF) - 1 < (unsigned int) 0xFF)
#endif
#ifndef WTERMSIG
# define WTERMSIG(s) ((s) & 0x7F)
#endif

#include "system.h"
#include "xstrtol.h"
#include "sig2str.h"
#include "operand2sig.h"
#include "cloexec.h"
#include "error.h"
#include "long-options.h"
#include "quote.h"

#define PROGRAM_NAME "timeout"

#define AUTHORS proper_name_utf8 ("Padraig Brady", "P\303\241draig Brady")

/* Note ETIMEDOUT is 110 on GNU/Linux systems but this is non standard */
#define EXIT_TIMEDOUT 124

/* Internal failure.  */
#define EXIT_CANCELED 125

static int timed_out;
static int term_signal = SIGTERM;  /* same default as kill command.  */
static int monitored_pid;
static int sigs_to_ignore[NSIG];   /* so monitor can ignore sigs it resends.  */

static struct option const long_options[] =
{
  {"signal", required_argument, NULL, 's'},
  {NULL, 0, NULL, 0}
};

/* send sig to group but not ourselves.
 * FIXME: Is there a better way to achieve this?  */
static int
send_sig (int where, int sig)
{
  sigs_to_ignore[sig] = 1;
  return kill (where, sig);
}

static void
cleanup (int sig)
{
  if (sig == SIGALRM)
    {
      timed_out = 1;
      sig = term_signal;
    }
  if (monitored_pid)
    {
      if (sigs_to_ignore[sig])
        {
          sigs_to_ignore[sig] = 0;
          return;
        }
      send_sig (0, sig);
      if (sig != SIGKILL && sig != SIGCONT)
        send_sig (0, SIGCONT);
    }
  else /* we're the child or the child is not exec'd yet.  */
    _exit (128 + sig);
}

void
usage (int status)
{
  if (status != EXIT_SUCCESS)
    fprintf (stderr, _("Try `%s --help' for more information.\n"),
             program_name);
  else
    {
      printf (_("\
Usage: %s [OPTION] NUMBER[SUFFIX] COMMAND [ARG]...\n\
  or:  %s [OPTION]\n"), program_name, program_name);

      fputs (_("\
Start COMMAND, and kill it if still running after NUMBER seconds.\n\
SUFFIX may be `s' for seconds (the default), `m' for minutes,\n\
`h' for hours or `d' for days.\n\
\n\
"), stdout);

      fputs (_("\
Mandatory arguments to long options are mandatory for short options too.\n\
"), stdout);
      fputs (_("\
  -s, --signal=SIGNAL\n\
                   specify the signal to be sent on timeout.\n\
                   SIGNAL may be a name like `HUP' or a number.\n\
                   See `kill -l` for a list of signals\n"), stdout);

      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      fputs (_("\n\
If the command times out, then we exit with status 124,\n\
otherwise the normal exit status of the command is returned.\n\
If no signal is specified, the TERM signal is sent. The TERM signal\n\
will kill processes which do not catch this signal. For other processes,\n\
it may be necessary to use the KILL (9) signal, since this signal cannot\n\
be caught.\n"), stdout);
      emit_bug_reporting_address ();
    }
  exit (status);
}

/* Given a long integer value *X, and a suffix character, SUFFIX_CHAR,
   scale *X by the multiplier implied by SUFFIX_CHAR.  SUFFIX_CHAR may
   be the NUL byte or `s' to denote seconds, `m' for minutes, `h' for
   hours, or `d' for days.  If SUFFIX_CHAR is invalid, don't modify *X
   and return false.  If *X would overflow an integer, don't modify *X
   and return false. Otherwise return true.  */

static bool
apply_time_suffix (unsigned long *x, char suffix_char)
{
  unsigned int multiplier = 1;

  switch (suffix_char)
    {
    case 0:
    case 's':
      return true;
    case 'd':
      multiplier *= 24;
    case 'h':
      multiplier *= 60;
    case 'm':
      if (multiplier > UINT_MAX / 60) /* 16 bit overflow */
        return false;
      multiplier *= 60;
      break;
    default:
      return false;
    }

  if (*x > UINT_MAX / multiplier)
    return false;

  *x *= multiplier;

  return true;
}

static void
install_signal_handlers (void)
{
  struct sigaction sa;
  sigemptyset(&sa.sa_mask);  /* Allow concurrent calls to handler */
  sa.sa_handler = cleanup;
  sa.sa_flags = SA_RESTART;  /* restart syscalls (like wait() below) */

  sigaction (SIGALRM, &sa, NULL); /* our timeout.  */
  sigaction (SIGINT, &sa, NULL);  /* Ctrl-C at terminal for example.  */
  sigaction (SIGQUIT, &sa, NULL); /* Ctrl-\ at terminal for example.  */
  sigaction (SIGTERM, &sa, NULL); /* if we're killed, stop monitored proc.  */
  sigaction (SIGHUP, &sa, NULL);  /* terminal closed for example.  */
}

int
main (int argc, char **argv)
{
  unsigned long timeout;
  char signame[SIG2STR_MAX];
  int c;
  char *ep;

  initialize_main (&argc, &argv);
  set_program_name (argv[0]);
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  initialize_exit_failure (EXIT_CANCELED);
  atexit (close_stdout);

  parse_long_options (argc, argv, PROGRAM_NAME, PACKAGE_NAME, Version,
                      usage, AUTHORS, (char const *) NULL);

  while ((c = getopt_long (argc, argv, "+s:", long_options, NULL)) != -1)
    {
      switch (c)
        {
        case 's':
          term_signal = operand2sig (optarg, signame);
          if (term_signal == -1)
            usage (EXIT_CANCELED);
          break;
        default:
          usage (EXIT_CANCELED);
          break;
        }
    }

  if (argc - optind < 2)
    usage (EXIT_CANCELED);

  if (xstrtoul (argv[optind], &ep, 10, &timeout, NULL)
      /* Invalid interval. Note 0 disables timeout  */
      || (timeout > UINT_MAX)
      /* Extra chars after the number and an optional s,m,h,d char.  */
      || (*ep && *(ep + 1))
      /* Check any suffix char and update timeout based on the suffix.  */
      || !apply_time_suffix (&timeout, *ep))
    {
      error (0, 0, _("invalid time interval %s"), quote (argv[optind]));
      usage (EXIT_CANCELED);
    }
  optind++;

  argv += optind;

  /* Ensure we're in our own group so all subprocesses can be killed.
     Note we don't just put the child in a separate group as
     then we would need to worry about foreground and background groups
     and propagating signals between them.  */
  setpgid (0, 0);

  /* Setup handlers before fork() so that we
     handle any signals caused by child, without races.  */
  install_signal_handlers ();
  signal (SIGTTIN, SIG_IGN);    /* don't sTop if background child needs tty.  */
  signal (SIGTTOU, SIG_IGN);    /* don't sTop if background child needs tty.  */

  monitored_pid = fork ();
  if (monitored_pid == -1)
    {
      error (0, errno, _("fork system call failed"));
      return EXIT_CANCELED;
    }
  else if (monitored_pid == 0)
    {                           /* child */
      int exit_status;

      /* exec doesn't reset SIG_IGN -> SIG_DFL.  */
      signal (SIGTTIN, SIG_DFL);
      signal (SIGTTOU, SIG_DFL);

      execvp (argv[0], argv);   /* FIXME: should we use "sh -c" ... here?  */

      /* exit like sh, env, nohup, ...  */
      exit_status = (errno == ENOENT ? EXIT_ENOENT : EXIT_CANNOT_INVOKE);
      error (0, errno, _("failed to run command %s"), quote (argv[0]));
      return exit_status;
    }
  else
    {
      int status;

      alarm (timeout);

      /* We're just waiting for a single process here, so wait() suffices.
         Note the signal() calls above on GNU/Linux and BSD at least,
         essentially call the lower level sigaction() with the SA_RESTART flag
         set, which ensures the following wait call will only return if the
         child exits, not on this process receiving a signal. Also we're not
         passing WUNTRACED | WCONTINUED to a waitpid() call and so will not get
         indication that the child has stopped or continued.  */
      if (wait (&status) == -1)
        {
          /* shouldn't happen.  */
          error (0, errno, _("error waiting for command"));
          status = EXIT_CANCELED;
        }
      else
        {
          if (WIFEXITED (status))
            status = WEXITSTATUS (status);
          else if (WIFSIGNALED (status))
            status = WTERMSIG (status) + 128; /* what sh does at least.  */
          else
            {
              /* shouldn't happen.  */
              error (0, 0, _("unknown status from command (0x%X)"), status);
              status = EXIT_FAILURE;
            }
        }

      if (timed_out)
        return EXIT_TIMEDOUT;
      else
        return status;
    }
}
