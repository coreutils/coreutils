dnl From Jim Meyering.
#serial 3
AC_DEFUN(jm_HEADER_TIOCGWINSZ_IN_TERMIOS_H,
[AC_REQUIRE([AM_SYS_POSIX_TERMIOS])
 AC_CACHE_CHECK([whether use of TIOCGWINSZ requires termios.h],
	        jm_cv_sys_tiocgwinsz_needs_termios_h,
  [jm_cv_sys_tiocgwinsz_needs_termios_h=no

   if test $am_cv_sys_posix_termios = yes; then
     AC_EGREP_CPP([yes],
     [#include <sys/types.h>
#      include <termios.h>
#      ifdef TIOCGWINSZ
         yes
#      endif
     ], jm_cv_sys_tiocgwinsz_needs_termios_h=yes)
   fi
  ])
])

AC_DEFUN(jm_WINSIZE_IN_PTEM,
  [AC_CHECK_HEADER([sys/ptem.h],
		   AC_DEFINE(WINSIZE_IN_PTEM, 1,
      [Define if your system defines `struct winsize' in sys/ptem.h.]))
  ]
)
