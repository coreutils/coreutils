/* iopoll.c  -- broken pipe detection (while waiting for input)
   Copyright (C) 1989-2022 Free Software Foundation, Inc.

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

#include <assert.h>

  /* poll(2) is needed on AIX (where 'select' gives a readable
     event immediately) and Solaris (where 'select' never gave
     a readable event).  Also use poll(2) on systems we know work
     and/or are already using poll (linux).  */

#if defined _AIX || defined __sun || defined __APPLE__ || \
    defined __linux__ || defined __ANDROID__
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
#include "iopoll.h"
#include "isapipe.h"


/* Wait for FDIN to become ready for reading or FDOUT to become a broken pipe.
   If either of those are -1, then they're not checked.  Set BLOCK to true
   to wait for an event, otherwise return the status immediately.
   Return 0 if not BLOCKing and there is no event on the requested descriptors.
   Return 0 if FDIN can be read() without blocking, or IOPOLL_BROKEN_OUTPUT if
   FDOUT becomes a broken pipe, otherwise IOPOLL_ERROR if there is a poll()
   or select() error.  */

extern int
iopoll (int fdin, int fdout, bool block)
{
#if IOPOLL_USES_POLL

  struct pollfd pfds[2] = {  /* POLLRDBAND needed for illumos, macOS.  */
    { .fd = fdin,  .events = POLLIN | POLLRDBAND, .revents = 0 },
    { .fd = fdout, .events = POLLRDBAND, .revents = 0 },
  };
  int ret = 0;

  while (0 <= ret || errno == EINTR)
    {
      ret = poll (pfds, 2, block ? -1 : 0);

      if (ret < 0)
        continue;
      if (ret == 0 && ! block)
        return 0;
      assert (0 < ret);
      if (pfds[0].revents) /* input available or pipe closed indicating EOF; */
        return 0;          /* should now be able to read() without blocking  */
      if (pfds[1].revents)            /* POLLERR, POLLHUP (or POLLNVAL) */
        return IOPOLL_BROKEN_OUTPUT;  /* output error or broken pipe    */
    }

#else  /* fall back to select()-based implementation */

  int nfds = (fdin > fdout ? fdin : fdout) + 1;
  int ret = 0;

  /* If fdout has an error condition (like a broken pipe) it will be seen
     as ready for reading.  Assumes fdout is not actually readable.  */
  while (0 <= ret || errno == EINTR)
    {
      fd_set rfds;
      FD_ZERO (&rfds);
      if (0 <= fdin)
        FD_SET (fdin, &rfds);
      if (0 <= fdout)
        FD_SET (fdout, &rfds);

      struct timeval delay = { .tv_sec = 0, .tv_usec = 0 };
      ret = select (nfds, &rfds, NULL, NULL, block ? NULL : &delay);

      if (ret < 0)
        continue;
      if (ret == 0 && ! block)
        return 0;
      assert (0 < ret);
      if (0 <= fdin && FD_ISSET (fdin, &rfds))   /* input available or EOF; */
        return 0;          /* should now be able to read() without blocking */
      if (0 <= fdout && FD_ISSET (fdout, &rfds)) /* equiv to POLLERR        */
        return IOPOLL_BROKEN_OUTPUT;      /* output error or broken pipe    */
    }

#endif
  return IOPOLL_ERROR;
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
