dnl From Jim Meyering.
dnl FIXME: this should migrate into libit.

AC_DEFUN(AM_FUNC_MKTIME,
[AC_REQUIRE([AC_HEADER_TIME])dnl
 AC_CHECK_HEADERS(sys/time.h)
 AC_CACHE_CHECK([for working mktime], am_cv_func_working_mktime,
  [AC_TRY_RUN([/* Test program from Paul Eggert (eggert@twinsun.com)
   and Tony Leneis (tony@plaza.ds.adp.com).  */
#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

static time_t time_t_max;

static void
mktime_test (now)
     time_t now;
{
  if (mktime (localtime (&now)) != now)
    exit (1);
  now = time_t_max - now;
  if (mktime (localtime (&now)) != now)
    exit (1);
}

int
main ()
{
  time_t t, delta;
  for (time_t_max = 1; 0 < time_t_max; time_t_max *= 2)
    continue;
  time_t_max--;
  delta = time_t_max / 997; /* a suitable prime number */
  for (t = 0; t <= time_t_max - delta; t += delta)
    mktime_test (t);
  mktime_test ((time_t) 60 * 60);
  mktime_test ((time_t) 60 * 60 * 24);
  exit (0);
}
	      ],
	     am_cv_func_working_mktime=yes, am_cv_func_working_mktime=no,
	     dnl When crosscompiling, assume mktime is missing or broken.
	     am_cv_func_working_mktime=no)
  ])
  if test $am_cv_func_working_mktime = no; then
    LIBOBJS="$LIBOBJS mktime.o"
  fi
])
