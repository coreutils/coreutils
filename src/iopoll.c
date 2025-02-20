/* iopoll.c -- broken pipe detection / non blocking output handling
   Copyright (C) 2022-2025 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.

   Written by Carl Edquist in collaboration with Arsen Arsenović.  */

#include <config.h>

/* poll(2) is needed on AIX (where 'select' gives a readable
   event immediately) and Solaris (where 'select' never gave
   a readable event).  Also use poll(2) on systems we know work
   and/or are already using poll (linux).  On macOS we know poll()
   doesn't work on 10.5, while it does work on >= 11 at least. */
#if defined _AIX || defined __sun \
    || defined __linux__ || defined __ANDROID__ \
    || __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__ >= 110000
# define IOPOLL_USES_POLL 1
  /* Check we've not enabled gnulib's poll module
     as that will emulate poll() in a way not
     currently compatible with our usage.  */
# if defined HAVE_POLL
#  error "gnulib's poll() replacement is currently incompatible"
# endif
#endif

#if IOPOLL_USES_POLL
# include <poll.h>
#else
# include <sys/select.h>
#endif

#include "system.h"
#include "assure.h"
#include "iopoll.h"
#include "isapipe.h"


/* BROKEN_OUTPUT selects the mode of operation of this function.
   If BROKEN_OUTPUT, wait for FDIN to become ready for reading
   or FDOUT to become a broken pipe.
   If !BROKEN_OUTPUT, wait for FDIN or FDOUT to become ready for writing.
   If either of those are -1, then they're not checked.  Set BLOCK to true
   to wait for an event, otherwise return the status immediately.
   Return 0 if not BLOCKing and there is no event on the requested descriptors.
   Return 0 if FDIN can be read() without blocking, or IOPOLL_BROKEN_OUTPUT if
   FDOUT becomes a broken pipe. If !BROKEN_OUTPUT return 0 if FDOUT writable.
   Otherwise return IOPOLL_ERROR if there is a poll() or select() error.  */

static int
iopoll_internal (int fdin, int fdout, bool block, bool broken_output)
{
  affirm (fdin != -1 || fdout != -1);

#if IOPOLL_USES_POLL
  struct pollfd pfds[2] = {  /* POLLRDBAND needed for illumos, macOS.  */
    { .fd = fdin,  .events = POLLIN | POLLRDBAND, .revents = 0 },
    { .fd = fdout, .events = POLLRDBAND, .revents = 0 },
  };
  int check_out_events = POLLERR | POLLHUP | POLLNVAL;
  int ret = 0;

  if (! broken_output)
    {
      pfds[0].events = pfds[1].events = POLLOUT;
      check_out_events = POLLOUT;
    }

  while (0 <= ret || errno == EINTR)
    {
      ret = poll (pfds, 2, block ? -1 : 0);

      if (ret < 0)
        continue;
      if (ret == 0 && ! block)
        return 0;
      affirm (0 < ret);
      if (pfds[0].revents) /* input available or pipe closed indicating EOF; */
        return 0;          /* should now be able to read() without blocking  */
      if (pfds[1].revents & check_out_events)
        return broken_output ? IOPOLL_BROKEN_OUTPUT : 0;
    }

#else  /* fall back to select()-based implementation */

  int nfds = (fdin > fdout ? fdin : fdout) + 1;
  int ret = 0;

  if (FD_SETSIZE < nfds)
    {
      errno = EINVAL;
      ret = -1;
    }

  /* If fdout has an error condition (like a broken pipe) it will be seen
     as ready for reading.  Assumes fdout is not actually readable.  */
  while (0 <= ret || errno == EINTR)
    {
      fd_set fds;
      FD_ZERO (&fds);
      if (0 <= fdin)
        FD_SET (fdin, &fds);
      if (0 <= fdout)
        FD_SET (fdout, &fds);

      struct timeval delay = {0};
      ret = select (nfds,
                    broken_output ? &fds : nullptr,
                    broken_output ? nullptr : &fds,
                    nullptr, block ? nullptr : &delay);

      if (ret < 0)
        continue;
      if (ret == 0 && ! block)
        return 0;
      affirm (0 < ret);
      if (0 <= fdin && FD_ISSET (fdin, &fds))    /* input available or EOF; */
        return 0;          /* should now be able to read() without blocking */
      if (0 <= fdout && FD_ISSET (fdout, &fds))  /* equiv to POLLERR        */
        return broken_output ? IOPOLL_BROKEN_OUTPUT : 0;
    }

#endif
  return IOPOLL_ERROR;
}

extern int
iopoll (int fdin, int fdout, bool block)
{
  return iopoll_internal (fdin, fdout, block, true);
}



/* Return true if fdin is relevant for iopoll().
   An fd is not relevant for iopoll() if it is always ready for reading,
   which is the case for a regular file or block device.  */

extern bool
iopoll_input_ok (int fdin)
{
  struct stat st;
  bool always_ready = fstat (fdin, &st) == 0
                      && (S_ISREG (st.st_mode)
                          || S_ISBLK (st.st_mode));
  return ! always_ready;
}

/* Return true if fdout is suitable for iopoll().
   Namely, fdout refers to a pipe.  */

extern bool
iopoll_output_ok (int fdout)
{
  return isapipe (fdout) > 0;
}

#ifdef EWOULDBLOCK
# define IS_EAGAIN(errcode) ((errcode) == EAGAIN || (errcode) == EWOULDBLOCK)
#else
# define IS_EAGAIN(errcode) ((errcode) == EAGAIN)
#endif

/* Inspect the errno of the previous syscall.
   On EAGAIN, wait for the underlying file descriptor to become writable.
   Return true, if EAGAIN has been successfully handled. */

static bool
fwait_for_nonblocking_write (FILE *f)
{
  if (! IS_EAGAIN (errno))
    /* non-recoverable write error */
    return false;

  int fd = fileno (f);
  if (fd == -1)
    goto fail;

  /* wait for the file descriptor to become writable */
  if (iopoll_internal (-1, fd, true, false) != 0)
    goto fail;

  /* successfully waited for the descriptor to become writable */
  clearerr (f);
  return true;

fail:
  errno = EAGAIN;
  return false;
}


/* wrapper for fclose() that also waits for F if non blocking.  */

extern bool
fclose_wait (FILE *f)
{
  for (;;)
    {
      if (fflush (f) == 0)
        break;

      if (! fwait_for_nonblocking_write (f))
        break;
    }

  return fclose (f) == 0;
}


/* wrapper for fwrite() that also waits for F if non blocking.  */

extern bool
fwrite_wait (char const *buf, ssize_t size, FILE *f)
{
  for (;;)
    {
      const size_t written = fwrite (buf, 1, size, f);
      size -= written;
      affirm (size >= 0);
      if (size <= 0)  /* everything written */
        return true;

      if (! fwait_for_nonblocking_write (f))
        return false;

      buf += written;
    }
}
