#serial 1

dnl From Jim Meyering

# If ssize_t is not defined in sys/types.h, define it to `int'.

AC_DEFUN(jm_TYPE_SSIZE_T,
  [AC_CACHE_CHECK(for ssize_t in sys/types.h, jm_ac_cv_type_ssize_t,
    [
      AC_EGREP_HEADER(ssize_t, sys/types.h,
		      jm_ac_cv_type_ssize_t=yes,
		      jm_ac_cv_type_ssize_t=no)
      if test $jm_ac_cv_type_ssize_t = no; then
	AC_DEFINE(ssize_t, int)
      fi
    ]
   )
  ]
)
