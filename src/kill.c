/* kill -- send a signal to a process
   Copyright (C) 2001 Free Software Foundation, Inc.

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

/* Written by Marcus Brinkmann.  */

#include <config.h>
#include <stdio.h>
#include <getopt.h>
#include <sys/types.h>

#include <signal.h>

#include "system.h"
#include "closeout.h"
#include "human.h"
#include "error.h"
#include "xstrtol.h"

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "kill"

#define AUTHORS "Marcus Brinkmann"


/* An invalid signal number.  */
#define NO_SIG -1

/* A structure holding the number and the name of a signal.  */
struct sigspec
{
  int signum;
  char *signame;
};

/* The list of signals was taken from bash 2.05.  The best name for a
   signal comes after any possible alias, so it is read it from back
   to front.  This is why the terminating null entry comes first.  */
static struct sigspec sigspecs[] =
{
  { NO_SIG, NULL },

  /* Null is used to test for the existance and ownership of a PID.  */
  { 0, "0" },

/* AIX */
#if defined (SIGLOST)	/* resource lost (eg, record-lock lost) */
  { SIGLOST, "LOST" },
#endif

#if defined (SIGMSG)	/* HFT input data pending */
  { SIGMSG, "MSG" },
#endif

#if defined (SIGDANGER)	/* system crash imminent */
  { SIGDANGER, "DANGER" },
#endif

#if defined (SIGMIGRATE) /* migrate process to another CPU */
  { SIGMIGRATE, "MIGRATE" },
#endif

#if defined (SIGPRE)	/* programming error */
  { SIGPRE, "PRE" },
#endif

#if defined (SIGVIRT)	/* AIX virtual time alarm */
  { SIGVIRT, "VIRT" },
#endif

#if defined (SIGALRM1)	/* m:n condition variables */
  { SIGALRM1, "ALRM1" },
#endif

#if defined (SIGWAITING)	/* m:n scheduling */
  { SIGWAITING, "WAITING" },
#endif

#if defined (SIGGRANT)	/* HFT monitor mode granted */
  { SIGGRANT, "GRANT" },
#endif

#if defined (SIGKAP)	/* keep alive poll from native keyboard */
  { SIGKAP, "KAP" },
#endif

#if defined (SIGRETRACT) /* HFT monitor mode retracted */
  { SIGRETRACT, "RETRACT" },
#endif

#if defined (SIGSOUND)	/* HFT sound sequence has completed */
  { SIGSOUND, "SOUND" },
#endif

#if defined (SIGSAK)	/* Secure Attention Key */
  { SIGSAK, "SAK" },
#endif

/* SunOS5 */
#if defined (SIGLWP)	/* special signal used by thread library */
  { SIGLWP, "LWP" },
#endif

#if defined (SIGFREEZE)	/* special signal used by CPR */
  { SIGFREEZE, "FREEZE" },
#endif

#if defined (SIGTHAW)	/* special signal used by CPR */
  { SIGTHAW, "THAW" },
#endif

#if defined (SIGCANCEL)	/* thread cancellation signal used by libthread */
  { SIGCANCEL, "CANCEL" },
#endif

/* HP-UX */
#if defined (SIGDIL)	/* DIL signal (?) */
  { SIGDIL, "DIL" },
#endif

/* System V */
#if defined (SIGCLD)	/* Like SIGCHLD.  */
  { SIGCLD, "CLD" },
#endif

#if defined (SIGPWR)	/* power state indication */
  { SIGPWR, "PWR" },
#endif

#if defined (SIGPOLL)	/* Pollable event (for streams)  */
  { SIGPOLL, "POLL" },
#endif

/* Unknown */
#if defined (SIGWINDOW)
  { SIGWINDOW, "WINDOW" },
#endif

/* Common */
#if defined (SIGHUP)	/* hangup */
  { SIGHUP, "HUP" },
#endif

#if defined (SIGINT)	/* interrupt */
  { SIGINT, "INT" },
#endif

#if defined (SIGQUIT)	/* quit */
  { SIGQUIT, "QUIT" },
#endif

#if defined (SIGILL)	/* illegal instruction (not reset when caught) */
  { SIGILL, "ILL" },
#endif

#if defined (SIGTRAP)	/* trace trap (not reset when caught) */
  { SIGTRAP, "TRAP" },
#endif

#if defined (SIGIOT)	/* IOT instruction */
  { SIGIOT, "IOT" },
#endif

#if defined (SIGABRT)	/* Cause current process to dump core. */
  { SIGABRT, "ABRT" },
#endif

#if defined (SIGEMT)	/* EMT instruction */
  { SIGEMT, "EMT" },
#endif

#if defined (SIGFPE)	/* floating point exception */
  { SIGFPE, "FPE" },
#endif

#if defined (SIGKILL)	/* kill (cannot be caught or ignored) */
  { SIGKILL, "KILL" },
#endif

#if defined (SIGBUS)	/* bus error */
  { SIGBUS, "BUS" },
#endif

#if defined (SIGSEGV)	/* segmentation violation */
  { SIGSEGV, "SEGV" },
#endif

#if defined (SIGSYS)	/* bad argument to system call */
  { SIGSYS, "SYS" },
#endif

#if defined (SIGPIPE)	/* write on a pipe with no one to read it */
  { SIGPIPE, "PIPE" },
#endif

#if defined (SIGALRM)	/* alarm clock */
  { SIGALRM, "ALRM" },
#endif

#if defined (SIGTERM)	/* software termination signal from kill */
  { SIGTERM, "TERM" },
#endif

#if defined (SIGURG)	/* urgent condition on IO channel */
  { SIGURG, "URG" },
#endif

#if defined (SIGSTOP)	/* sendable stop signal not from tty */
  { SIGSTOP, "STOP" },
#endif

#if defined (SIGTSTP)	/* stop signal from tty */
  { SIGTSTP, "TSTP" },
#endif

#if defined (SIGCONT)	/* continue a stopped process */
  { SIGCONT, "CONT" },
#endif

#if defined (SIGCHLD)	/* to parent on child stop or exit */
  { SIGCHLD, "CHLD" },
#endif

#if defined (SIGTTIN)	/* to readers pgrp upon background tty read */
  { SIGTTIN, "TTIN" },
#endif

#if defined (SIGTTOU)	/* like TTIN for output if (tp->t_local&LTOSTOP) */
  { SIGTTOU, "TTOU" },
#endif

#if defined (SIGIO)	/* input/output possible signal */
  { SIGIO, "IO" },
#endif

#if defined (SIGXCPU)	/* exceeded CPU time limit */
  { SIGXCPU, "XCPU" },
#endif

#if defined (SIGXFSZ)	/* exceeded file size limit */
  { SIGXFSZ, "XFSZ" },
#endif

#if defined (SIGVTALRM)	/* virtual time alarm */
  { SIGVTALRM, "VTALRM" },
#endif

#if defined (SIGPROF)	/* profiling time alarm */
  { SIGPROF, "PROF" },
#endif

#if defined (SIGWINCH)	/* window changed */
  { SIGWINCH, "WINCH" },
#endif

/* 4.4 BSD */
#if defined (SIGINFO) && !defined (_SEQUENT_)	/* information request */
  { SIGINFO, "INFO" },
#endif

#if defined (SIGUSR1)	/* user defined signal 1 */
  { SIGUSR1, "USR1" },
#endif

#if defined (SIGUSR2)	/* user defined signal 2 */
  { SIGUSR2, "USR2" },
#endif

#if defined (SIGKILLTHR)	/* BeOS: Kill Thread */
  { SIGKILLTHR, "KILLTHR" }
#endif
};

/* The last entry of the complete signal list, including real time
   signals.  */
struct sigspec *sigspecs_last;

/* The number of sigspecs in the list.  */
int sigspecs_size;

/* The number of digits in NSIG (approx.).  */
int nsig_digits;


/* The name this program was run with, for error messages.  */
char *program_name;

/* All options which require an argument are known to the
   pre-scan loop in main().  */
#define OPT_SIGSPEC_LONG "sigspec"
#define OPT_SIGNUM_LONG "signum"

static struct option const long_options[] =
{
  {"list", no_argument, NULL, 'l'},
  {"long-list", no_argument, NULL, 'L'},
  {OPT_SIGSPEC_LONG, required_argument, NULL, 's'},
  {OPT_SIGNUM_LONG, required_argument, NULL, 'n'},
  {GETOPT_HELP_OPTION_DECL},
  {GETOPT_VERSION_OPTION_DECL},
  {NULL, 0, NULL, 0}
};

void
usage (int status)
{
  if (status != 0)
    fprintf (stderr, _("Try `%s --help' for more information.\n"),
	     program_name);
  else
    {
      printf (_("\
Usage: %s [-s SIGSPEC | -n SIGNUM | -SIGSPEC] PID ...\n\
  or:  %s -l [SIGSPEC] ...\n\
"),
	      program_name, program_name);
      fputs (_("\
Send the signal named by SIGSPEC or SIGNUM to processes named by PID.\n\
\n\
  -s, --" OPT_SIGSPEC_LONG " SIGSPEC     name or number of signal to be sent\n\
  -n, --" OPT_SIGNUM_LONG " SIGNUM       number of signal to be sent\n\
\n\
  -l, --list                list the signal names\n\
  -L, --long-list           list the signal names with their numbers\n\
\n\
"), stdout);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      fputs (_("\
\n\
kill returns true if at least one signal was successfully sent, or\n\
false if an error occurs or an invalid option is encountered.\n\
"), stdout);
      puts (_("\nReport bugs to <bug-sh-utils@gnu.org>."));
    }
  exit (status);
}


/* The function is called once before option parsing.  It calculates
   the maximum number of digits in a signum, adds the realtime signal
   specs to the signal list, and adds all signals of the form -SIGTERM
   and -TERM as possible options.  */
static void
initialize (void)
{
  char number[LONGEST_HUMAN_READABLE + 1];
  struct sigspec *newspecs;

  nsig_digits = strlen (human_readable ((uintmax_t) NSIG, number, 1, 1));

  sigspecs_size = sizeof (sigspecs) / sizeof (struct sigspec);

#if defined (SIGRTMIN) || defined (SIGRTMAX)
  /* POSIX 1003.1b-1993 defines real time signals.  The following code
     is so convoluted because it also takes care of incomplete
     implementations.  */
# ifndef SIGRTMIN
#  define SIGRTMIN SIGRTMAX
# endif
# ifndef SIGRTMAX
#  define SIGRTMAX SIGRTMIN
# endif

  /* Sanity check.  */
  if (SIGRTMAX >= SIGRTMIN)
    {
      int rtsigc = SIGRTMAX - SIGRTMIN + 1;
      int i;
      /* Account for "RTMIN+" resp "RTMAX-", the number and '\0'.  */
      int maxlength = nsig_digits + 6 + 1;

      newspecs = xmalloc (sizeof (struct sigspec)
			  * (sigspecs_size + rtsigc));

      /* After this, newspecs will always point to the last element of
	 the array.  */
      newspecs->signum = NO_SIG;
      newspecs->signame = NULL;

      (++newspecs)->signum = SIGRTMAX;
      newspecs->signame = "RTMAX";
      if (rtsigc > 1)
	{
	  (++newspecs)->signum = SIGRTMIN;
	  newspecs->signame = "RTMIN";
	}

      /* Create new elements for all missing realtime signals.  */
      for (i = 0; i < rtsigc - 2; i++)
	{
	  (++newspecs)->signum = SIGRTMIN + 1 + i;
	  newspecs->signame = xmalloc (maxlength);

	  snprintf (newspecs->signame, maxlength, "%s%d",
		    (i < (rtsigc - 2)/2
		     ? "RTMIN+" : "RTMAX-"),
		    (i < (rtsigc - 2)/2
		     ? i + 1 : (rtsigc - 2) - i));
	}

      /* Copy the existing elements in the following space.  */
      for (i = 1; i < sigspecs_size; i++)
	*(++newspecs) = sigspecs[i];

      sigspecs_last = newspecs;
      sigspecs_size += rtsigc;
    }
#else
  sigspecs_last = sigspecs + (sigspecs_size - 1);
#endif
}


typedef enum { LIST_NONE, LIST_FLAT, LIST_PRETTY } list_t;

/* Print out a table listing all signals specifications with their
   preferred name.  */
static void
list_signals (list_t type)
{
  int i = 0;
  int entrylen = 0;
  int unsorted;
  int entries;
  int last_signum = NO_SIG;
  int column = 0;
  struct sigspec *spec = sigspecs_last;
  struct sigspec **specs = xmalloc (sizeof (struct sigspec *)
				    * (sigspecs_size - 1));


  /* Gather maximum name length and prepare sort array.  Note that the
     list is reversed in the array.  This is taken into account by the
     output routine below.  */
  while (spec->signum != NO_SIG)
    {
      specs[i++] = spec;
      if (spec->signame && strlen (spec->signame) > entrylen)
	entrylen = strlen (spec->signame);
      spec--;
    }

  /* Sort the array by signal number.  This is a simple bubble sort,
     but the point is that the order of entries with the same signum
     is presevered (otherwise the preferred alias is lost).  */
  if (sigspecs_size > 2)
    do
      {
	unsorted = 0;

	for (i = 0; i < sigspecs_size - 2; i++)
	  {
	    if (specs[i]->signum > specs[i+1]->signum)
	      {
		struct sigspec *saved = specs[i];
		specs[i] = specs[i+1];
		specs[i+1] = saved;
		unsorted = 1;
	      }
	  }
      }
    while (unsorted);

  /* Account for "NR) NAME ".  Calculate for 79 columns, the 80 takes
     into account that the last entry is not followed by a space.  */
  entrylen += nsig_digits + 2 + 1;
  entries = 80 / entrylen;
  if (entries < 1)
    entries = 1;

  for (i = 0; i < sigspecs_size - 1; i++)
    {
      /* Skip duplicated signal numbers, signums without name and the
	 special signal number `0'.  */
      if (specs[i]->signame && specs[i]->signum
	  && (last_signum == NO_SIG || last_signum != specs[i]->signum))
	{
	  switch (type)
	    {
	    case LIST_PRETTY:
	      column++;
	      printf ("%*i) %-*s", nsig_digits, specs[i]->signum,
		      entrylen - nsig_digits - 2 + (column != entries ? 1 : 0),
		      specs[i]->signame);
	      if (column == entries - 1)
		{
		  column = 0;
		  putchar ('\n');
		}
	      break;
	    case LIST_FLAT:
	      column += printf ("%s%s",
				(column == 0 ? ""
				 : (column + strlen (specs[i]->signame) > 78
				    ? "\n" : " ")),
				specs[i]->signame);
	      if (column > 79)
		column = strlen (specs[i]->signame);
	      break;
	    default:
	      break;
	    }
	  last_signum = specs[i]->signum;
	}
      if (i == sigspecs_size - 2)
	putchar ('\n');
    }
}


/* Turn a string into a signal number, or a number into a signal
   number.  If STRING is "2", or "INT", then return the integer 2.
   Return NO_SIG if STRING doesn't contain a valid signal
   descriptor.  */
static int
decode_signal (char const *sigspec)
{
  struct sigspec const *spec = sigspecs_last;
  long l;

  if (xstrtol (sigspec, NULL, 0, &l, "") == LONGINT_OK)
    {
      int sig = l;

      if (sig != l)
	return NO_SIG;

      while (spec->signum != NO_SIG && spec->signum != sig)
	spec--;
      return spec->signum;
    }

  /* A leading `SIG' may be omitted. */
  while (spec->signum != NO_SIG)
    {
      if (spec->signame
	  && (strcasecmp (sigspec, spec->signame) == 0
	      || (strncasecmp (sigspec, "SIG", 3) == 0
		  && strcasecmp (sigspec + 3, spec->signame) == 0)))
	return (spec->signum);
      spec--;
    }
  return NO_SIG;
}

/* Find the preferred name for the signal with the number SIGNUM.  */
static char const *const
name_signal (int signum)
{
  struct sigspec const *spec = sigspecs_last;

  while (spec->signum != NO_SIG && spec->signum != signum)
    spec--;
  return spec->signame;
}

/* Send the signal SIGNUM to process PID, using kill ().  If an error
   occurs, it is reported and passed through to the caller.  */
static int
send_signal (pid_t pid, int signum)
{
  int err;

  err = kill (pid, signum);
  if (err)
    {
      uintmax_t nr = (uintmax_t) (pid < 0 ? -pid : pid);
      char number[LONGEST_HUMAN_READABLE + 1];

      error (0, errno, "(%s%s)", (pid < 0 ? "-" : ""),
	     human_readable (nr, number, 1, 1));
    }
  return err;
}


int
main (int argc, char **argv)
{
  int optc;
  int err = 0;
  int success = 0;
  list_t list = LIST_NONE;
  int sig_num = NO_SIG;
  int i;
  char **extra_opt;
  int extra_opt_size = 0;

  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  initialize ();

  extra_opt = xmalloc ((argc - 1) * sizeof (char *));
  for (i = 1; i < argc; i++)
    {
      intmax_t dummy;

      /* getopt will ignore everything following `--', so we do as
         well.  */
      if (!strcmp ("--", argv[i]))
	break;

      /* Skip this argument if it doesn't look like an option.  */
      if (argv[i][0] != '-')
	continue;

      /* Skip it if it follows an option requiring an argument.  */
      if (i > 1 && argv[i-1][0] == '-')
	{
	  /* A short option that doesn't look like -nTERM.  */
	  if ((argv[i-1][1] == 'n' || argv[i-1][1] == 's')
	      && argv[i-1][2] == '\0')
	    continue;

	  /* A long option (not `--', which was excluded above).  */
	  if (argv[i-1][1] == '-'
	      && (!strncmp (OPT_SIGNUM_LONG, &(argv[i-1][2]),
			    strlen (&argv[i-1][2]))
		  || !strncmp (OPT_SIGSPEC_LONG, &(argv[i-1][2]),
			       strlen (&argv[i-1][2]))))
	    continue;
	}

      /* At this point we know that this argument is not argument to
	 another option, and that it starts with `-' but is not `--'.  */

      if (decode_signal (&(argv[i][1])) != NO_SIG
	  || xstrtoimax (argv[i], NULL, 10, &dummy, "") == LONGINT_OK)
	{
	  /* It is either a valid signal specifier or potentially a
	     valid process group.  So remember it and make getopt not
	     care about it.  */
	  extra_opt[extra_opt_size++] = argv[i];
	  argv[i][0] = 'X';
	}
    }

  while ((optc = getopt_long (argc, argv, "s:n:lL", long_options, NULL))
	 != -1)
    switch (optc)
      {
      case 's':
      case 'n':
	if (sig_num != NO_SIG)
	  {
	    error (0, 0, _("%s: only one signal specififier allowed"), optarg);
	    usage (1);
	  }
	sig_num = decode_signal (optarg);
	if (sig_num == NO_SIG)
	  error (1, 0, _("%s: invalid signal specifier"), optarg);
	break;
      case 'l':
	list = LIST_FLAT;
	break;
      case 'L':
	list = LIST_PRETTY;
	break;
      case_GETOPT_HELP_CHAR;
      case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);
      default:
	usage (1);
      }

  argc -= optind;
  argv += optind;

  for (i = 0; i < extra_opt_size; i++)
    extra_opt[i][0] = '-';

  if (extra_opt_size > 0 && sig_num == NO_SIG)
    {
      char **arg = argv;

      /* Find the first extra option collected in the remaining
	 argument list and if necessary, replace it with the first
	 remaining argument.  This is a precaution in case getopt()
	 mangles the order of non-option arguments.  */

      while (*arg && *arg != extra_opt[0])
	arg++;
      if (*arg && *arg != argv[0])
	*arg = argv[0];

      argc--;
      argv++;

      /* Interpret the first one as a signal specifier.  */
      sig_num = decode_signal (&(extra_opt[0][1]));
      if (sig_num == NO_SIG)
	error (1, 0, _("%s: invalid signal specifier"), extra_opt[0]);
    }

  if (!list && sig_num == NO_SIG)
    sig_num = SIGTERM;

  if (argc == 0)
    {
      if (list)
	list_signals (list);
      else
	{
	  error (0, 0, _("too few arguments"));
	  usage (1);
	}
    }
  else
    {
      int j;
      for (j = 0; j < argc; j++)
	if (list)
	  {
	    int signum = decode_signal (argv[j]);
	    if (signum == NO_SIG)
	      {
		error (0, 0, _("%s: invalid signal specifier"), argv[j]);
		err = 1;
	      }
	    else
	      {
		char const *const name = name_signal (signum);
		printf ("%s\n", name ? name : "(unknown)");
		success = 1;
	      }
	  }
	else
	  {
	    intmax_t nr;
	    pid_t pid;
	    int inval = 0;

	    if (xstrtoimax (argv[j], NULL, 10, &nr, "") != LONGINT_OK)
	      inval = 1;
	    pid = (pid_t) nr;
	    if (inval || pid != nr)
	      {
		error (0, 0, _("%s: invalid process id"), argv[j]);
		err = 1;
		continue;
	      }
	    if (send_signal (pid, sig_num))
	      err = 1;
	    else
	      success = 1;
	  }
    }

  exit ((success || !err) ? 0 : 1);
}
