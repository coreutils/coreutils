/* pathchk -- check whether pathnames are valid or portable
   Copyright (C) 1991-2003 Free Software Foundation, Inc.

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

/* Usage: pathchk [-p] [--portability] path...

   For each PATH, print a message if any of these conditions are false:
   * all existing leading directories in PATH have search (execute) permission
   * strlen (PATH) <= PATH_MAX
   * strlen (each_directory_in_PATH) <= NAME_MAX

   Exit status:
   0			All PATH names passed all of the tests.
   1			An error occurred.

   Options:
   -p, --portability	Instead of performing length checks on the
			underlying filesystem, test the length of the
			pathname and its components against the POSIX
			minimum limits for portability, _POSIX_NAME_MAX
			and _POSIX_PATH_MAX in 2.9.2.  Also check that
			the pathname contains no character not in the
			portable filename character set.

   David MacKenzie <djm@gnu.ai.mit.edu>
   and Jim Meyering <meyering@cs.utexas.edu> */

#include <config.h>
#include <stdio.h>
#include <getopt.h>
#include <sys/types.h>

#include <errno.h>
#ifndef errno
extern int errno;
#endif

#include "system.h"
#include "error.h"
#include "long-options.h"
#include "closeout.h"

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "pathchk"

#define AUTHORS N_ ("David MacKenzie and Jim Meyering")

#define NEED_PATHCONF_WRAPPER 0
#if HAVE_PATHCONF
# ifndef PATH_MAX
#  define PATH_MAX_FOR(p) pathconf_wrapper ((p), _PC_PATH_MAX)
#  define NEED_PATHCONF_WRAPPER 1
# endif /* not PATH_MAX */
# ifndef NAME_MAX
#  define NAME_MAX_FOR(p) pathconf_wrapper ((p), _PC_NAME_MAX);
#  undef NEED_PATHCONF_WRAPPER
#  define NEED_PATHCONF_WRAPPER 1
# endif /* not NAME_MAX */

#else

# include <sys/param.h>
# ifndef PATH_MAX
#  ifdef MAXPATHLEN
#   define PATH_MAX MAXPATHLEN
#  else /* not MAXPATHLEN */
#   define PATH_MAX _POSIX_PATH_MAX
#  endif /* not MAXPATHLEN */
# endif /* not PATH_MAX */

# ifndef NAME_MAX
#  ifdef MAXNAMLEN
#   define NAME_MAX MAXNAMLEN
#  else /* not MAXNAMLEN */
#   define NAME_MAX _POSIX_NAME_MAX
#  endif /* not MAXNAMLEN */
# endif /* not NAME_MAX */

#endif

#ifndef _POSIX_PATH_MAX
# define _POSIX_PATH_MAX 255
#endif
#ifndef _POSIX_NAME_MAX
# define _POSIX_NAME_MAX 14
#endif

#ifndef PATH_MAX_FOR
# define PATH_MAX_FOR(p) PATH_MAX
#endif
#ifndef NAME_MAX_FOR
# define NAME_MAX_FOR(p) NAME_MAX
#endif

static int validate_path (char *path, int portability);

/* The name this program was run with. */
char *program_name;

static struct option const longopts[] =
{
  {"portability", no_argument, NULL, 'p'},
  {NULL, 0, NULL, 0}
};

#if NEED_PATHCONF_WRAPPER
/* Distinguish between the cases when pathconf fails and when it reports there
   is no limit (the latter is the case for PATH_MAX on the Hurd).  When there
   is no limit, return LONG_MAX.  Otherwise, return pathconf's return value.  */

static long int
pathconf_wrapper (const char *filename, int param)
{
  long int ret;

  errno = 0;
  ret = pathconf (filename, param);
  if (ret < 0 && errno == 0)
    return LONG_MAX;

  return ret;
}
#endif

void
usage (int status)
{
  if (status != 0)
    fprintf (stderr, _("Try `%s --help' for more information.\n"),
	     program_name);
  else
    {
      printf (_("Usage: %s [OPTION]... NAME...\n"), program_name);
      fputs (_("\
Diagnose unportable constructs in NAME.\n\
\n\
  -p, --portability   check for all POSIX systems, not only this one\n\
"), stdout);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      printf (_("\nReport bugs to <%s>.\n"), PACKAGE_BUGREPORT);
    }
  exit (status);
}

int
main (int argc, char **argv)
{
  int exit_status = 0;
  int check_portability = 0;
  int optc;

  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  parse_long_options (argc, argv, PROGRAM_NAME, GNU_PACKAGE, VERSION,
		      AUTHORS, usage);

  while ((optc = getopt_long (argc, argv, "p", longopts, NULL)) != -1)
    {
      switch (optc)
	{
	case 0:
	  break;

	case 'p':
	  check_portability = 1;
	  break;

	default:
	  usage (EXIT_FAILURE);
	}
    }

  if (optind == argc)
    {
      error (0, 0, _("too few arguments"));
      usage (EXIT_FAILURE);
    }

  for (; optind < argc; ++optind)
    exit_status |= validate_path (argv[optind], check_portability);

  exit (exit_status);
}

/* Each element is nonzero if the corresponding ASCII character is
   in the POSIX portable character set, and zero if it is not.
   In addition, the entry for `/' is nonzero to simplify checking. */
static char const portable_chars[256] =
{
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0-15 */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 16-31 */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, /* 32-47 */
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, /* 48-63 */
  0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 64-79 */
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, /* 80-95 */
  0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 96-111 */
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, /* 112-127 */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/* If PATH contains only portable characters, return 1, else 0.  */

static int
portable_chars_only (const char *path)
{
  const char *p;

  for (p = path; *p; ++p)
    if (portable_chars[(unsigned char) *p] == 0)
      {
	error (0, 0, _("path `%s' contains nonportable character `%c'"),
	       path, *p);
	return 0;
      }
  return 1;
}

/* Return 1 if PATH is a usable leading directory, 0 if not,
   2 if it doesn't exist.  */

static int
dir_ok (const char *path)
{
  struct stat stats;

  if (stat (path, &stats))
    return 2;

  if (!S_ISDIR (stats.st_mode))
    {
      error (0, 0, _("`%s' is not a directory"), path);
      return 0;
    }

  /* Use access to test for search permission because
     testing permission bits of st_mode can lose with new
     access control mechanisms.  Of course, access loses if you're
     running setuid. */
  if (access (path, X_OK) != 0)
    {
      if (errno == EACCES)
	error (0, 0, _("directory `%s' is not searchable"), path);
      else
	error (0, errno, "%s", path);
      return 0;
    }

  return 1;
}

/* Make sure that
   strlen (PATH) <= PATH_MAX
   && strlen (each-existing-directory-in-PATH) <= NAME_MAX

   If PORTABILITY is nonzero, compare against _POSIX_PATH_MAX and
   _POSIX_NAME_MAX instead, and make sure that PATH contains no
   characters not in the POSIX portable filename character set, which
   consists of A-Z, a-z, 0-9, ., _, -.

   Make sure that all leading directories along PATH that exist have
   `x' permission.

   Return 0 if all of these tests are successful, 1 if any fail. */

static int
validate_path (char *path, int portability)
{
  long int path_max;
  int last_elem;		/* Nonzero if checking last element of path. */
  int exists IF_LINT (= 0);	/* 2 if the path element exists.  */
  char *slash;
  char *parent;			/* Last existing leading directory so far.  */

  if (portability && !portable_chars_only (path))
    return 1;

  if (*path == '\0')
    return 0;

  /* Figure out the parent of the first element in PATH.  */
  parent = xstrdup (*path == '/' ? "/" : ".");

  slash = path;
  last_elem = 0;
  while (1)
    {
      long int name_max;
      long int length;		/* Length of partial path being checked. */
      char *start;		/* Start of path element being checked. */

      /* Find the end of this element of the path.
	 Then chop off the rest of the path after this element. */
      while (*slash == '/')
	slash++;
      start = slash;
      slash = strchr (slash, '/');
      if (slash != NULL)
	*slash = '\0';
      else
	{
	  last_elem = 1;
	  slash = strchr (start, '\0');
	}

      if (!last_elem)
	{
	  exists = dir_ok (path);
	  if (exists == 0)
	    {
	      free (parent);
	      return 1;
	    }
	}

      length = slash - start;
      /* Since we know that `parent' is a directory, it's ok to call
	 pathconf with it as the argument.  (If `parent' isn't a directory
	 or doesn't exist, the behavior of pathconf is undefined.)
	 But if `parent' is a directory and is on a remote file system,
	 it's likely that pathconf can't give us a reasonable value
	 and will return -1.  (NFS and tempfs are not POSIX . . .)
	 In that case, we have no choice but to assume the pessimal
	 POSIX minimums.  */
      name_max = portability ? _POSIX_NAME_MAX : NAME_MAX_FOR (parent);
      if (name_max < 0)
	name_max = _POSIX_NAME_MAX;
      if (length > name_max)
	{
	  error (0, 0, _("name `%s' has length %ld; exceeds limit of %ld"),
		 start, length, name_max);
	  free (parent);
	  return 1;
	}

      if (last_elem)
	break;

      if (exists == 1)
	{
	  free (parent);
	  parent = xstrdup (path);
	}

      *slash++ = '/';
    }

  /* `parent' is now the last existing leading directory in the whole path,
     so it's ok to call pathconf with it as the argument.  */
  path_max = portability ? _POSIX_PATH_MAX : PATH_MAX_FOR (parent);
  if (path_max < 0)
    path_max = _POSIX_PATH_MAX;
  free (parent);
  if (strlen (path) > (size_t) path_max)
    {
      error (0, 0, _("path `%s' has length %d; exceeds limit of %ld"),
	     path, strlen (path), path_max);
      return 1;
    }

  return 0;
}
