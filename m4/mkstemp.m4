#serial 1

# On some hosts (e.g., HP-UX 10.20, SunOS 4.1.4, Solaris 2.5.1), mkstemp has a
# silly limit that it can create no more than 26 files from a given template.
# Other systems lack mkstemp altogether.  On either type of system, arrange
# to use the replacement function.
AC_DEFUN([UTILS_FUNC_MKSTEMP],
[dnl
  AC_REPLACE_FUNCS(mkstemp)
  if test $ac_cv_func_mkstemp = no; then
    utils_cv_func_mkstemp_limitations=yes
  else
    AC_CACHE_CHECK([for mkstemp limitations],
      utils_cv_func_mkstemp_limitations,
      [
	AC_TRY_RUN([
#         include <stdlib.h>
	  int main ()
	  {
	    int i;
	    for (i = 0; i < 30; i++)
	      {
		char template[] = "conftestXXXXXX";
		int fd = mkstemp (template);
		if (fd == -1)
		  exit (1);
		close (fd);
	      }
	    exit (0);
	  }
	  ],
	utils_cv_func_mkstemp_limitations=no,
	utils_cv_func_mkstemp_limitations=yes,
	utils_cv_func_mkstemp_limitations=yes
	)
      ]
    )
  fi

  if test $utils_cv_func_mkstemp_limitations = yes; then
    AC_LIBOBJ(mkstemp)
    AC_LIBOBJ(tempname)
    AC_DEFINE(mkstemp, rpl_mkstemp,
      [Define to rpl_mkstemp if the replacement function should be used.])
  fi
])
