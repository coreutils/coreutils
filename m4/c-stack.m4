# Check prerequisites for compiling lib/c-stack.c.

# Copyright (C) 2002, 2003 Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA.

# Written by Paul Eggert.

AC_DEFUN([AC_SYS_XSI_STACK_OVERFLOW_HEURISTIC],
  [# for STACK_DIRECTION
   AC_REQUIRE([AC_FUNC_ALLOCA])
   AC_CHECK_FUNCS(setrlimit)

   AC_CACHE_CHECK([for working C stack overflow detection],
     ac_cv_sys_xsi_stack_overflow_heuristic,
     [AC_TRY_RUN(
	[
	 #include <unistd.h>
	 #include <signal.h>
	 #include <ucontext.h>
	 #if HAVE_SETRLIMIT
	 # include <sys/types.h>
	 # include <sys/time.h>
	 # include <sys/resource.h>
	 #endif

	 static union
	 {
	   char buffer[SIGSTKSZ];
	   long double ld;
	   long u;
	   void *p;
	 } alternate_signal_stack;

	 #if STACK_DIRECTION
	 # define find_stack_direction(ptr) STACK_DIRECTION
	 #else
	 static int
	 find_stack_direction (char const *addr)
	 {
	   char dummy;
	   return (! addr ? find_stack_direction (&dummy)
		   : addr < &dummy ? 1 : -1);
	 }
	 #endif

	 static void
	 segv_handler (int signo, siginfo_t *info, void *context)
	 {
	   if (0 < info->si_code)
	     {
	       ucontext_t const *user_context = context;
	       char const *stack_min = user_context->uc_stack.ss_sp;
	       size_t stack_size = user_context->uc_stack.ss_size;
	       char const *faulting_address = info->si_addr;
	       size_t s = faulting_address - stack_min;
	       size_t page_size = sysconf (_SC_PAGESIZE);
	       if (find_stack_direction (0) < 0)
		 s += page_size;
	       if (s < stack_size + page_size)
		 _exit (0);
	     }

	   _exit (1);
	 }

	 static int
	 c_stack_action (void)
	 {
	   stack_t st;
	   struct sigaction act;
	   int r;

	   st.ss_flags = 0;
	   st.ss_sp = alternate_signal_stack.buffer;
	   st.ss_size = sizeof alternate_signal_stack.buffer;
	   r = sigaltstack (&st, 0);
	   if (r != 0)
	     return r;

	   sigemptyset (&act.sa_mask);
	   act.sa_flags = SA_NODEFER | SA_ONSTACK | SA_RESETHAND | SA_SIGINFO;
	   act.sa_sigaction = segv_handler;
	   return sigaction (SIGSEGV, &act, 0);
	 }

	 static int
	 recurse (char *p)
	 {
	   char array[500];
	   array[0] = 1;
	   return *p + recurse (array);
	 }

	 int
	 main (void)
	 {
	   #if HAVE_SETRLIMIT && defined RLIMIT_STACK
	   /* Before starting the endless recursion, try to be friendly
	      to the user's machine.  On some Linux 2.2.x systems, there
	      is no stack limit for user processes at all.  We don't want
	      to kill such systems.  */
	   struct rlimit rl;
	   rl.rlim_cur = rl.rlim_max = 0x100000; /* 1 MB */
	   setrlimit (RLIMIT_STACK, &rl);
	   #endif

	   c_stack_action ();
	   return recurse ("\1");
	 }
	],
	[ac_cv_sys_xsi_stack_overflow_heuristic=yes],
	[ac_cv_sys_xsi_stack_overflow_heuristic=no],
	[ac_cv_sys_xsi_stack_overflow_heuristic=cross-compiling])])

   if test $ac_cv_sys_xsi_stack_overflow_heuristic = yes; then
     AC_DEFINE(HAVE_XSI_STACK_OVERFLOW_HEURISTIC, 1,
       [Define to 1 if extending the stack slightly past the limit causes
	a SIGSEGV, and an alternate stack can be established with sigaltstack,
	and the signal handler is passed a context that specifies the
	run time stack.  This behavior is defined by POSIX 1003.1-2001
        with the X/Open System Interface (XSI) option
	and is a standardized way to implement a SEGV-based stack
        overflow detection heuristic.])
   fi])


AC_DEFUN([jm_PREREQ_C_STACK],
  [AC_REQUIRE([AC_SYS_XSI_STACK_OVERFLOW_HEURISTIC])

   # for STACK_DIRECTION
   AC_REQUIRE([AC_FUNC_ALLOCA])

   AC_CHECK_FUNCS(getcontext sigaltstack)
   AC_CHECK_DECLS([getcontext], , , [#include <ucontext.h>])
   AC_CHECK_DECLS([sigaltstack], , , [#include <signal.h>])

   AC_CHECK_HEADERS(sys/resource.h ucontext.h unistd.h)

   AC_CHECK_TYPES([stack_t], , , [#include <signal.h>])])
