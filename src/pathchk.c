/* pathchk -- check whether file names are valid or portable
   Copyright (C) 1991-2004 Free Software Foundation, Inc.

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

#include <config.h>
#include <stdio.h>
#include <getopt.h>
#include <sys/types.h>
#if HAVE_WCHAR_H
# include <wchar.h>
#endif

#include "system.h"
#include "error.h"
#include "euidaccess.h"
#include "quote.h"
#include "quotearg.h"

#if ! (HAVE_MBRLEN && HAVE_MBSTATE_T)
# define mbrlen(s, n, ps) 1
# define mbstate_t int
#endif

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "pathchk"

#define AUTHORS "Paul Eggert", "David MacKenzie", "Jim Meyering"

#ifndef _POSIX_PATH_MAX
# define _POSIX_PATH_MAX 255
#endif
#ifndef _POSIX_NAME_MAX
# define _POSIX_NAME_MAX 14
#endif

#ifdef _XOPEN_NAME_MAX
# define NAME_MAX_MINIMUM _XOPEN_NAME_MAX
#else
# define NAME_MAX_MINIMUM _POSIX_NAME_MAX
#endif
#ifdef _XOPEN_PATH_MAX
# define PATH_MAX_MINIMUM _XOPEN_PATH_MAX
#else
# define PATH_MAX_MINIMUM _POSIX_PATH_MAX
#endif

#if ! (HAVE_PATHCONF && defined _PC_NAME_MAX && defined _PC_PATH_MAX)
# ifndef _PC_NAME_MAX
#  define _PC_NAME_MAX 0
#  define _PC_PATH_MAX 1
# endif
# ifndef pathconf
#  define pathconf(file, flag) \
     (flag == _PC_NAME_MAX ? NAME_MAX_MINIMUM : PATH_MAX_MINIMUM)
# endif
#endif

static bool validate_file_name (char *file, bool portability);

/* The name this program was run with. */
char *program_name;

static struct option const longopts[] =
{
  {"portability", no_argument, NULL, 'p'},
  {GETOPT_HELP_OPTION_DECL},
  {GETOPT_VERSION_OPTION_DECL},
  {NULL, 0, NULL, 0}
};

void
usage (int status)
{
  if (status != EXIT_SUCCESS)
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
  bool ok = true;
  bool check_portability = false;
  int optc;

  initialize_main (&argc, &argv);
  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  while ((optc = getopt_long (argc, argv, "+p", longopts, NULL)) != -1)
    {
      switch (optc)
	{
	case 'p':
	  check_portability = true;
	  break;

	case_GETOPT_HELP_CHAR;

	case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);

	default:
	  usage (EXIT_FAILURE);
	}
    }

  if (optind == argc)
    {
      error (0, 0, _("missing operand"));
      usage (EXIT_FAILURE);
    }

  for (; optind < argc; ++optind)
    ok &= validate_file_name (argv[optind], check_portability);

  exit (ok ? EXIT_SUCCESS : EXIT_FAILURE);
}

/* If FILE (of length FILELEN) contains only portable characters,
   return true, else report an error and return false.  */

static bool
portable_chars_only (char const *file, size_t filelen)
{
  size_t validlen = strspn (file,
			    ("/"
			     "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
			     "abcdefghijklmnopqrstuvwxyz"
			     "0123456789._-"));
  char const *invalid = file + validlen;

  if (*invalid)
    {
      mbstate_t mbstate = {0};
      size_t charlen = mbrlen (invalid, filelen - validlen, &mbstate);
      error (0, 0,
	     _("nonportable character %s in file name %s"),
	     quotearg_n_style_mem (1, locale_quoting_style, invalid,
				   (charlen <= MB_LEN_MAX ? charlen : 1)),
	     quote_n (0, file));
      return false;
    }

  return true;
}

/* Return the address of the start of the next file name component in F.  */

static char *
component_start (char *f)
{
  while (*f == '/')
    f++;
  return f;
}

/* Return the size of the file name component F.  F must be nonempty.  */

static size_t
component_len (char const *f)
{
  size_t len;
  for (len = 1; f[len] != '/' && f[len]; len++)
    continue;
  return len;
}

/* Make sure that
   strlen (FILE) <= PATH_MAX
   && strlen (each-existing-directory-in-FILE) <= NAME_MAX

   If PORTABILITY is true, compare against _POSIX_PATH_MAX and
   _POSIX_NAME_MAX instead, and make sure that FILE contains no
   characters not in the POSIX portable filename character set, which
   consists of A-Z, a-z, 0-9, ., _, - (plus / for separators).

   If PORTABILITY is false, make sure that all leading directories
   along FILE that exist are searchable.

   Return true if all of these tests are successful, false if any fail.  */

static bool
validate_file_name (char *file, bool portability)
{
  size_t filelen = strlen (file);

  /* Start of file name component being checked.  */
  char *start;

  /* True if component lengths need to be checked.  */
  bool check_component_lengths;

  if (portability && ! portable_chars_only (file, filelen))
    return false;

  if (*file == '\0')
    return true;

  if (portability || PATH_MAX_MINIMUM <= filelen)
    {
      size_t maxsize;

      if (portability)
	maxsize = _POSIX_PATH_MAX;
      else
	{
	  long int size;
	  char const *dir = (*file == '/' ? "/" : ".");
	  errno = 0;
	  size = pathconf (dir, _PC_PATH_MAX);
	  if (size < 0 && errno != 0)
	    {
	      error (0, errno,
		     _("%s: unable to determine maximum file name length"),
		     dir);
	      return false;
	    }
	  maxsize = MIN (size, SIZE_MAX);
	}

      if (maxsize <= filelen)
	{
	  unsigned long int len = filelen;
	  unsigned long int maxlen = maxsize - 1;
	  error (0, 0, _("limit %lu exceeded by length %lu of file name %s"),
		 maxlen, len, quote (file));
	  return false;
	}
    }

  if (! portability)
    {
      /* Check whether a file name component is in a directory that
	 is not searchable, or has some other serious problem.  */

      struct stat st;
      if (lstat (file, &st) != 0 && errno != ENOENT)
	{
	  error (0, errno, "%s", file);
	  return false;
	}
    }

  /* Check whether pathconf (..., _PC_NAME_MAX) can be avoided, i.e.,
     whether all file name components are so short that they are valid
     in any file system on this platform.  If PORTABILITY, though,
     it's more convenient to check component lengths below.  */

  check_component_lengths = portability;
  if (! check_component_lengths)
    {
      for (start = file; *(start = component_start (start)); )
	{
	  size_t length = component_len (start);

	  if (NAME_MAX_MINIMUM < length)
	    {
	      check_component_lengths = true;
	      break;
	    }

	  start += length;
	}
    }

  if (check_component_lengths)
    {
      /* The limit on file name components for the current component.
         This defaults to NAME_MAX_MINIMUM, for the sake of non-POSIX
         systems (NFS, say?) where pathconf fails on "." or "/" with
         errno == ENOENT.  */
      size_t name_max = NAME_MAX_MINIMUM;

      /* If nonzero, the known limit on file name components.  */
      size_t known_name_max = (portability ? _POSIX_NAME_MAX : 0);

      for (start = file; *(start = component_start (start)); )
	{
	  size_t length;

	  if (known_name_max)
	    name_max = known_name_max;
	  else
	    {
	      long int len;
	      char const *dir = (start == file ? "." : file);
	      char c = *start;
	      errno = 0;
	      *start = '\0';
	      len = pathconf (dir, _PC_NAME_MAX);
	      *start = c;
	      if (0 <= len)
		name_max = MIN (len, SIZE_MAX);
	      else
		switch (errno)
		  {
		  case 0:
		    /* There is no limit.  */
		    name_max = SIZE_MAX;
		    break;

		  case ENOENT:
		    /* DIR does not exist; use its parent's maximum.  */
		    known_name_max = name_max;
		    break;

		  default:
		    *start = '\0';
		    error (0, errno, "%s", dir);
		    *start = c;
		    return false;
		  }
	    }

	  length = component_len (start);

	  if (name_max < length)
	    {
	      unsigned long int len = length;
	      unsigned long int maxlen = name_max;
	      char c = start[len];
	      start[len] = '\0';
	      error (0, 0,
		     _("limit %lu exceeded by length %lu "
		       "of file name component %s"),
		     maxlen, len, quote (start));
	      start[len] = c;
	      return false;
	    }

	  start += length;
	}
    }

  return true;
}
