#serial 103
dnl FIXME: when this goes back into automake, reset this to a small number

dnl From Jim Meyering.
dnl FIXME: this should migrate into libit.

dnl FIXME: when this goes back into automake, remove all jm_ prefixes

AC_DEFUN(jm_AM_FUNC_MKTIME,
[AC_REQUIRE([AC_HEADER_TIME])dnl
 AC_CHECK_HEADERS(sys/time.h)
 AC_CACHE_CHECK([for working mktime], jm_am_cv_func_working_mktime,
  [AC_TRY_RUN(
changequote(<<, >>)dnl
<</* Test program from Paul Eggert (eggert@twinsun.com)
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

/* Values we'll use to set the TZ environment variable.  */
static const char *const tz_strings[] = {
  NULL, "GMT0", "JST-9", "EST+3EDT+2,M10.1.0/00:00:00,M2.3.0/00:00:00"
};
#define N_STRINGS (sizeof (tz_strings) / sizeof (tz_strings[0]))

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

static void
irix_6_4_bug ()
{
  /* Based on code from Ariel Faigon.  */
  struct tm tm;
  tm.tm_year = 96;
  tm.tm_mon = 3;
  tm.tm_mday = 0;
  tm.tm_hour = 0;
  tm.tm_min = 0;
  tm.tm_sec = 0;
  tm.tm_isdst = -1;
  mktime (&tm);
  if (tm.tm_mon != 2 || tm.tm_mday != 31)
    exit (1);
}

int
main ()
{
  time_t t, delta;
  int i;

  for (time_t_max = 1; 0 < time_t_max; time_t_max *= 2)
    continue;
  time_t_max--;
  delta = time_t_max / 997; /* a suitable prime number */
  for (i = 0; i < N_STRINGS; i++)
    {
      if (tz_strings[i])
	putenv (tz_strings[i]);

      for (t = 0; t <= time_t_max - delta; t += delta)
	mktime_test (t);
      mktime_test ((time_t) 60 * 60);
      mktime_test ((time_t) 60 * 60 * 24);
    }
  irix_6_4_bug ();
  exit (0);
}
	      >>,
changequote([, ])dnl
	     jm_am_cv_func_working_mktime=yes, jm_am_cv_func_working_mktime=no,
	     dnl When crosscompiling, assume mktime is missing or broken.
	     jm_am_cv_func_working_mktime=no)
  ])
  if test $jm_am_cv_func_working_mktime = no; then
    LIBOBJS="$LIBOBJS mktime.o"
  fi
])
