#serial 1

# On some systems (e.g., HPUX-10.20), mkstemp has the silly limit that it
# can create no more than 26 files from a given template.  Other systems
# lack mkstemp altogether.  On either type of system, arrange to use the
# replacement function.
AC_DEFUN([UTILS_FUNC_MKSTEMP],
[dnl
  AC_REPLACE_FUNCS(mkstemp)
  if test $ac_cv_func_mkstemp = no; then
    utils_cv_func_mkstemp_limitations=yes
  else
    AC_CACHE_CHECK([for mkstemp limitations],
      utils_cv_func_mkstemp_limitations,
      [
	utils_tmpdir_mkstemp=mkst-$$$$
	# Arrange for deletion-upon-exception of this temporary directory.
	ac_clean_files="$ac_clean_files $utils_tmpdir_mkstemp"
	mkdir $utils_tmpdir_mkstemp

	AC_TRY_RUN([
#         include <stdlib.h>
	  int main ()
	  {
	    int i;
	    for (i = 0; i < 30; i++)
	      {
		char template[] = "$utils_tmpdir_mkstemp/aXXXXXX";
		if (mkstemp (template) == -1)
		  exit (1);
	      }
	    exit (0);
	  }
	  ],
	utils_cv_func_mkstemp_limitations=no,
	utils_cv_func_mkstemp_limitations=yes,
	utils_cv_func_mkstemp_limitations=yes
	)

	rm -rf $utils_tmpdir_mkstemp
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
