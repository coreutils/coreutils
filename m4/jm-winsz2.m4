#serial 4

AC_DEFUN([jm_HEADER_TIOCGWINSZ_NEEDS_SYS_IOCTL],
[AC_REQUIRE([jm_HEADER_TIOCGWINSZ_IN_TERMIOS_H])
 AC_CACHE_CHECK([whether use of TIOCGWINSZ requires sys/ioctl.h],
	        jm_cv_sys_tiocgwinsz_needs_sys_ioctl_h,
  [jm_cv_sys_tiocgwinsz_needs_sys_ioctl_h=no

  if test $jm_cv_sys_tiocgwinsz_needs_termios_h = no; then
    AC_EGREP_CPP([yes],
    [#include <sys/types.h>
#     include <sys/ioctl.h>
#     ifdef TIOCGWINSZ
        yes
#     endif
    ], jm_cv_sys_tiocgwinsz_needs_sys_ioctl_h=yes)
  fi
  ])
  if test $jm_cv_sys_tiocgwinsz_needs_sys_ioctl_h = yes; then
    AC_DEFINE(GWINSZ_IN_SYS_IOCTL, 1,
      [Define if your system defines TIOCGWINSZ in sys/ioctl.h.])
  fi
])
