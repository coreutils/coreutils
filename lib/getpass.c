/* Copyright (C) 1992-2001, 2003 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#if _LIBC
# define HAVE_STDIO_EXT_H 1
#endif

#include <stdbool.h>

#include <stdio.h>
#if HAVE_STDIO_EXT_H
# include <stdio_ext.h>
#else
# define __fsetlocking(stream, type) /* empty */
#endif
#if !_LIBC
# include "getline.h"
#endif

#include <termios.h>
#include <unistd.h>

#if _LIBC
# include <wchar.h>
#endif

#if _LIBC
# define NOTCANCEL_MODE "c"
#else
# define NOTCANCEL_MODE
#endif

#if _LIBC
# define flockfile(s) _IO_flockfile (s)
# define funlockfile(s) _IO_funlockfile (s)
#else
# include "unlocked-io.h"
#endif

#if _LIBC
# include <bits/libc-lock.h>
#else
# define __libc_cleanup_push(function, arg) /* empty */
# define __libc_cleanup_pop(execute) /* empty */
#endif

#if !_LIBC
# define __getline getline
# define __tcgetattr tcgetattr
#endif

/* It is desirable to use this bit on systems that have it.
   The only bit of terminal state we want to twiddle is echoing, which is
   done in software; there is no need to change the state of the terminal
   hardware.  */

#ifndef TCSASOFT
# define TCSASOFT 0
#endif

static void
call_fclose (void *arg)
{
  if (arg != NULL)
    fclose (arg);
}

char *
getpass (const char *prompt)
{
  FILE *tty;
  FILE *in, *out;
  struct termios s, t;
  bool tty_changed;
  static char *buf;
  static size_t bufsize;
  ssize_t nread;

  /* Try to write to and read from the terminal if we can.
     If we can't open the terminal, use stderr and stdin.  */

  tty = fopen ("/dev/tty", "w+" NOTCANCEL_MODE);
  if (tty == NULL)
    {
      in = stdin;
      out = stderr;
    }
  else
    {
      /* We do the locking ourselves.  */
      __fsetlocking (tty, FSETLOCKING_BYCALLER);

      out = in = tty;
    }

  /* Make sure the stream we opened is closed even if the thread is
     canceled.  */
  __libc_cleanup_push (call_fclose, tty);

  flockfile (out);

  /* Turn echoing off if it is on now.  */

  if (__tcgetattr (fileno (in), &t) == 0)
    {
      /* Save the old one. */
      s = t;
      /* Tricky, tricky. */
      t.c_lflag &= ~(ECHO|ISIG);
      tty_changed = (tcsetattr (fileno (in), TCSAFLUSH|TCSASOFT, &t) == 0);
    }
  else
    tty_changed = false;

  /* Write the prompt.  */
#ifdef USE_IN_LIBIO
  if (_IO_fwide (out, 0) > 0)
    __fwprintf (out, L"%s", prompt);
  else
#endif
    fputs_unlocked (prompt, out);
  fflush_unlocked (out);

  /* Read the password.  */
  nread = __getline (&buf, &bufsize, in);

#if !_LIBC
  /* As far as is known, glibc doesn't need this no-op fseek.  */

  /* According to the C standard, input may not be followed by output
     on the same stream without an intervening call to a file
     positioning function.  Suppose in == out; then without this fseek
     call, on Solaris, HP-UX, AIX, OSF/1, the previous input gets
     echoed, whereas on IRIX, the following newline is not output as
     it should be.  POSIX imposes similar restrictions if fileno (in)
     == fileno (out).  The POSIX restrictions are tricky and change
     from POSIX version to POSIX version, so play it safe and invoke
     fseek even if in != out.  */
  fseek (out, 0, SEEK_CUR);
#endif

  if (buf != NULL)
    {
      if (nread < 0)
	buf[0] = '\0';
      else if (buf[nread - 1] == '\n')
	{
	  /* Remove the newline.  */
	  buf[nread - 1] = '\0';
	  if (tty_changed)
	    {
	      /* Write the newline that was not echoed.  */
#ifdef USE_IN_LIBIO
	      if (_IO_fwide (out, 0) > 0)
		putwc_unlocked (L'\n', out);
	      else
#endif
		putc_unlocked ('\n', out);
	    }
	}
    }

  /* Restore the original setting.  */
  if (tty_changed)
    (void) tcsetattr (fileno (in), TCSAFLUSH|TCSASOFT, &s);

  funlockfile (out);

  __libc_cleanup_pop (0);

  call_fclose (tty);

  return buf;
}
