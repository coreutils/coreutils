/* su for GNU.  Run a shell with substitute user and group IDs.
   Copyright (C) 1992 Free Software Foundation, Inc.

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
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* Run a shell with the real and effective UID and GID and groups
   of USER, default `root'.

   The shell run is taken from USER's password entry, /bin/sh if
   none is specified there.  If the account has a password, su
   prompts for a password unless run by a user with real UID 0.

   Does not change the current directory.
   Sets `HOME' and `SHELL' from the password entry for USER, and if
   USER is not root, sets `USER' and `LOGNAME' to USER.
   The subshell is not a login shell.

   If one or more ARGs are given, they are passed as additional
   arguments to the subshell.

   Does not handle /bin/sh or other shells specially
   (setting argv[0] to "-su", passing -c only to certain shells, etc.).
   I don't see the point in doing that, and it's ugly.

   This program intentionally does not support a "wheel group" that
   restricts who can su to UID 0 accounts.  RMS considers that to
   be fascist.

   Options:
   -, -l, --login	Make the subshell a login shell.
			Unset all environment variables except
			TERM, HOME and SHELL (set as above), and USER
			and LOGNAME (set unconditionally as above), and
			set PATH to a default value.
			Change to USER's home directory.
			Prepend "-" to the shell's name.
   -c, --commmand=COMMAND
			Pass COMMAND to the subshell with a -c option
			instead of starting an interactive shell.
   -f, --fast		Pass the -f option to the subshell.
   -m, -p, --preserve-environment
			Do not change HOME, USER, LOGNAME, SHELL.
			Run $SHELL instead of USER's shell from /etc/passwd
			unless not the superuser and USER's shell is
			restricted.
			Overridden by --login and --shell.
   -s, --shell=shell	Run SHELL instead of USER's shell from /etc/passwd
			unless not the superuser and USER's shell is
			restricted.

   Compile-time options:
   -DSYSLOG_SUCCESS	Log successful su's (by default, to root) with syslog.
   -DSYSLOG_FAILURE	Log failed su's (by default, to root) with syslog.

   -DSYSLOG_NON_ROOT	Log all su's, not just those to root (UID 0).
   Never logs attempted su's to nonexistent accounts.

   Written by David MacKenzie <djm@gnu.ai.mit.edu>.  */

#ifdef HAVE_CONFIG_H
#if defined (CONFIG_BROKETS)
/* We use <config.h> instead of "config.h" so that a compilation
   using -I. -I$srcdir will use ./config.h rather than $srcdir/config.h
   (which it would do because it found this file in $srcdir).  */
#include <config.h>
#else
#include "config.h"
#endif
#endif

#include <stdio.h>
#include <getopt.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include "system.h"

#ifdef HAVE_SYSLOG_H
#include <syslog.h>
static void log_su ();
#else /* !HAVE_SYSLOG_H */
#ifdef SYSLOG_SUCCESS
#undef SYSLOG_SUCCESS
#endif
#ifdef SYSLOG_FAILURE
#undef SYSLOG_FAILURE
#endif
#ifdef SYSLOG_NON_ROOT
#undef SYSLOG_NON_ROOT
#endif
#endif /* !HAVE_SYSLOG_H */

#ifdef _POSIX_VERSION
#include <limits.h>
#ifdef NGROUPS_MAX
#undef NGROUPS_MAX
#endif
#define NGROUPS_MAX sysconf (_SC_NGROUPS_MAX)
#else /* not _POSIX_VERSION */
struct passwd *getpwuid ();
struct group *getgrgid ();
uid_t getuid ();
#include <sys/param.h>
#if !defined(NGROUPS_MAX) && defined(NGROUPS)
#define NGROUPS_MAX NGROUPS
#endif
#endif /* not _POSIX_VERSION */

#ifdef _POSIX_SOURCE
#define endgrent()
#define endpwent()
#endif

#ifdef HAVE_SHADOW_H
#include <shadow.h>
#endif

#include "version.h"

/* The default PATH for simulated logins to non-superuser accounts.  */
#define DEFAULT_LOGIN_PATH ":/usr/ucb:/bin:/usr/bin"

/* The default PATH for simulated logins to superuser accounts.  */
#define DEFAULT_ROOT_LOGIN_PATH "/usr/ucb:/bin:/usr/bin:/etc"

/* The shell to run if none is given in the user's passwd entry.  */
#define DEFAULT_SHELL "/bin/sh"

/* The user to become if none is specified.  */
#define DEFAULT_USER "root"

char *crypt ();
char *getpass ();
char *getusershell ();
void endusershell ();
void setusershell ();

char *basename ();
char *xmalloc ();
char *xrealloc ();
void error ();

static char *concat ();
static int correct_password ();
static int elements ();
static int restricted_shell ();
static void change_identity ();
static void modify_environment ();
static void run_shell ();
static void usage ();
static void xputenv ();

extern char **environ;

/* The name this program was run with.  */
char *program_name;

/* If non-zero, display usage information and exit.  */
static int show_help;

/* If non-zero, print the version on standard output and exit.  */
static int show_version;

/* If nonzero, pass the `-f' option to the subshell.  */
static int fast_startup;

/* If nonzero, simulate a login instead of just starting a shell.  */
static int simulate_login;

/* If nonzero, change some environment vars to indicate the user su'd to.  */
static int change_environment;

static struct option const longopts[] =
{
  {"command", required_argument, 0, 'c'},
  {"fast", no_argument, &fast_startup, 1},
  {"help", no_argument, &show_help, 1},
  {"login", no_argument, &simulate_login, 1},
  {"preserve-environment", no_argument, &change_environment, 0},
  {"shell", required_argument, 0, 's'},
  {"version", no_argument, &show_version, 1},
  {0, 0, 0, 0}
};

void
main (argc, argv)
     int argc;
     char **argv;
{
  int optc;
  char *new_user = DEFAULT_USER;
  char *command = 0;
  char **additional_args = 0;
  char *shell = 0;
  struct passwd *pw;

  program_name = argv[0];
  fast_startup = 0;
  simulate_login = 0;
  change_environment = 1;

  while ((optc = getopt_long (argc, argv, "c:flmps:", longopts, (int *) 0))
	 != EOF)
    {
      switch (optc)
	{
	case 0:
	  break;

	case 'c':
	  command = optarg;
	  break;

	case 'f':
	  fast_startup = 1;
	  break;

	case 'l':
	  simulate_login = 1;
	  break;

	case 'm':
	case 'p':
	  change_environment = 0;
	  break;

	case 's':
	  shell = optarg;
	  break;

	default:
	  usage (1);
	}
    }

  if (show_version)
    {
      printf ("%s\n", version_string);
      exit (0);
    }

  if (show_help)
    usage (0);

  if (optind < argc && !strcmp (argv[optind], "-"))
    {
      simulate_login = 1;
      ++optind;
    }
  if (optind < argc)
    new_user = argv[optind++];
  if (optind < argc)
    additional_args = argv + optind;

  pw = getpwnam (new_user);
  if (pw == 0)
    error (1, 0, "user %s does not exist", new_user);
  endpwent ();
  if (!correct_password (pw))
    {
#ifdef SYSLOG_FAILURE
      log_su (pw, 0);
#endif
      error (1, 0, "incorrect password");
    }
#ifdef SYSLOG_SUCCESS
  else
    {
      log_su (pw, 1);
    }
#endif

  if (pw->pw_shell == 0 || pw->pw_shell[0] == 0)
    pw->pw_shell = DEFAULT_SHELL;
  if (shell == 0 && change_environment == 0)
    shell = getenv ("SHELL");
  if (shell != 0 && getuid () && restricted_shell (pw->pw_shell))
    {
      /* The user being su'd to has a nonstandard shell, and so is
	 probably a uucp account or has restricted access.  Don't
	 compromise the account by allowing access with a standard
	 shell.  */
      error (0, 0, "using restricted shell %s", pw->pw_shell);
      shell = 0;
    }
  if (shell == 0)
    shell = pw->pw_shell;
  shell = strcpy (xmalloc (strlen (shell) + 1), shell);
  modify_environment (pw, shell);

  change_identity (pw);
  if (simulate_login && chdir (pw->pw_dir))
    error (0, errno, "warning: cannot change directory to %s", pw->pw_dir);
  run_shell (shell, command, additional_args);
}

/* Ask the user for a password.
   Return 1 if the user gives the correct password for entry PW,
   0 if not.  Return 1 without asking for a password if run by UID 0
   or if PW has an empty password.  */

static int
correct_password (pw)
     struct passwd *pw;
{
  char *unencrypted, *encrypted, *correct;
#ifdef HAVE_SHADOW_H
  /* Shadow passwd stuff for SVR3 and maybe other systems.  */
  struct spwd *sp = getspnam (pw->pw_name);

  endspent ();
  if (sp)
    correct = sp->sp_pwdp;
  else
#endif
  correct = pw->pw_passwd;

  if (getuid () == 0 || correct == 0 || correct[0] == '\0')
    return 1;

  unencrypted = getpass ("Password:");
  if (unencrypted == NULL)
    {
      error (0, 0, "getpass: cannot open /dev/tty");
      return 0;
    }
  encrypted = crypt (unencrypted, correct);
  bzero (unencrypted, strlen (unencrypted));
  return strcmp (encrypted, correct) == 0;
}

/* Update `environ' for the new shell based on PW, with SHELL being
   the value for the SHELL environment variable.  */

static void
modify_environment (pw, shell)
     struct passwd *pw;
     char *shell;
{
  char *term;

  if (simulate_login)
    {
      /* Leave TERM unchanged.  Set HOME, SHELL, USER, LOGNAME, PATH.
         Unset all other environment variables.  */
      term = getenv ("TERM");
      environ = (char **) xmalloc (2 * sizeof (char *));
      environ[0] = 0;
      if (term)
	xputenv (concat ("TERM", "=", term));
      xputenv (concat ("HOME", "=", pw->pw_dir));
      xputenv (concat ("SHELL", "=", shell));
      xputenv (concat ("USER", "=", pw->pw_name));
      xputenv (concat ("LOGNAME", "=", pw->pw_name));
      xputenv (concat ("PATH", "=", pw->pw_uid
		       ? DEFAULT_LOGIN_PATH : DEFAULT_ROOT_LOGIN_PATH));
    }
  else
    {
      /* Set HOME, SHELL, and if not becoming a super-user,
	 USER and LOGNAME.  */
      if (change_environment)
	{
	  xputenv (concat ("HOME", "=", pw->pw_dir));
	  xputenv (concat ("SHELL", "=", shell));
	  if (pw->pw_uid)
	    {
	      xputenv (concat ("USER", "=", pw->pw_name));
	      xputenv (concat ("LOGNAME", "=", pw->pw_name));
	    }
	}
    }
}

/* Become the user and group(s) specified by PW.  */

static void
change_identity (pw)
     struct passwd *pw;
{
#ifdef NGROUPS_MAX
  errno = 0;
  if (initgroups (pw->pw_name, pw->pw_gid) == -1)
    error (1, errno, "cannot set groups");
  endgrent ();
#endif
  if (setgid (pw->pw_gid))
    error (1, errno, "cannot set group id");
  if (setuid (pw->pw_uid))
    error (1, errno, "cannot set user id");
}

/* Run SHELL, or DEFAULT_SHELL if SHELL is empty.
   If COMMAND is nonzero, pass it to the shell with the -c option.
   If ADDITIONAL_ARGS is nonzero, pass it to the shell as more
   arguments.  */

static void
run_shell (shell, command, additional_args)
     char *shell;
     char *command;
     char **additional_args;
{
  char **args;
  int argno = 1;

  if (additional_args)
    args = (char **) xmalloc (sizeof (char *)
			      * (10 + elements (additional_args)));
  else
    args = (char **) xmalloc (sizeof (char *) * 10);
  if (simulate_login)
    {
      args[0] = xmalloc (strlen (shell) + 2);
      args[0][0] = '-';
      strcpy (args[0] + 1, basename (shell));
    }
  else
    args[0] = basename (shell);
  if (fast_startup)
    args[argno++] = "-f";
  if (command)
    {
      args[argno++] = "-c";
      args[argno++] = command;
    }
  if (additional_args)
    for (; *additional_args; ++additional_args)
      args[argno++] = *additional_args;
  args[argno] = 0;
  execv (shell, args);
  error (1, errno, "cannot run %s", shell);
}

#if defined (SYSLOG_SUCCESS) || defined (SYSLOG_FAILURE)
/* Log the fact that someone has run su to the user given by PW;
   if SUCCESSFUL is nonzero, they gave the correct password, etc.  */

static void
log_su (pw, successful)
     struct passwd *pw;
     int successful;
{
  char *new_user, *old_user, *tty;

#ifndef SYSLOG_NON_ROOT
  if (pw->pw_uid)
    return;
#endif
  new_user = pw->pw_name;
  /* The utmp entry (via getlogin) is probably the best way to identify
     the user, especially if someone su's from a su-shell.  */
  old_user = getlogin ();
  if (old_user == 0)
    old_user = "";
  tty = ttyname (2);
  if (tty == 0)
    tty = "";
  /* 4.2BSD openlog doesn't have the third parameter.  */
  openlog (basename (program_name), 0
#ifdef LOG_AUTH
	   , LOG_AUTH
#endif
	   );
  syslog (LOG_NOTICE,
#ifdef SYSLOG_NON_ROOT
	  "%s(to %s) %s on %s",
#else
	  "%s%s on %s",
#endif
	  successful ? "" : "FAILED SU ",
#ifdef SYSLOG_NON_ROOT
	  new_user,
#endif
	  old_user, tty);
  closelog ();
}
#endif

/* Return 1 if SHELL is a restricted shell (one not returned by
   getusershell), else 0, meaning it is a standard shell.  */

static int
restricted_shell (shell)
     char *shell;
{
  char *line;

  setusershell ();
  while ((line = getusershell ()) != NULL)
    {
      if (*line != '#' && strcmp (line, shell) == 0)
	{
	  endusershell ();
	  return 0;
	}
    }
  endusershell ();
  return 1;
}

/* Return the number of elements in ARR, a null-terminated array.  */

static int
elements (arr)
     char **arr;
{
  int n = 0;

  for (n = 0; *arr; ++arr)
    ++n;
  return n;
}

/* Add VAL to the environment, checking for out of memory errors.  */

static void
xputenv (val)
     char *val;
{
  if (putenv (val))
    error (1, 0, "virtual memory exhausted");
}

/* Return a newly-allocated string whose contents concatenate
   those of S1, S2, S3.  */

static char *
concat (s1, s2, s3)
     char *s1, *s2, *s3;
{
  int len1 = strlen (s1), len2 = strlen (s2), len3 = strlen (s3);
  char *result = (char *) xmalloc (len1 + len2 + len3 + 1);

  strcpy (result, s1);
  strcpy (result + len1, s2);
  strcpy (result + len1 + len2, s3);
  result[len1 + len2 + len3] = 0;

  return result;
}

static void
usage (status)
     int status;
{
  if (status != 0)
    fprintf (stderr, "Try `%s --help' for more information.\n",
	     program_name);
  else
    {
      printf ("Usage: %s [OPTION]... [-] [USER [ARG]...]\n", program_name);
      printf ("\
\n\
  -l, --login                  make the shell a login shell\n\
  -c, --commmand=COMMAND       pass a single COMMAND to the shell with -c\n\
  -f, --fast                   pass -f to the shell (for csh or tcsh)\n\
  -m, --preserve-environment   do not reset environment variables\n\
  -p                           same as -m\n\
  -s, --shell=SHELL            run SHELL if /etc/shells allows it\n\
      --help                   display this help and exit\n\
      --version                output version information and exit\n\
\n\
A mere - implies -l.   If USER not given, assume root.\n\
");
    }
  exit (status);
}
