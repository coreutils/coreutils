/* Stack overflow handling.

   Copyright (C) 2002 Free Software Foundation, Inc.

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

/* Written by Paul Eggert.  */

/* NOTES:

   A program that uses alloca, dynamic arrays, or large local
   variables may extend the stack by more than a page at a time.  If
   so, when the stack overflows the operating system may not detect
   the overflow until the program uses the array, and this module may
   incorrectly report a program error instead of a stack overflow.

   To avoid this problem, allocate only small objects on the stack; a
   program should be OK if it limits single allocations to a page or
   less.  Allocate larger arrays in static storage, or on the heap
   (e.g., with malloc).  Yes, this is a pain, but we don't know of any
   better solution that is portable.

   No attempt has been made to deal with multithreaded applications.

   If ! HAVE_XSI_STACK_OVERFLOW_HEURISTIC, the current implementation
   assumes that, if the RLIMIT_STACK limit changes during execution,
   then c_stack_action is invoked immediately afterwards.  */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#ifndef __attribute__
# if __GNUC__ < 3 || __STRICT_ANSI__
#  define __attribute__(x)
# endif
#endif

#include "gettext.h"
#define _(msgid) gettext (msgid)

#include <errno.h>
#ifndef ENOTSUP
# define ENOTSUP EINVAL
#endif
#ifndef EOVERFLOW
# define EOVERFLOW EINVAL
#endif

#include <signal.h>
#if ! HAVE_STACK_T && ! defined stack_t
typedef struct sigaltstack stack_t;
#endif

#include <stdlib.h>
#include <string.h>

#if HAVE_SYS_RESOURCE_H
/* Include sys/time.h here, because...
   SunOS-4.1.x <sys/resource.h> fails to include <sys/time.h>.
   This gives "incomplete type" errors for ru_utime and tu_stime.  */
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# endif
# include <sys/resource.h>
#endif

#if HAVE_UCONTEXT_H
# include <ucontext.h>
#endif

#if HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifndef STDERR_FILENO
# define STDERR_FILENO 2
#endif

#if DEBUG
# include <stdio.h>
#endif

#include "c-stack.h"
#include "exitfail.h"

extern char *program_name;

/* The user-specified action to take when a SEGV-related program error
   or stack overflow occurs.  */
static void (* volatile segv_action) (int);

/* Translated messages for program errors and stack overflow.  Do not
   translate them in the signal handler, since gettext is not
   async-signal-safe.  */
static char const * volatile program_error_message;
static char const * volatile stack_overflow_message;

/* Output an error message, then exit with status EXIT_FAILURE if it
   appears to have been a stack overflow, or with a core dump
   otherwise.  This function is async-signal-safe.  */

static void die (int) __attribute__ ((noreturn));
static void
die (int signo)
{
  char const *message =
    signo ? program_error_message : stack_overflow_message;
  segv_action (signo);
  write (STDERR_FILENO, program_name, strlen (program_name));
  write (STDERR_FILENO, ": ", 2);
  write (STDERR_FILENO, message, strlen (message));
  write (STDERR_FILENO, "\n", 1);
  if (! signo)
    _exit (exit_failure);
  kill (getpid (), signo);
  abort ();
}

#if HAVE_SIGALTSTACK && HAVE_DECL_SIGALTSTACK

/* Direction of the C runtime stack.  This function is
   async-signal-safe.  */

# if STACK_DIRECTION
#  define find_stack_direction(ptr) STACK_DIRECTION
# else
static int
find_stack_direction (char const *addr)
{
  char dummy;
  return ! addr ? find_stack_direction (&dummy) : addr < &dummy ? 1 : -1;
}
# endif

# if HAVE_XSI_STACK_OVERFLOW_HEURISTIC
#  define get_stack_location(argv) 0
# else

#  if defined RLIMIT_STACK && defined _SC_PAGESIZE

/* Return the minimum machine address deducible from ARGV.  This
   includes the addresses of all the strings that ARGV points at, as
   well as the address of ARGV itself.  */

static char const *
min_address_from_argv (char * const *argv)
{
  char const *min = (char const *) argv;
  char const *p;
  while ((p = *argv++))
    if (p < min)
      min = p;
  return min;
}

/* Return the maximum machine address deducible from ARGV.  */

static char const *
max_address_from_argv (char * const *argv)
{
  char const *max = *argv;
  char const *max1;
  char const *p;
  while ((p = *argv++))
    if (max < p)
      max = p;
  max1 = (char const *) (argv + 1);
  return max && max1 < max ? max + strlen (max) + 1 : max1;
}
#  endif

/* The base and size of the stack, determined at startup.  */
static char const * volatile stack_base;
static size_t volatile stack_size;

/* Store the base and size of the stack into the static variables
   STACK_BASE and STACK_SIZE.  The base is the numerically lowest
   address in the stack.  Return -1 (setting errno) if this cannot be
   done.  */

static int
get_stack_location (char * const *argv)
{
#  if ! (defined RLIMIT_STACK && defined _SC_PAGESIZE)

  errno = ENOTSUP;
  return -1;

#  else

  struct rlimit rlimit;
  int r = getrlimit (RLIMIT_STACK, &rlimit);
  if (r == 0)
    {
      char const *base;
      size_t size = rlimit.rlim_cur;
      extern char **environ;
      size_t page_size = sysconf (_SC_PAGESIZE);
      int stack_direction = find_stack_direction (0);

#   if HAVE_GETCONTEXT && HAVE_DECL_GETCONTEXT
      ucontext_t context;
      if (getcontext (&context) == 0)
	{
	  base = context.uc_stack.ss_sp;
	  if (stack_direction < 0)
	    base -= size - context.uc_stack.ss_size;
	}
      else
#   endif
	{
	  if (stack_direction < 0)
	    {
	      char const *a = max_address_from_argv (argv);
	      char const *b = max_address_from_argv (environ);
	      base = (a < b ? b : a) - size;
	      base += - (size_t) base % page_size;
	    }
	  else
	    {
	      char const *a = min_address_from_argv (argv);
	      char const *b = min_address_from_argv (environ);
	      base = a < b ? a : b;
	      base -= (size_t) base % page_size;
	    }
	}

      if (size != rlimit.rlim_cur
	  || rlimit.rlim_cur < 0
	  || base + size < base
#   ifdef RLIM_SAVED_CUR
	  || rlimit.rlim_cur == RLIM_SAVED_CUR
#   endif
#   ifdef RLIM_SAVED_MAX
	  || rlimit.rlim_cur == RLIM_SAVED_MAX
#   endif
#   ifdef RLIM_INFINITY
	  || rlimit.rlim_cur == RLIM_INFINITY
#   endif
	  )
	{
	  errno = EOVERFLOW;
	  return -1;
	}

      stack_base = base;
      stack_size = size;

#   if DEBUG
      fprintf (stderr, "get_stack_location base=%p size=%lx\n",
	       base, (unsigned long) size);
#   endif
    }

  return r;

#  endif
}
# endif

/* Storage for the alternate signal stack.  */
static union
{
  char buffer[SIGSTKSZ];

  /* These other members are for proper alignment.  There's no
     standard way to guarantee stack alignment, but this seems enough
     in practice.  */
  long double ld;
  long l;
  void *p;
} alternate_signal_stack;

# if defined SA_ONSTACK && defined SA_SIGINFO && defined _SC_PAGESIZE

/* Handle a segmentation violation and exit.  This function is
   async-signal-safe.  */

static void segv_handler (int, siginfo_t *, void *) __attribute__((noreturn));
static void
segv_handler (int signo, siginfo_t *info,
	      void *context __attribute__ ((unused)))
{
  /* Clear SIGNO if it seems to have been a stack overflow.  */
  if (0 < info->si_code)
    {
      /* If the faulting address is within the stack, or within one
	 page of the stack end, assume that it is a stack
	 overflow.  */
#  if HAVE_XSI_STACK_OVERFLOW_HEURISTIC
      ucontext_t const *user_context = context;
      char const *stack_base = user_context->uc_stack.ss_sp;
      size_t stack_size = user_context->uc_stack.ss_size;
#  endif
      char const *faulting_address = info->si_addr;
      size_t s = faulting_address - stack_base;
      size_t page_size = sysconf (_SC_PAGESIZE);
      if (find_stack_direction (0) < 0)
	s += page_size;
      if (s < stack_size + page_size)
	signo = 0;

#  if DEBUG
      {
	char buf[1024];
	sprintf (buf,
		 "segv_handler fault=%p base=%p size=%lx page=%lx signo=%d\n",
		 faulting_address, stack_base, (unsigned long) stack_size,
		 (unsigned long) page_size, signo);
	write (STDERR_FILENO, buf, strlen (buf));
      }
#  endif
    }

  die (signo);
}
# endif

static void
null_action (int signo __attribute__ ((unused)))
{
}

/* Assuming ARGV is the argument vector of `main', set up ACTION so
   that it is invoked on C stack overflow.  Return -1 (setting errno)
   if this cannot be done.

   When ACTION is called, it is passed an argument equal to SIGSEGV
   for a segmentation violation that does not appear related to stack
   overflow, and is passed zero otherwise.

   A null ACTION acts like an action that does nothing.

   ACTION must be async-signal-safe.  ACTION together with its callees
   must not require more than SIGSTKSZ bytes of stack space.  */

int
c_stack_action (char * const *argv __attribute__ ((unused)),
		void (*action) (int))
{
  int r = get_stack_location (argv);
  if (r != 0)
    return r;

  {
    stack_t st;
    st.ss_flags = 0;
    st.ss_sp = alternate_signal_stack.buffer;
    st.ss_size = sizeof alternate_signal_stack.buffer;
    r = sigaltstack (&st, 0);
    if (r != 0)
      return r;
  }

  segv_action = action ? action : null_action;
  program_error_message = _("program error");
  stack_overflow_message = _("stack overflow");

  {
# if ! (defined SA_ONSTACK && defined SA_SIGINFO && defined _SC_PAGESIZE)
    return signal (SIGSEGV, die) == SIG_ERR ? -1 : 0;
# else
    struct sigaction act;
    sigemptyset (&act.sa_mask);

    /* POSIX 1003.1-2001 says SA_RESETHAND implies SA_NODEFER, but
       this is not true on Solaris 8 at least.  It doesn't hurt to use
       SA_NODEFER here, so leave it in.  */
    act.sa_flags = SA_NODEFER | SA_ONSTACK | SA_RESETHAND | SA_SIGINFO;

    act.sa_sigaction = segv_handler;

    return sigaction (SIGSEGV, &act, 0);
# endif
  }
}

#else /* ! (HAVE_SIGALTSTACK && HAVE_DECL_SIGALTSTACK) */

int
c_stack_action (char * const *argv __attribute__ ((unused)),
		void (*action) (int)  __attribute__ ((unused)))
{
  errno = ENOTSUP;
  return -1;
}

#endif



#if DEBUG

int volatile exit_failure;

static long
recurse (char *p)
{
  char array[500];
  array[0] = 1;
  return *p + recurse (array);
}

char *program_name;

int
main (int argc __attribute__ ((unused)), char **argv)
{
  program_name = argv[0];
  fprintf (stderr, "The last line of output should be \"stack overflow\".\n");
  if (c_stack_action (argv, 0) == 0)
    return recurse ("\1");
  perror ("c_stack_action");
  return 1;
}

#endif /* DEBUG */

/*
Local Variables:
compile-command: "gcc -DDEBUG -DHAVE_CONFIG_H -I.. -g -O -Wall -W c-stack.c"
End:
*/
