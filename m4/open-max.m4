#serial 1
# Determine approximately how many files may be open simultaneously
# in one process.  This is approximate, since while running this test,
# the configure script already has a few files open.
# From Jim Meyering

AC_DEFUN([UTILS_SYS_OPEN_MAX],
[
  AC_CACHE_CHECK([determine how many files may be open simultaneously],
                 utils_cv_sys_open_max,
  [
  AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
    int
    main ()
    {
      FILE *result = fopen ("conftest.omax", "w");
      int i = 1;
      /* Impose an arbitrary limit, in case some system has no
	 effective limit on the number of simultaneously open files.  */
      while (i < 30000)
	{
	  FILE *s = fopen ("conftest.op", "w");
	  if (!s)
	    break;
	  ++i;
	}
      fprintf (result, "%d\n", i);
      exit (fclose (result) == EOF);
    }
  ]])],
       [utils_cv_sys_open_max=`cat conftest.omax`],
       [utils_cv_sys_open_max='internal error in open-max.m4'],
       [utils_cv_sys_open_max='cross compiling run-test in open-max.m4'])])

  AC_DEFINE_UNQUOTED([UTILS_OPEN_MAX],
    $utils_cv_sys_open_max,
    [the maximum number of simultaneously open files per process])
])
