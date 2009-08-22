/* tempname.c - generate the name of a temporary file.

   Copyright (C) 1991, 1992, 1993, 1994, 1995, 1996, 1997, 1998, 1999,
   2000, 2001, 2002, 2003, 2005, 2006, 2007, 2009 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* Extracted from glibc sysdeps/posix/tempname.c.  See also tmpdir.c.  */

#if !_LIBC
# include <config.h>
# include "tempname.h"
# include "randint.h"
#endif

#include <sys/types.h>
#include <assert.h>

#include <errno.h>
#ifndef __set_errno
# define __set_errno(Val) errno = (Val)
#endif

#include <stdio.h>
#ifndef P_tmpdir
# define P_tmpdir "/tmp"
#endif
#ifndef TMP_MAX
# define TMP_MAX 238328
#endif
#ifndef __GT_FILE
# define __GT_FILE	0
# define __GT_BIGFILE	1
# define __GT_DIR	2
# define __GT_NOCREATE	3
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <sys/time.h>
#include <stdint.h>
#include <unistd.h>

#include <sys/stat.h>

#if _LIBC
# define struct_stat64 struct stat64
# define small_open __open
# define large_open __open64
#else
# define struct_stat64 struct stat
# define small_open open
# define large_open open
# define __gen_tempname gen_tempname
# define __getpid getpid
# define __gettimeofday gettimeofday
# define __mkdir mkdir
# define __lxstat64(version, file, buf) lstat (file, buf)
# define __xstat64(version, file, buf) stat (file, buf)
#endif

#if ! (HAVE___SECURE_GETENV || _LIBC)
# define __secure_getenv getenv
#endif

#if _LIBC
/* Return nonzero if DIR is an existent directory.  */
static int
direxists (const char *dir)
{
  struct_stat64 buf;
  return __xstat64 (_STAT_VER, dir, &buf) == 0 && S_ISDIR (buf.st_mode);
}

/* Path search algorithm, for tmpnam, tmpfile, etc.  If DIR is
   non-null and exists, uses it; otherwise uses the first of $TMPDIR,
   P_tmpdir, /tmp that exists.  Copies into TMPL a template suitable
   for use with mk[s]temp.  Will fail (-1) if DIR is non-null and
   doesn't exist, none of the searched dirs exists, or there's not
   enough space in TMPL. */
int
__path_search (char *tmpl, size_t tmpl_len, const char *dir, const char *pfx,
               int try_tmpdir)
{
  const char *d;
  size_t dlen, plen;

  if (!pfx || !pfx[0])
    {
      pfx = "file";
      plen = 4;
    }
  else
    {
      plen = strlen (pfx);
      if (plen > 5)
        plen = 5;
    }

  if (try_tmpdir)
    {
      d = __secure_getenv ("TMPDIR");
      if (d != NULL && direxists (d))
        dir = d;
      else if (dir != NULL && direxists (dir))
        /* nothing */ ;
      else
        dir = NULL;
    }
  if (dir == NULL)
    {
      if (direxists (P_tmpdir))
        dir = P_tmpdir;
      else if (strcmp (P_tmpdir, "/tmp") != 0 && direxists ("/tmp"))
        dir = "/tmp";
      else
        {
          __set_errno (ENOENT);
          return -1;
        }
    }

  dlen = strlen (dir);
  while (dlen > 1 && dir[dlen - 1] == '/')
    dlen--;			/* remove trailing slashes */

  /* check we have room for "${dir}/${pfx}XXXXXX\0" */
  if (tmpl_len < dlen + 1 + plen + 6 + 1)
    {
      __set_errno (EINVAL);
      return -1;
    }

  sprintf (tmpl, "%.*s/%.*sXXXXXX", (int) dlen, dir, (int) plen, pfx);
  return 0;
}
#endif /* _LIBC */

static inline bool
check_x_suffix (char const *s, size_t len)
{
  return strspn (s, "X") == len;
}

/* These are the characters used in temporary file names.  */
static const char letters[] =
"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

/* Generate a temporary file name based on TMPL.  TMPL must end in a
   a sequence of at least X_SUFFIX_LEN "X"s.  The name constructed
   does not exist at the time of the call to __gen_tempname.  TMPL is
   overwritten with the result.

   KIND may be one of:
   __GT_NOCREATE:	simply verify that the name does not exist
                        at the time of the call.
   __GT_FILE:		create the file using open(O_CREAT|O_EXCL)
                        and return a read-write fd.  The file is mode 0600.
   __GT_BIGFILE:	same as __GT_FILE but use open64().
   __GT_DIR:		create a directory, which will be mode 0700.

   We use a clever algorithm to get hard-to-predict names. */
int
gen_tempname_len (char *tmpl, int kind, size_t x_suffix_len)
{
  size_t len;
  char *XXXXXX;
  unsigned int count;
  int fd = -1;
  int save_errno = errno;
  struct_stat64 st;
  struct randint_source *rand_src;

  /* A lower bound on the number of temporary files to attempt to
     generate.  The maximum total number of temporary file names that
     can exist for a given template is 62**6.  It should never be
     necessary to try all these combinations.  Instead if a reasonable
     number of names is tried (we define reasonable as 62**3) fail to
     give the system administrator the chance to remove the problems.  */
#define ATTEMPTS_MIN (62 * 62 * 62)

  /* The number of times to attempt to generate a temporary file.  To
     conform to POSIX, this must be no smaller than TMP_MAX.  */
#if ATTEMPTS_MIN < TMP_MAX
  unsigned int attempts = TMP_MAX;
#else
  unsigned int attempts = ATTEMPTS_MIN;
#endif

  len = strlen (tmpl);
  if (len < x_suffix_len || ! check_x_suffix (&tmpl[len - x_suffix_len],
                                              x_suffix_len))
    {
      __set_errno (EINVAL);
      return -1;
    }

  rand_src = randint_all_new (NULL, 8);
  if (! rand_src)
    return -1;

  /* This is where the Xs start.  */
  XXXXXX = &tmpl[len - x_suffix_len];

  for (count = 0; count < attempts; ++count)
    {
      size_t i;

      for (i = 0; i < x_suffix_len; i++)
        {
          XXXXXX[i] = letters[randint_genmax (rand_src, sizeof letters - 2)];
        }

      switch (kind)
        {
        case __GT_FILE:
          fd = small_open (tmpl, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
          break;

        case __GT_BIGFILE:
          fd = large_open (tmpl, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
          break;

        case __GT_DIR:
          fd = __mkdir (tmpl, S_IRUSR | S_IWUSR | S_IXUSR);
          break;

        case __GT_NOCREATE:
          /* This case is backward from the other three.  This function
             succeeds if __xstat fails because the name does not exist.
             Note the continue to bypass the common logic at the bottom
             of the loop.  */
          if (__lxstat64 (_STAT_VER, tmpl, &st) < 0)
            {
              if (errno == ENOENT)
                {
                  __set_errno (save_errno);
                  fd = 0;
                  goto done;
                }
              else
                {
                  /* Give up now. */
                  fd = -1;
                  goto done;
                }
            }
          continue;

        default:
          assert (! "invalid KIND in __gen_tempname");
        }

      if (fd >= 0)
        {
          __set_errno (save_errno);
          goto done;
        }
      else if (errno != EEXIST)
        {
          fd = -1;
          goto done;
        }
    }

  randint_all_free (rand_src);

  /* We got out of the loop because we ran out of combinations to try.  */
  __set_errno (EEXIST);
  return -1;

 done:
  {
    int saved_errno = errno;
    randint_all_free (rand_src);
    __set_errno (saved_errno);
  }
  return fd;
}

int
__gen_tempname (char *tmpl, int kind)
{
  return gen_tempname_len (tmpl, kind, 6);
}
