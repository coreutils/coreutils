/* error.c -- error handler for noninteractive utilities
   Copyright (C) 1990, 91, 92, 93, 94, 95, 96 Free Software Foundation, Inc.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

/* Written by David MacKenzie <djm@gnu.ai.mit.edu>.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>

#if HAVE_VPRINTF || HAVE_DOPRNT || _LIBC
# if __STDC__
#  include <stdarg.h>
#  define VA_START(args, lastarg) va_start(args, lastarg)
# else
#  include <varargs.h>
#  define VA_START(args, lastarg) va_start(args)
# endif
#else
# define va_alist a1, a2, a3, a4, a5, a6, a7, a8
# define va_dcl char *a1, *a2, *a3, *a4, *a5, *a6, *a7, *a8;
#endif

#if STDC_HEADERS || _LIBC
# include <stdlib.h>
# include <string.h>
#else
void exit ();
#endif

#ifndef _
# define _(String) String
#endif

/* If NULL, error will flush stdout, then print on stderr the program
   name, a colon and a space.  Otherwise, error will call this
   function without parameters instead.  */
void (*error_print_progname) (
#if __STDC__ - 0
			      void
#endif
			      );

/* This variable is incremented each time `error' is called.  */
unsigned int error_message_count;

#ifdef _LIBC
/* In the GNU C library, there is a predefined variable for this.  */

# define program_name program_invocation_name
# include <errno.h>

#else

/* The calling program should define program_name and set it to the
   name of the executing program.  */
extern char *program_name;

# if HAVE_STRERROR
#  ifndef strerror		/* On some systems, strerror is a macro */
char *strerror ();
#  endif
# else
static char *
private_strerror (errnum)
     int errnum;
{
  extern char *sys_errlist[];
  extern int sys_nerr;

  if (errnum > 0 && errnum <= sys_nerr)
    return sys_errlist[errnum];
  return _("Unknown system error");
}
#  define strerror private_strerror
# endif	/* HAVE_STRERROR */
#endif	/* _LIBC */

/* Print the program name and error message MESSAGE, which is a printf-style
   format string with optional args.
   If ERRNUM is nonzero, print its corresponding system error message.
   Exit with status STATUS if it is nonzero.  */
/* VARARGS */

void
#if defined(VA_START) && __STDC__
error (int status, int errnum, const char *message, ...)
#else
error (status, errnum, message, va_alist)
     int status;
     int errnum;
     char *message;
     va_dcl
#endif
{
#ifdef VA_START
  va_list args;
#endif

  if (error_print_progname)
    (*error_print_progname) ();
  else
    {
      fflush (stdout);
      fprintf (stderr, "%s: ", program_name);
    }

#ifdef VA_START
  VA_START (args, message);
# if HAVE_VPRINTF || _LIBC
  vfprintf (stderr, message, args);
# else
  _doprnt (message, args, stderr);
# endif
  va_end (args);
#else
  fprintf (stderr, message, a1, a2, a3, a4, a5, a6, a7, a8);
#endif

  ++error_message_count;
  if (errnum)
    fprintf (stderr, ": %s", strerror (errnum));
  putc ('\n', stderr);
  fflush (stderr);
  if (status)
    exit (status);
}

/* Sometimes we want to have at most one error per line.  This
   variable controls whether this mode is selected or not.  */
int error_one_per_line;

void
#if defined(VA_START) && __STDC__
error_at_line (int status, int errnum, const char *file_name,
	       unsigned int line_number, const char *message, ...)
#else
error_at_line (status, errnum, file_name, line_number, message, va_alist)
     int status;
     int errnum;
     const char *file_name;
     unsigned int line_number;
     char *message;
     va_dcl
#endif
{
#ifdef VA_START
  va_list args;
#endif

  if (error_one_per_line)
    {
      static const char *old_file_name;
      static unsigned int old_line_number;

      if (old_line_number == line_number &&
	  (file_name == old_file_name || !strcmp (old_file_name, file_name)))
	/* Simply return and print nothing.  */
	return;

      old_file_name = file_name;
      old_line_number = line_number;
    }

  if (error_print_progname)
    (*error_print_progname) ();
  else
    {
      fflush (stdout);
      fprintf (stderr, "%s:", program_name);
    }

  if (file_name != NULL)
    fprintf (stderr, "%s:%d: ", file_name, line_number);

#ifdef VA_START
  VA_START (args, message);
# if HAVE_VPRINTF || _LIBC
  vfprintf (stderr, message, args);
# else
  _doprnt (message, args, stderr);
# endif
  va_end (args);
#else
  fprintf (stderr, message, a1, a2, a3, a4, a5, a6, a7, a8);
#endif

  ++error_message_count;
  if (errnum)
    fprintf (stderr, ": %s", strerror (errnum));
  putc ('\n', stderr);
  fflush (stderr);
  if (status)
    exit (status);
}
