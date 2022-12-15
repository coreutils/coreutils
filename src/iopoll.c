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

  /* poll(2) is needed on AIX (where 'select' gives a readable
     event immediately) and Solaris (where 'select' never gave
     a readable event).  Also use poll(2) on systems we know work
     and/or are already using poll (linux).  */

#if defined _AIX || defined __sun || defined __APPLE__ || \
    defined __linux__ || defined __ANDROID__
# define IOPOLL_USES_POLL 1
#endif

#if IOPOLL_USES_POLL
# include <poll.h>
#else
# include <sys/select.h>
#endif

#include "system.h"
#include "iopoll.h"
#include "isapipe.h"


/* Wait for fdin to become ready for reading or fdout to become a broken pipe.
   Return 0 if fdin can be read() without blocking, or IOPOLL_BROKEN_OUTPUT if
   fdout becomes a broken pipe, otherwise IOPOLL_ERROR if there is a poll()
   or select() error.  */

#if IOPOLL_USES_POLL

extern int
iopoll (int fdin, int fdout)
{
  struct pollfd pfds[2] = {  /* POLLRDBAND needed for illumos, macOS.  */
    { .fd = fdin,  .events = POLLIN | POLLRDBAND, .revents = 0 },
    { .fd = fdout, .events = POLLRDBAND, .revents = 0 },
  };

  while (poll (pfds, 2, -1) > 0 || errno == EINTR)
    {
      if (errno == EINTR)
        continue;
      if (pfds[0].revents) /* input available or pipe closed indicating EOF; */
        return 0;          /* should now be able to read() without blocking  */
      if (pfds[1].revents)            /* POLLERR, POLLHUP (or POLLNVAL) */
        return IOPOLL_BROKEN_OUTPUT;  /* output error or broken pipe    */
    }
  return IOPOLL_ERROR;  /* poll error */
}

#else  /* fall back to select()-based implementation */

extern int
iopoll (int fdin, int fdout)
{
  int nfds = (fdin > fdout ? fdin : fdout) + 1;
  int ret = 0;

  /* If fdout has an error condition (like a broken pipe) it will be seen
     as ready for reading.  Assumes fdout is not actually readable.  */
  while (ret >= 0 || errno == EINTR)
    {
      fd_set rfds;
      FD_ZERO (&rfds);
      FD_SET (fdin, &rfds);
      FD_SET (fdout, &rfds);
      ret = select (nfds, &rfds, NULL, NULL, NULL);

      if (ret < 0)
        continue;
      if (FD_ISSET (fdin, &rfds))  /* input available or EOF; should now */
        return 0;                  /* be able to read() without blocking */
      if (FD_ISSET (fdout, &rfds))     /* POLLERR, POLLHUP (or POLLIN)   */
        return IOPOLL_BROKEN_OUTPUT;   /* output error or broken pipe    */
    }
  return IOPOLL_ERROR;  /* select error */
}

#endif


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
