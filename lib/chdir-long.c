/* provide a chdir function that tries not to fail due to ENAMETOOLONG
   Copyright (C) 2004 Free Software Foundation, Inc.

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

/* written by Jim Meyering */

#include <config.h>

#include "chdir-long.h"

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <assert.h>
#include <limits.h>

#include "mempcpy.h"
#include "openat.h"

#ifndef O_DIRECTORY
# define O_DIRECTORY 0
#endif

#ifndef MIN
# define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef PATH_MAX
# error "compile this file only if your system defines PATH_MAX"
#endif

/* FIXME: this use of `MIN' is our sole concession to arbitrary limitations.
   If, for some system, PATH_MAX is larger than 8191 and you call
   chdir_long with a directory name that is longer than PATH_MAX,
   yet that contains a single component that is more than 8191 bytes
   long, then this function will fail.  */
#define MAX_COMPONENT_LENGTH MIN (PATH_MAX - 1, 8 * 1024)

struct cd_buf
{
  /* FIXME maybe allocate this via malloc, rather than using the stack.
     But that would be the sole use of malloc.  Is it worth it to
     let chdir_long fail due to a low-memory condition?
     But when using malloc, and assuming we remove the `concession'
     above, we'll still have to avoid allocating 2^31 bytes on
     systems that define PATH_MAX to very large number.
     Ideally, we'd allocate enough to deal with most names, and
     dynamically increase the buffer size only when necessary.  */
  char buffer[MAX_COMPONENT_LENGTH + 1];
  char *avail;
  int fd;
};

/* Like memchr, but return the number of bytes from MEM
   to the first occurrence of C thereafter.  Search only
   LEN bytes.  Return LEN if C is not found.  */
static inline size_t
memchrcspn (char const *mem, int c, size_t len)
{
  char const *found = memchr (mem, c, len);
  if (!found)
    return len;

  len = found - mem;
  return len;
}

static void
cdb_init (struct cd_buf *cdb)
{
  cdb->avail = cdb->buffer;
  cdb->fd = AT_FDCWD;
}

static inline bool
cdb_empty (struct cd_buf const *cdb)
{
  return cdb->avail == cdb->buffer;
}

static inline int
cdb_fchdir (struct cd_buf const *cdb)
{
  return fchdir (cdb->fd);
}

static int
cdb_advance_fd (struct cd_buf *cdb, char const *dir)
{
  int new_fd = openat (cdb->fd, dir, O_RDONLY | O_DIRECTORY);
  if (new_fd < 0)
    {
      new_fd = openat (cdb->fd, dir, O_WRONLY | O_DIRECTORY);
      if (new_fd < 0)
	return -1;
    }

  if (cdb->fd != AT_FDCWD)
    close (cdb->fd);
  cdb->fd = new_fd;

  return 0;
}

static int
cdb_flush (struct cd_buf *cdb)
{
  if (cdb_empty (cdb))
    return 0;

  cdb->avail[0] = '\0';
  if (cdb_advance_fd (cdb, cdb->buffer) != 0)
    return -1;

  cdb->avail = cdb->buffer;

  return 0;
}

static void
cdb_free (struct cd_buf *cdb)
{
  if (0 <= cdb->fd && close (cdb->fd) != 0)
    abort ();
}

static int
cdb_append (struct cd_buf *cdb, char const *s, size_t len)
{
  char const *end = cdb->buffer + sizeof cdb->buffer;

  /* Insert a slash separator if there is a preceding byte
     and it's not a slash.  */
  bool need_slash = (cdb->buffer < cdb->avail && cdb->avail[-1] != '/');
  size_t n_free;

  if (sizeof cdb->buffer < len + 1)
    {
      /* This single component is too long.  */
      errno = ENAMETOOLONG;
      return -1;
    }

  /* See if there's enough room for the `/', the new component and
     a trailing NUL.  */
  n_free = end - cdb->avail;
  if (n_free < need_slash + len + 1)
    {
      if (cdb_flush (cdb) != 0)
	return -1;
      need_slash = false;
    }

  if (need_slash)
    *(cdb->avail)++ = '/';

  cdb->avail = mempcpy (cdb->avail, s, len);
  return 0;
}

/* This is a wrapper around chdir that works even on PATH_MAX-limited
   systems.  It handles an arbitrarily long directory name by extracting
   and processing manageable portions of the name.  On systems without
   the openat syscall, this means changing the working directory to
   more and more `distant' points along the long directory name and
   then restoring the working directory.
   If any of those attempts to change or restore the working directory
   fails, this function exits nonzero.

   Note that this function may still fail with errno == ENAMETOOLONG,
   but only if the specified directory name contains a component that
   is long enough to provoke such a failure all by itself (e.g. if the
   component is longer than PATH_MAX on systems that define PATH_MAX).  */

int
chdir_long (char const *dir)
{
  int e = chdir (dir);
  if (e == 0 || errno != ENAMETOOLONG)
    return e;

  {
    size_t len = strlen (dir);
    char const *dir_end = dir + len;
    char const *d;
    struct cd_buf cdb;

    cdb_init (&cdb);

    /* If DIR is the empty string, then the chdir above
       must have failed and set errno to ENOENT.  */
    assert (0 < len);

    if (*dir == '/')
      {
	/* Names starting with exactly two slashes followed by at least
	   one non-slash are special --
	   for example, in some environments //Hostname/file may
	   denote a file on a different host.
	   Preserve those two leading slashes.  Treat all other
	   sequences of slashes like a single one.  */
	if (3 <= len && dir[1] == '/' && dir[2] != '/')
	  {
	    size_t name_len = 1 + strcspn (dir + 3, "/");
	    if (cdb_append (&cdb, dir, 2 + name_len) != 0)
	      goto Fail;
	    /* Advance D to next slash or to end of string. */
	    d = dir + 2 + name_len;
	    assert (*d == '/' || *d == '\0');
	  }
	else
	  {
	    if (cdb_append (&cdb, "/", 1) != 0)
	      goto Fail;
	    d = dir + 1;
	  }
      }
    else
      {
	d = dir;
      }

    while (1)
      {
	/* Skip any slashes to find start of next component --
	   or the end of DIR. */
	char const *start = d + strspn (d, "/");
	if (*start == '\0')
	  {
	    if (cdb_flush (&cdb) != 0)
	      goto Fail;
	    break;
	  }
	/* If the remaining portion is no longer than PATH_MAX, then
	   flush anything that is buffered and do the rest in one chunk.  */
	if (dir_end - start <= PATH_MAX)
	  {
	    if (cdb_flush (&cdb) != 0
		|| cdb_advance_fd (&cdb, start) != 0)
	      goto Fail;
	    break;
	  }

	len = memchrcspn (start, '/', dir_end - start);
	assert (len == strcspn (start, "/"));
	d = start + len;
	if (cdb_append (&cdb, start, len) != 0)
	  goto Fail;
      }

    if (cdb_fchdir (&cdb) != 0)
      goto Fail;

    cdb_free (&cdb);
    return 0;

   Fail:
    {
      int saved_errno = errno;
      cdb_free (&cdb);
      errno = saved_errno;
      return -1;
    }
  }
}

#if TEST_CHDIR

# include <stdio.h>
# include "closeout.h"
# include "error.h"

char *program_name;

int
main (int argc, char *argv[])
{
  char *line = NULL;
  size_t n = 0;
  int len;

  program_name = argv[0];
  atexit (close_stdout);

  len = getline (&line, &n, stdin);
  if (len < 0)
    {
      int saved_errno = errno;
      if (feof (stdin))
	exit (0);

      error (EXIT_FAILURE, saved_errno,
	     "reading standard input");
    }
  else if (len == 0)
    exit (0);

  if (line[len-1] == '\n')
    line[len-1] = '\0';

  if (chdir_long (line) != 0)
    error (EXIT_FAILURE, errno,
	   "chdir_long failed: %s", line);

  {
    /* Using `pwd' here makes sense only if it is a robust implementation,
       like the one in coreutils after the 2004-04-19 changes.  */
    char const *cmd = "pwd";
    execlp (cmd, (char *) NULL);
    error (EXIT_FAILURE, errno, "%s", cmd);
  }

  /* not reached */
  abort ();
}
#endif

/*
Local Variables:
compile-command: "gcc -DTEST_CHDIR=1 -DHAVE_CONFIG_H -I.. -g -O -W -Wall chdir-long.c libfetish.a"
End:
*/
