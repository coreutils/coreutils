#serial 1
# Check whether getcwd can return a path longer than PATH_MAX.
# If not, arrange to compile the wrapper function.
# From Jim Meyering

AC_DEFUN([GL_FUNC_GETCWD_ROBUST],
[
  AC_CACHE_CHECK([whether getcwd can return a path longer than PATH_MAX],
                 utils_cv_func_getcwd_robust,
  [
  # Arrange for deletion of the temporary directory this test creates.
  ac_clean_files="$ac_clean_files confdir3"
  AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifndef CHAR_BIT
# define CHAR_BIT 8
#endif

/* The extra casts work around common compiler bugs.  */
#define TYPE_SIGNED(t) (! ((t) 0 < (t) -1))
/* The outer cast is needed to work around a bug in Cray C 5.0.3.0.
   It is necessary at least when t == time_t.  */
#define TYPE_MINIMUM(t) ((t) (TYPE_SIGNED (t) \
			      ? ~ (t) 0 << (sizeof (t) * CHAR_BIT - 1) : (t) 0))
#define TYPE_MAXIMUM(t) ((t) (~ (t) 0 - TYPE_MINIMUM (t)))

#ifndef INT_MAX
# define INT_MAX TYPE_MAXIMUM (int)
#endif

#ifndef PATH_MAX
# define PATH_MAX 40000
#endif

/* The length of this name must be 8.  */
#define DIR_NAME "confdir3"

int
main ()
{
  /* The '9' comes from strlen (DIR_NAME) + 1.  */
#if INT_MAX - 9 <= PATH_MAX
  /* FIXME: Assuming there's a system for which this is true -- Hurd?,
     this should be done in a compile test.  */
  exit (0);
#else
  char buf[PATH_MAX + 20];
  char *cwd = getcwd (buf, PATH_MAX);
  size_t cwd_len;
  int fail = 0;
  size_t n_chdirs = 0;

  if (cwd == NULL)
    exit (1);

  cwd_len = strlen (cwd);

  while (1)
    {
      char *c;
      size_t len;

      cwd_len += 1 + strlen (DIR_NAME);
      if (mkdir (DIR_NAME, 0700) < 0
	  || chdir (DIR_NAME) < 0
	  || (c = getcwd (buf, PATH_MAX)) == NULL
	  || (len = strlen (c)) != cwd_len)
	{
	  fail = 1;
	  break;
	}
      ++n_chdirs;
      if (PATH_MAX < len)
	break;
    }

  /* Leaving behind such a deep directory is not polite.
     So clean up here, right away, even though the driving
     shell script would also clean up.  */
  {
    size_t i;

    /* Unlink first, in case the chdir failed.  */
    unlink (DIR_NAME);
    for (i = 0; i <= n_chdirs; i++)
      {
	if (chdir ("..") < 0)
	  break;
	rmdir (DIR_NAME);
      }
  }

  exit (fail);
#endif
}
  ]])],
       [utils_cv_func_getcwd_robust=yes],
       [utils_cv_func_getcwd_robust=no],
       [utils_cv_func_getcwd_robust=no])])

  if test $utils_cv_func_getcwd_robust = yes; then
    AC_LIBOBJ(getcwd)
    AC_DEFINE(getcwd, rpl_getcwd,
      [Define to rpl_getcwd if the wrapper function should be used.])
  fi
])
