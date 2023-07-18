/* temp-stream.c -- provide a stream to a per process temp file

   Copyright (C) 2023 Free Software Foundation, Inc.

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

#include <config.h>

#include <stdbool.h>
#include <stdio.h>

#include "stdlib--.h"  /* For mkstemp that returns safer FDs.  */
#include "system.h"
#include "tmpdir.h"

#include "temp-stream.h"


#if defined __MSDOS__ || defined _WIN32
/* Define this to non-zero on systems for which the regular mechanism
   (of unlinking an open file and expecting to be able to write, seek
   back to the beginning, then reread it) doesn't work.  E.g., on Windows
   and DOS systems.  */
# define DONT_UNLINK_WHILE_OPEN 1
#endif

#if DONT_UNLINK_WHILE_OPEN

/* FIXME-someday: remove all of this DONT_UNLINK_WHILE_OPEN junk.
   Using atexit like this is wrong, since it can fail
   when called e.g. 32 or more times.
   But this isn't a big deal, since the code is used only on WOE/DOS
   systems, and few people invoke tac on that many nonseekable files.  */

static char const *file_to_remove;
static FILE *fp_to_close;

static void
unlink_tempfile (void)
{
  fclose (fp_to_close);
  unlink (file_to_remove);
}

static void
record_or_unlink_tempfile (char const *fn, FILE *fp)
{
  if (!file_to_remove)
    {
      file_to_remove = fn;
      fp_to_close = fp;
      atexit (unlink_tempfile);
    }
}

#else

static void
record_or_unlink_tempfile (char const *fn, MAYBE_UNUSED FILE *fp)
{
  unlink (fn);
}

#endif

/* A wrapper around mkstemp that gives us both an open stream pointer,
   FP, and the corresponding FILE_NAME.  Always return the same FP/name
   pair, rewinding/truncating it upon each reuse.

   Note this honors $TMPDIR, unlike the standard defined tmpfile().

   Returns TRUE on success.  */
bool
temp_stream (FILE **fp, char **file_name)
{
  static char *tempfile = nullptr;
  static FILE *tmp_fp;
  if (tempfile == nullptr)
    {
      char *tempbuf = nullptr;
      size_t tempbuf_len = 128;

      while (true)
        {
          if (! (tempbuf = realloc (tempbuf, tempbuf_len)))
            {
              error (0, errno, _("failed to make temporary file name"));
              return false;
            }

          if (path_search (tempbuf, tempbuf_len, nullptr, "cutmp", true) == 0)
            break;

          if (errno != EINVAL || PATH_MAX / 2 < tempbuf_len)
            {
              error (0, errno == EINVAL ? ENAMETOOLONG : errno,
                     _("failed to make temporary file name"));
              return false;
            }

          tempbuf_len *= 2;
        }

      tempfile = tempbuf;

      /* FIXME: there's a small window between a successful mkstemp call
         and the unlink that's performed by record_or_unlink_tempfile.
         If we're interrupted in that interval, this code fails to remove
         the temporary file.  On systems that define DONT_UNLINK_WHILE_OPEN,
         the window is much larger -- it extends to the atexit-called
         unlink_tempfile.
         FIXME: clean up upon fatal signal.  Don't block them, in case
         $TMPDIR is a remote file system.  */

      int fd = mkstemp (tempfile);
      if (fd < 0)
        {
          error (0, errno, _("failed to create temporary file %s"),
                 quoteaf (tempfile));
          goto Reset;
        }

      tmp_fp = fdopen (fd, (O_BINARY ? "w+b" : "w+"));
      if (! tmp_fp)
        {
          error (0, errno, _("failed to open %s for writing"),
                 quoteaf (tempfile));
          close (fd);
          unlink (tempfile);
        Reset:
          free (tempfile);
          tempfile = nullptr;
          return false;
        }

      record_or_unlink_tempfile (tempfile, tmp_fp);
    }
  else
    {
      clearerr (tmp_fp);
      if (fseeko (tmp_fp, 0, SEEK_SET) < 0
          || ftruncate (fileno (tmp_fp), 0) < 0)
        {
          error (0, errno, _("failed to rewind stream for %s"),
                 quoteaf (tempfile));
          return false;
        }
    }

  *fp = tmp_fp;
  if (file_name)
    *file_name = tempfile;
  return true;
}
