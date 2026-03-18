/* yes - output a string repeatedly until killed
   Copyright (C) 1991-2026 Free Software Foundation, Inc.

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

/* David MacKenzie <djm@gnu.ai.mit.edu> */

#include <config.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/uio.h>

#include "system.h"

#include "alignalloc.h"
#include "full-write.h"
#include "isapipe.h"
#include "long-options.h"
#include "unistd--.h"

/* The official name of this program (e.g., no 'g' prefix).  */
#define PROGRAM_NAME "yes"

#define AUTHORS proper_name ("David MacKenzie")

void
usage (int status)
{
  if (status != EXIT_SUCCESS)
    emit_try_help ();
  else
    {
      printf (_("\
Usage: %s [STRING]...\n\
  or:  %s OPTION\n\
"),
              program_name, program_name);

      fputs (_("\
Repeatedly output a line with all specified STRING(s), or 'y'.\n\
\n\
"), stdout);
      oputs (HELP_OPTION_DESCRIPTION);
      oputs (VERSION_OPTION_DESCRIPTION);
      emit_ancillary_info (PROGRAM_NAME);
    }
  exit (status);
}

/* Fill DEST[0..BUFSIZE-1] with repeated copies of SRC[0..SRCSIZE-1],
   doubling the copy size each iteration.  DEST may equal SRC.  */

static void
repeat_pattern (char *dest, char const *src, idx_t srcsize, idx_t bufsize)
{
  if (dest != src)
    memcpy (dest, src, srcsize);
  for (idx_t filled = srcsize; filled < bufsize; )
    {
      idx_t chunk = MIN (filled, bufsize - filled);
      memcpy (dest + filled, dest, chunk);
      filled += chunk;
    }
}

#if HAVE_SPLICE

/* Empirically determined pipe size for best throughput.
   Needs to be <= /proc/sys/fs/pipe-max-size  */
enum { SPLICE_PIPE_SIZE = 512 * 1024 };

/* Enlarge a pipe towards SPLICE_PIPE_SIZE and return the actual
   capacity as a quarter of the pipe size (the empirical sweet spot
   for vmsplice throughput), rounded down to a multiple of COPYSIZE.
   Return 0 if the result would be smaller than COPYSIZE.  */

static idx_t
pipe_splice_size (int fd, idx_t copysize)
{
  int pipe_cap = 0;
# if defined F_SETPIPE_SZ && defined F_GETPIPE_SZ
  if ((pipe_cap = fcntl (fd, F_SETPIPE_SZ, SPLICE_PIPE_SIZE)) < 0)
    pipe_cap = fcntl (fd, F_GETPIPE_SZ);
# endif
  if (pipe_cap <= 0)
    pipe_cap = 64 * 1024;

  size_t buf_cap = pipe_cap / 4;
  return buf_cap / copysize * copysize;
}

#endif

/* Repeatedly write the COPYSIZE-byte pattern in BUF to standard output
   using vmsplice/splice zero-copy I/O.  Since the data never varies,
   SPLICE_F_GIFT tells the kernel the pages will not be modified.

   Return TRUE if splice I/O was used (caller should check errno and
   report any error).  Return FALSE if splice could not be used.  */

static bool
splice_write (MAYBE_UNUSED char const *buf, MAYBE_UNUSED idx_t copysize)
{
  bool output_started = false;
#if HAVE_SPLICE
  idx_t page_size = getpagesize ();

  bool stdout_is_pipe = isapipe (STDOUT_FILENO) > 0;

  /* Determine buffer size: enlarge the target pipe,
     then use 1/4 of actual capacity as the transfer size.  */
  int pipefd[2] = { -1, -1 };
  idx_t splice_bufsize;
  char *splice_buf = NULL;

  if (stdout_is_pipe)
    splice_bufsize = pipe_splice_size (STDOUT_FILENO, copysize);
  else
    {
      if (pipe2 (pipefd, 0) < 0)
        return false;
      splice_bufsize = pipe_splice_size (pipefd[0], copysize);
    }

  if (splice_bufsize == 0)
    goto done;

  /* Allocate page-aligned buffer for vmsplice.
     Needed with SPLICE_F_GIFT, but generally good for performance.  */
  if (! (splice_buf = alignalloc (page_size, splice_bufsize)))
    goto done;

  repeat_pattern (splice_buf, buf, copysize, splice_bufsize);

  /* For the pipe case, vmsplice directly to stdout.
     For the non-pipe case, vmsplice into the intermediate pipe
     and then splice from it to stdout.  */
  int vmsplice_fd = stdout_is_pipe ? STDOUT_FILENO : pipefd[1];

  for (;;)
    {
      struct iovec iov = { .iov_base = splice_buf,
                           .iov_len = splice_bufsize };

      while (iov.iov_len > 0)
        {
         /* Use SPLICE_F_{GIFT,MOVE} to allow the kernel to take references
            to the pages.  I.e., we're indicating we won't make changes.
            SPLICE_F_GIFT is only appropriate for full pages.  */
          unsigned int flags = iov.iov_len % page_size ? 0 : SPLICE_F_GIFT;
          ssize_t n = vmsplice (vmsplice_fd, &iov, 1, flags);
          if (n <= 0)
            goto done;
          if (stdout_is_pipe)
            output_started = true;
          iov.iov_base = (char *) iov.iov_base + n;
          iov.iov_len -= n;
        }

      /* For non-pipe stdout, drain intermediate pipe to stdout.  */
      if (! stdout_is_pipe)
        {
          idx_t remaining = splice_bufsize;
          while (remaining > 0)
            {
              ssize_t s = splice (pipefd[0], NULL, STDOUT_FILENO, NULL,
                                  remaining, SPLICE_F_MOVE);
              if (s <= 0)
                goto done;
              output_started = true;
              remaining -= s;
            }
        }
    }

done:
  if (pipefd[0] >= 0)
    {
      int saved_errno = errno;
      close (pipefd[0]);
      close (pipefd[1]);
      errno = saved_errno;
    }
  alignfree (splice_buf);
#endif
  return output_started;
}

int
main (int argc, char **argv)
{
  initialize_main (&argc, &argv);
  set_program_name (argv[0]);
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  parse_gnu_standard_options_only (argc, argv, PROGRAM_NAME, PACKAGE_NAME,
                                   Version, true, usage, AUTHORS,
                                   (char const *) NULL);

  char **operands = argv + optind;
  char **operand_lim = argv + argc;
  if (optind == argc)
    *operand_lim++ = bad_cast ("y");

  /* Buffer data locally once, rather than having the
     large overhead of stdio buffering each item.  */
  size_t bufalloc = 0;
  bool reuse_operand_strings = true;
  char **operandp = operands;
  do
    {
      size_t operand_len = strlen (*operandp);
      bufalloc += operand_len + 1;
      if (operandp + 1 < operand_lim
          && *operandp + operand_len + 1 != operandp[1])
        reuse_operand_strings = false;
    }
  while (++operandp < operand_lim);

  /* Improve performance by using a buffer size greater than BUFSIZ / 2.  */
  if (bufalloc <= BUFSIZ / 2)
    {
      bufalloc = BUFSIZ;
      reuse_operand_strings = false;
    }

#if defined __CHERI__
  /* Cheri capability bounds do not allow for this.  */
  reuse_operand_strings = false;
#endif

  /* Fill the buffer with one copy of the output.  If possible, reuse
     the operands strings; this wins when the buffer would be large.  */
  char *buf = reuse_operand_strings ? *operands : xmalloc (bufalloc);
  size_t bufused = 0;
  operandp = operands;
  do
    {
      size_t operand_len = strlen (*operandp);
      if (! reuse_operand_strings)
        memcpy (buf + bufused, *operandp, operand_len);
      bufused += operand_len;
      buf[bufused++] = ' ';
    }
  while (++operandp < operand_lim);
  buf[bufused - 1] = '\n';

  idx_t copysize = bufused;

  /* Repeatedly output the buffer until there is a write error; then fail.
     Do a minimal write first to check output with minimal set up cost.
     If successful then set up for efficient repetition.  */
  if (full_write (STDOUT_FILENO, buf, copysize) == copysize
      && splice_write (buf, copysize) == 0)
    {
      /* If a larger buffer was allocated, fill it by repeated copies.  */
      bufused = bufalloc / copysize * copysize;
      if (bufused > copysize)
        repeat_pattern (buf, buf, copysize, bufused);
      while (full_write (STDOUT_FILENO, buf, bufused) == bufused)
        continue;
    }

  error (0, errno, _("standard output"));
  main_exit (EXIT_FAILURE);
}
