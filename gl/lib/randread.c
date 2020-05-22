/* Generate buffers of random data.

   Copyright (C) 2006-2020 Free Software Foundation, Inc.

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

/* Written by Paul Eggert.  */

/* FIXME: Improve performance by adding support for the RDRAND machine
   instruction if available (e.g., Ivy Bridge processors).  */

#include <config.h>

#include "randread.h"

#include <errno.h>
#include <error.h>
#include <exitfail.h>
#include <fcntl.h>
#include <quote.h>
#include <stdalign.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include "gettext.h"
#define _(msgid) gettext (msgid)

#include "rand-isaac.h"
#include "stdio-safer.h"
#include "unlocked-io.h"
#include "xalloc.h"

#ifndef __attribute__
# if __GNUC__ < 2 || (__GNUC__ == 2 && __GNUC_MINOR__ < 8)
#  define __attribute__(x) /* empty */
# endif
#endif

#ifndef ATTRIBUTE_NORETURN
# define ATTRIBUTE_NORETURN __attribute__ ((__noreturn__))
#endif

#ifndef MIN
# define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#if _STRING_ARCH_unaligned || _STRING_INLINE_unaligned
# define ALIGNED_POINTER(ptr, type) true
#else
# define ALIGNED_POINTER(ptr, type) ((size_t) (ptr) % alignof (type) == 0)
#endif

#ifndef NAME_OF_NONCE_DEVICE
# define NAME_OF_NONCE_DEVICE "/dev/urandom"
#endif

/* The maximum buffer size used for reads of random data.  Using the
   value 2 * ISAAC_BYTES makes this the largest power of two that
   would not otherwise cause struct randread_source to grow.  */
#define RANDREAD_BUFFER_SIZE (2 * ISAAC_BYTES)

/* A source of random data for generating random buffers.  */
struct randread_source
{
  /* Stream to read random bytes from.  If null, the current
     implementation uses an internal PRNG (ISAAC).  */
  FILE *source;

  /* Function to call, and its argument, if there is an input error or
     end of file when reading from the stream; errno is nonzero if
     there was an error.  If this function returns, it should fix the
     problem before returning.  The default handler assumes that
     handler_arg is the file name of the source.  */
  void (*handler) (void const *);
  void const *handler_arg;

  /* The buffer for SOURCE.  It's kept here to simplify storage
     allocation and to make it easier to clear out buffered random
     data.  */
  union
  {
    /* The stream buffer, if SOURCE is not null.  */
    char c[RANDREAD_BUFFER_SIZE];

    /* The buffered ISAAC pseudorandom buffer, if SOURCE is null.  */
    struct isaac
    {
      /* The number of bytes that are buffered at the end of data.b.  */
      size_t buffered;

      /* State of the ISAAC generator.  */
      struct isaac_state state;

      /* Up to a buffer's worth of pseudorandom data.  */
      union
      {
        isaac_word w[ISAAC_WORDS];
        unsigned char b[ISAAC_BYTES];
      } data;
    } isaac;
  } buf;
};


/* The default error handler.  */

static void ATTRIBUTE_NORETURN
randread_error (void const *file_name)
{
  if (file_name)
    error (exit_failure, errno,
           errno == 0 ? _("%s: end of file") : _("%s: read error"),
           quote (file_name));
  abort ();
}

/* Simply return a new randread_source object with the default error
   handler.  */

static struct randread_source *
simple_new (FILE *source, void const *handler_arg)
{
  struct randread_source *s = xmalloc (sizeof *s);
  s->source = source;
  s->handler = randread_error;
  s->handler_arg = handler_arg;
  return s;
}

/* Put a nonce value into BUFFER, with size BUFSIZE.

   Return true on success, false (setting errno) on failure. */

static bool
get_nonce (void *buffer, size_t bufsize)
{
  char *buf = buffer;
  ssize_t seeded = 0;

  /* Get some data from FD if available.  */
  int fd = open (NAME_OF_NONCE_DEVICE, O_RDONLY | O_BINARY);
  if (fd < 0)
      return false;

  TEMP_FAILURE_RETRY(seeded = read(fd, buf, bufsize));
  if (seeded < 0)
      return false;

  close(fd);
  return true;
}

/* Create and initialize a random data source from NAME, or use a
   reasonable default source if NAME is null.

   If NAME is not null, NAME is saved for use as the argument of the
   default handler.  Unless a non-default handler is used, NAME's
   lifetime should be at least that of the returned value.

   Return NULL (setting errno) on failure.  */

struct randread_source *
randread_new (char const *name)
{
  FILE *source = NULL;
  struct randread_source *s;

  if (name)
    if (! (source = fopen_safer (name, "rb")))
      return NULL;

  s = simple_new (source, name);

  if (source)
    setvbuf (source, s->buf.c, _IOFBF, sizeof s->buf.c);
  else
    {
      s->buf.isaac.buffered = 0;
      if (!get_nonce (s->buf.isaac.state.m, sizeof s->buf.isaac.state.m))
        return NULL;
      isaac_seed(&s->buf.isaac.state);
    }

  return s;
}


/* Set S's handler and its argument.  HANDLER (HANDLER_ARG) is called
   when there is a read error or end of file from the random data
   source; errno is nonzero if there was an error.  If HANDLER
   returns, it should fix the problem before returning.  The default
   handler assumes that handler_arg is the file name of the source; it
   does not return.  */

void
randread_set_handler (struct randread_source *s, void (*handler) (void const *))
{
  s->handler = handler;
}

void
randread_set_handler_arg (struct randread_source *s, void const *handler_arg)
{
  s->handler_arg = handler_arg;
}


/* Place SIZE random bytes into the buffer beginning at P, using
   the stream in S.  */

static void
readsource (struct randread_source *s, unsigned char *p, size_t size)
{
  while (true)
    {
      size_t inbytes = fread (p, sizeof *p, size, s->source);
      int fread_errno = errno;
      p += inbytes;
      size -= inbytes;
      if (size == 0)
        break;
      errno = (ferror (s->source) ? fread_errno : 0);
      s->handler (s->handler_arg);
    }
}


/* Place SIZE pseudorandom bytes into the buffer beginning at P, using
   the buffered ISAAC generator in ISAAC.  */

static void
readisaac (struct isaac *isaac, void *p, size_t size)
{
  size_t inbytes = isaac->buffered;

  while (true)
    {
      char *char_p = p;

      if (size <= inbytes)
        {
          memcpy (p, isaac->data.b + ISAAC_BYTES - inbytes, size);
          isaac->buffered = inbytes - size;
          return;
        }

      memcpy (p, isaac->data.b + ISAAC_BYTES - inbytes, inbytes);
      p = char_p + inbytes;
      size -= inbytes;

      /* If P is aligned, write to *P directly to avoid the overhead
         of copying from the buffer.  */
      if (ALIGNED_POINTER (p, isaac_word))
        {
          isaac_word *wp = p;
          while (ISAAC_BYTES <= size)
            {
              isaac_refill (&isaac->state, wp);
              wp += ISAAC_WORDS;
              size -= ISAAC_BYTES;
              if (size == 0)
                {
                  isaac->buffered = 0;
                  return;
                }
            }
          p = wp;
        }

      isaac_refill (&isaac->state, isaac->data.w);
      inbytes = ISAAC_BYTES;
    }
}


/* Consume random data from *S to generate a random buffer BUF of size
   SIZE.  */

void
randread (struct randread_source *s, void *buf, size_t size)
{
  if (s->source)
    readsource (s, buf, size);
  else
    readisaac (&s->buf.isaac, buf, size);
}


/* Clear *S so that it no longer contains undelivered random data, and
   deallocate any system resources associated with *S.  Return 0 if
   successful, a negative number (setting errno) if not (this is rare,
   but can occur in theory if there is an input error).  */

int
randread_free (struct randread_source *s)
{
  FILE *source = s->source;
  explicit_bzero (s, sizeof *s);
  free (s);
  return (source ? fclose (source) : 0);
}
