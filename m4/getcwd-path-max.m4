#serial 2
# Check whether getcwd has the bug that it succeeds for a working directory
# longer than PATH_MAX, yet returns a truncated directory name.
# If so, arrange to compile the wrapper function.

# This is necessary for at least GNU libc on linux-2.4.19 and 2.4.20.
# I've heard that this is due to a Linux kernel bug, and that it has
# been fixed between 2.4.21-pre3 and 2.4.21-pre4.  */

# From Jim Meyering

AC_DEFUN([GL_FUNC_GETCWD_PATH_MAX],
[
  AC_CACHE_CHECK([whether getcwd properly handles paths longer than PATH_MAX],
                 gl_cv_func_getcwd_vs_path_max,
  [
  AC_CHECK_DECLS([getcwd])
  # Arrange for deletion of the temporary directory this test creates.
  ac_clean_files="$ac_clean_files confdir3"
  AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>

/* Don't get link errors because mkdir is redefined to rpl_mkdir.  */
#undef mkdir

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
/* There might be a better way to handle this case, but note:
   - the value shouldn't be anywhere near INT_MAX, and
   - the value shouldn't be so big that the local declaration, below,
   blows the stack.  */
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
      /* If mkdir or chdir fails, be pessimistic and consider that
	 as a failure, too.  */
      if (mkdir (DIR_NAME, 0700) < 0 || chdir (DIR_NAME) < 0)
	{
	  fail = 1;
	  break;
	}
      if ((c = getcwd (buf, PATH_MAX)) == NULL)
        {
	  /* This allows any failure to indicate there is no bug.
	     FIXME: check errno?  */
	  break;
	}
      if ((len = strlen (c)) != cwd_len)
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
       [gl_cv_func_getcwd_vs_path_max=yes],
       [gl_cv_func_getcwd_vs_path_max=no],
       [gl_cv_func_getcwd_vs_path_max=no])])

  if test $gl_cv_func_getcwd_vs_path_max = yes; then
    AC_LIBOBJ(getcwd)
    AC_DEFINE(getcwd, rpl_getcwd,
      [Define to rpl_getcwd if the wrapper function should be used.])
  fi
])
