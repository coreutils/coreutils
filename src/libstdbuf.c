/* libstdbuf -- a shared lib to preload to setup stdio buffering for a command
   Copyright (C) 2009-2025 Free Software Foundation, Inc.

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

/* Written by Pádraig Brady.  LD_PRELOAD idea from Brian Dessent.  */

#include <config.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "gettext.h"
#define _(msgid) gettext (msgid)

/* Deactivate config.h's "rpl_"-prefixed definitions, since we don't
   link gnulib here, and the replacements aren't needed.  */
#undef fprintf
#undef free
#undef malloc
#undef strtoul

/* Note currently for glibc (2.3.5) the following call does not change
   the buffer size, and more problematically does not give any indication
   that the new size request was ignored:

       setvbuf (stdout, nullptr, _IOFBF, 8192);

   The ISO C99 standard section 7.19.5.6 on the setvbuf function says:

   ... If buf is not a null pointer, the array it points to _may_ be used
   instead of a buffer allocated by the setvbuf function and the argument
   size specifies the size of the array; otherwise, size _may_ determine
   the size of a buffer allocated by the setvbuf function. ...

   Obviously some interpret the above to mean setvbuf (..., size)
   is only a hint from the application which I don't agree with.

   FreeBSD's libc seems more sensible in this regard. From the man page:

   The size argument may be given as zero to obtain deferred optimal-size
   buffer allocation as usual.  If it is not zero, then except for
   unbuffered files, the buf argument should point to a buffer at least size
   bytes long; this buffer will be used instead of the current buffer.  (If
   the size argument is not zero but buf is null, a buffer of the given size
   will be allocated immediately, and released on close.  This is an extension
   to ANSI C; portable code should use a size of 0 with any null buffer.)
   --------------------
   Another issue is that on glibc-2.7 the following doesn't buffer
   the first write if it's greater than 1 byte.

       setvbuf (stdout, buf, _IOFBF, 127);

   Now the POSIX standard says that "allocating a buffer of size bytes does
   not necessarily imply that all of size bytes are used for the buffer area".
   However I think it's just a buggy implementation due to the various
   inconsistencies with write sizes and subsequent writes.  */

static void
apply_mode (FILE *stream, char const *stream_name, char const *envvar)
{
  char *buf = nullptr;
  int setvbuf_mode;
  unsigned long int size = 0;

  char const *mode = getenv (envvar);
  if (!mode)
    return;

  if (*mode == '0')
    setvbuf_mode = _IONBF;
  else if (*mode == 'L')
    setvbuf_mode = _IOLBF;      /* FIXME: should we allow 1ML  */
  else
    {
      setvbuf_mode = _IOFBF;
      char *mode_end;
      size = strtoul (mode, &mode_end, 10);
      if (size == 0 || *mode_end)
        {
          fprintf (stderr, _("invalid buffering mode %s for %s\n"),
                   mode, stream_name);
          return;
        }

      /* If strtoul might have overflowed or if the size is more than
         half of size_t range, treat it as an allocation failure.
         Huge sizes can cause problems with some stdio implementations.  */
      buf = (size <= ((unsigned long int) -2 < (size_t) -1 / 2
                      ? (unsigned long int) -2 : (size_t) -1 / 2)
             ? malloc (size) : nullptr);
      if (!buf)
        {
          /* We could defer the allocation to libc, however since
             glibc currently ignores the combination of null buffer
             with non zero size, we'll fail here.  */
          fprintf (stderr,
                   _("failed to allocate a %lu byte stdio buffer\n"),
                   size);
          return;
        }
      /* buf will be freed by fclose.  */
    }

  if (setvbuf (stream, buf, setvbuf_mode, size) != 0)
    {
      fprintf (stderr, _("could not set buffering of %s to mode %s\n"),
               stream_name, mode);
      free (buf);
    }
}

/* Use __attribute to avoid elision of __attribute__ on SUNPRO_C etc.  */
static void __attribute ((constructor))
stdbuf (void)
{
  /* Do first so can write errors to stderr.  */
  apply_mode (stderr, "stderr", "_STDBUF_E");

  apply_mode (stdin, "stdin", "_STDBUF_I");
  apply_mode (stdout, "stdout", "_STDBUF_O");
}
