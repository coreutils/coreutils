/* su for GNU.  Run a shell with substitute user and group IDs.
   Copyright (C) 1992-2002 Free Software Foundation, Inc.

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

#include <config.h>
#include <stdio.h>
#include <getopt.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>

/* Hide any system prototype for getusershell.
   This is necessary because some Cray systems have a conflicting
   prototype (returning `int') in <unistd.h>.  */
#define getusershell _getusershell_sys_proto_

#include "system.h"
#include "closeout.h"
#include "dirname.h"

#undef getusershell

#if HAVE_SYSLOG_H && HAVE_SYSLOG
# include <syslog.h>
#else
# undef SYSLOG_SUCCESS
# undef SYSLOG_FAILURE
# undef SYSLOG_NON_ROOT
#endif

#if HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif

#ifndef HAVE_ENDGRENT
# define endgrent() ((void) 0)
#endif

#ifndef HAVE_ENDPWENT
# define endpwent() ((void) 0)
#endif

#if HAVE_SHADOW_H
# include <shadow.h>
#endif

#include "error.h"

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "su"

#define AUTHORS "David MacKenzie"

#if HAVE_PATHS_H
# include <paths.h>
#endif

/* The default PATH for simulated logins to non-superuser accounts.  */
#ifdef _PATH_DEFPATH
# define DEFAULT_LOGIN_PATH _PATH_DEFPATH
#else
# define DEFAULT_LOGIN_PATH ":/usr/ucb:/bin:/usr/bin"
#endif

/* The default PATH for simulated logins to superuser accounts.  */
#ifdef _PATH_DEFPATH_ROOT
# define DEFAULT_ROOT_LOGIN_PATH _PATH_DEFPATH_ROOT
#else
# define DEFAULT_ROOT_LOGIN_PATH "/usr/ucb:/bin:/usr/bin:/etc"
#endif

/* The shell to run if none is given in the user's passwd entry.  */
#define DEFAULT_SHELL "/bin/sh"

/* The user to become if none is specified.  */
#define DEFAULT_USER "root"

char *crypt ();
char *getpass ();
char *getusershell ();
void endusershell ();
void setusershell ();

extern char **environ;

static void run_shell (const char *, const char *, char **)
     ATTRIBUTE_NORETURN;

/* The name this program was run with.  */
char *program_name;

/* If nonzero, pass the `-f' option to the subshell.  */
static int fast_startup;

/* If nonzero, simulate a login instead of just starting a shell.  */
static int simulate_login;

/* If nonzero, change some environment vars to indicate the user su'd to.  */
static int change_environment;

static struct option const longopts[] =
{
  {"command", required_argument, 0, 'c'},
  {"fast", no_argument, NULL, 'f'},
  {"login", no_argument, NULL, 'l'},
  {"preserve-environment", no_argument, &change_environment, 0},
  {"shell", required_argument, 0, 's'},
  {GETOPT_HELP_OPTION_DECL},
  {GETOPT_VERSION_OPTION_DECL},
  {0, 0, 0, 0}
};

/* Add VAL to the environment, checking for out of memory errors.  */

static void
xputenv (char *val)
{
  if (putenv (val))
    xalloc_die ();
}

/* Return a newly-allocated string whose contents concatenate
   those of S1, S2, S3.  */

static char *
concat (const char *s1, const char *s2, const char *s3)
{
  int len1 = strlen (s1), len2 = strlen (s2), len3 = strlen (s3);
  char *result = (char *) xmalloc (len1 + len2 + len3 + 1);

  strcpy (result, s1);
  strcpy (result + len1, s2);
  strcpy (result + len1 + len2, s3);
  result[len1 + len2 + len3] = 0;

  return result;
}

/* Return the number of elements in ARR, a null-terminated array.  */

static int
elements (char **arr)
{
  int n = 0;

  for (n = 0; *arr; ++arr)
    ++n;
  return n;
}

#if defined (SYSLOG_SUCCESS) || defined (SYSLOG_FAILURE)
/* Log the fact that someone has run su to the user given by PW;
   if SUCCESSFUL is nonzero, they gave the correct password, etc.  */

static void
log_su (const struct passwd *pw, int successful)
{
  const char *new_user, *old_user, *tty;

# ifndef SYSLOG_NON_ROOT
  if (pw->pw_uid)
    return;
# endif
  new_user = pw->pw_name;
  /* The utmp entry (via getlogin) is probably the best way to identify
     the user, especially if someone su's from a su-shell.  */
  old_user = getlogin ();
  if (old_user == NULL)
    {
      /* getlogin can fail -- usually due to lack of utmp entry.
	 Resort to getpwuid.  */
      struct passwd *pwd = getpwuid (getuid ());
      old_user = (pwd ? pwd->pw_name : "");
    }
  tty = ttyname (2);
  if (tty == NULL)
    tty = "none";
  /* 4.2BSD openlog doesn't have the third parameter.  */
  openlog (base_name (program_name), 0
# ifdef LOG_AUTH
	   , LOG_AUTH
# endif
	   );
  syslog (LOG_NOTICE,
# ifdef SYSLOG_NON_ROOT
	  "%s(to %s) %s on %s",
# else
	  "%s%s on %s",
# endif
	  successful ? "" : "FAILED SU ",
# ifdef SYSLOG_NON_ROOT
	  new_user,
# endif
	  old_user, tty);
  closelog ();
}
#endif

/* Ask the user for a password.
   Return 1 if the user gives the correct password for entry PW,
   0 if not.  Return 1 without asking for a password if run by UID 0
   or if PW has an empty password.  */

static int
correct_password (const struct passwd *pw)
{
  char *unencrypted, *encrypted, *correct;
#if HAVE_GETSPNAM && HAVE_STRUCT_SPWD_SP_PWDP
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

  unencrypted = getpass (_("Password:"));
  if (unencrypted == NULL)
    {
      error (0, 0, _("getpass: cannot open /dev/tty"));
      return 0;
    }
  encrypted = crypt (unencrypted, correct);
  memset (unencrypted, 0, strlen (unencrypted));
  return strcmp (encrypted, correct) == 0;
}

/* Update `environ' for the new shell based on PW, with SHELL being
   the value for the SHELL environment variable.  */

static void
modify_environment (const struct passwd *pw, const char *shell)
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
      xputenv (concat ("PATH", "=", (pw->pw_uid
				     ? DEFAULT_LOGIN_PATH
				     : DEFAULT_ROOT_LOGIN_PATH)));
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
change_identity (const struct passwd *pw)
{
#ifdef HAVE_INITGROUPS
  errno = 0;
  if (initgroups (pw->pw_name, pw->pw_gid) == -1)
    error (EXIT_FAILURE, errno, _("cannot set groups"));
  endgrent ();
#endif
  if (setgid (pw->pw_gid))
    error (EXIT_FAILURE, errno, _("cannot set group id"));
  if (setuid (pw->pw_uid))
    error (EXIT_FAILURE, errno, _("cannot set user id"));
}

/* Run SHELL, or DEFAULT_SHELL if SHELL is empty.
   If COMMAND is nonzero, pass it to the shell with the -c option.
   If ADDITIONAL_ARGS is nonzero, pass it to the shell as more
   arguments.  */

static void
run_shell (const char *shell, const char *command, char **additional_args)
{
  const char **args;
  int argno = 1;

  if (additional_args)
    args = (const char **) xmalloc (sizeof (char *)
				    * (10 + elements (additional_args)));
  else
    args = (const char **) xmalloc (sizeof (char *) * 10);
  if (simulate_login)
    {
      char *arg0;
      char *shell_basename;

      shell_basename = base_name (shell);
      arg0 = xmalloc (strlen (shell_basename) + 2);
      arg0[0] = '-';
      strcpy (arg0 + 1, shell_basename);
      args[0] = arg0;
    }
  else
    args[0] = base_name (shell);
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
  args[argno] = NULL;
  execv (shell, (char **) args);

  {
    int exit_status = (errno == ENOENT ? 127 : 126);
    error (0, errno, "%s", shell);
    exit (exit_status);
  }
}

/* Return 1 if SHELL is a restricted shell (one not returned by
   getusershell), else 0, meaning it is a standard shell.  */

static int
restricted_shell (const char *shell)
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

void
usage (int status)
{
  if (status != 0)
    fprintf (stderr, _("Try `%s --help' for more information.\n"),
	     program_name);
  else
    {
      printf (_("Usage: %s [OPTION]... [-] [USER [ARG]...]\n"), program_name);
      fputs (_("\
Change the effective user id and group id to that of USER.\n\
\n\
  -, -l, --login               make the shell a login shell\n\
  -c, --commmand=COMMAND       pass a single COMMAND to the shell with -c\n\
  -f, --fast                   pass -f to the shell (for csh or tcsh)\n\
  -m, --preserve-environment   do not reset environment variables\n\
  -p                           same as -m\n\
  -s, --shell=SHELL            run SHELL if /etc/shells allows it\n\
"), stdout);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      fputs (_("\
\n\
A mere - implies -l.   If USER not given, assume root.\n\
"), stdout);
      printf (_("\nReport bugs to <%s>.\n"), PACKAGE_BUGREPORT);
      close_stdout ();
    }
  exit (status);
}

int
main (int argc, char **argv)
{
  int optc;
  const char *new_user = DEFAULT_USER;
  char *command = 0;
  char **additional_args = 0;
  char *shell = 0;
  struct passwd *pw;
  struct passwd pw_copy;

  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  fast_startup = 0;
  simulate_login = 0;
  change_environment = 1;

  while ((optc = getopt_long (argc, argv, "c:flmps:", longopts, NULL)) != -1)
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

	case_GETOPT_HELP_CHAR;

	case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);

	default:
	  usage (EXIT_FAILURE);
	}
    }

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
    error (EXIT_FAILURE, 0, _("user %s does not exist"), new_user);
  endpwent ();

  /* Make sure pw->pw_shell is non-NULL.  It may be NULL when NEW_USER
     is a username that is retrieved via NIS (YP), but that doesn't have
     a default shell listed.  */
  if (pw->pw_shell == NULL || pw->pw_shell[0] == '\0')
    pw->pw_shell = (char *) DEFAULT_SHELL;

  /* Make a copy of the password information and point pw at the local
     copy instead.  Otherwise, some systems (e.g. Linux) would clobber
     the static data through the getlogin call from log_su.  */
  pw_copy = *pw;
  pw = &pw_copy;
  pw->pw_name = xstrdup (pw->pw_name);
  pw->pw_dir = xstrdup (pw->pw_dir);
  pw->pw_shell = xstrdup (pw->pw_shell);

  if (!correct_password (pw))
    {
#ifdef SYSLOG_FAILURE
      log_su (pw, 0);
#endif
      error (EXIT_FAILURE, 0, _("incorrect password"));
    }
#ifdef SYSLOG_SUCCESS
  else
    {
      log_su (pw, 1);
    }
#endif

  if (shell == 0 && change_environment == 0)
    shell = getenv ("SHELL");
  if (shell != 0 && getuid () && restricted_shell (pw->pw_shell))
    {
      /* The user being su'd to has a nonstandard shell, and so is
	 probably a uucp account or has restricted access.  Don't
	 compromise the account by allowing access with a standard
	 shell.  */
      error (0, 0, _("using restricted shell %s"), pw->pw_shell);
      shell = 0;
    }
  if (shell == 0)
    {
      shell = xstrdup (pw->pw_shell);
    }
  modify_environment (pw, shell);

  change_identity (pw);
  if (simulate_login && chdir (pw->pw_dir))
    error (0, errno, _("warning: cannot change directory to %s"), pw->pw_dir);

  run_shell (shell, command, additional_args);
}
