#serial 1
# Use replacement ftw.c if the one in the C library is inadequate or buggy.
# From Jim Meyering

AC_DEFUN([AC_FUNC_FTW],
[AC_CACHE_CHECK([for working GNU ftw], ac_cv_func_ftw_working,
[
  # prerequisites
  AC_REQUIRE([AC_HEADER_DIRENT])
  AC_CHECK_HEADERS(sys/param.h)
  AC_CHECK_DECLS([stpcpy])

  # The following test would fail prior to glibc-2.3.2, because `depth'
  # would be 2 rather than 4.
  mkdir -p conftest.dir/a/b/c
  AC_RUN_IFELSE([AC_LANG_SOURCE([], [[
#include <string.h>
#include <stdlib.h>
#include <ftw.h>

static char *_f[] = { "conftest.dir", "conftest.dir/a",
		      "conftest.dir/a/b", "conftest.dir/a/b/c" };
static char **p = _f;
static int depth;

static int
cb (const char *file, const struct stat *sb, int file_type, struct FTW *info)
{
  if (strcmp (file, *p++) != 0)
    exit (1);
  ++depth;
  return 0;
}

int
main ()
{
  int err = nftw ("conftest.dir", cb, 30, FTW_PHYS | FTW_MOUNT | FTW_CHDIR);
  exit ((err == 0 && depth == 4) ? 0 : 1);
}
]])],
               [ac_cv_func_ftw_working=yes],
               [ac_cv_func_ftw_working=no],
               [ac_cv_func_ftw_working=no])])
test $ac_cv_func_ftw_working = no && AC_LIBOBJ([ftw])
])# AC_FUNC_FTW
