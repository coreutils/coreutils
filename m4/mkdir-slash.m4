#serial 1

# On some systems, mkdir ("foo/", 0700) fails because of the trailing slash.
# On such systems, arrange to use a wrapper function that removes any
# trailing slashes.
AC_DEFUN([UTILS_FUNC_MKDIR_TRAILING_SLASH],
[dnl
  AC_CACHE_CHECK([whether mkdir fails due to a trailing slash],
    utils_cv_func_mkdir_trailing_slash_bug,
    [
      # Arrange for deletion of the temporary directory this test might create.
      ac_clean_files="$ac_clean_files confdir-slash"
      AC_TRY_RUN([
#       include <sys/types.h>
#       include <sys/stat.h>
#       include <stdlib.h>
	int main ()
	{
	  rmdir ("confdir-slash");
	  exit (mkdir ("confdir-slash/", 0700));
	}
	],
      utils_cv_func_mkdir_trailing_slash_bug=no,
      utils_cv_func_mkdir_trailing_slash_bug=yes,
      utils_cv_func_mkdir_trailing_slash_bug=yes
      )
    ]
  )

  if test $utils_cv_func_mkdir_trailing_slash_bug = yes; then
    AC_LIBOBJ(mkdir)
    AC_DEFINE(mkdir, rpl_mkdir,
      [Define to rpl_mkdir if the replacement function should be used.])
  fi
])
