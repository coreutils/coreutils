#serial 2
# Use the replacement ftw.c if the one in the C library is inadequate or buggy.
# For now, we always use the code in lib/ because libc doesn't have the FTW_DCH
# or FTW_DCHP that we need.  Arrange to use lib/ftw.h.  And since that
# implementation uses tsearch.c/tdestroy, add tsearch.o to the list of
# objects and arrange to use lib/search.h if necessary.
# From Jim Meyering

AC_DEFUN([AC_FUNC_FTW],
[
  # prerequisites
  AC_REQUIRE([AC_HEADER_STAT])
  AC_REQUIRE([jm_FUNC_LSTAT])
  AC_REQUIRE([AC_HEADER_DIRENT])
  AC_CHECK_HEADERS(sys/param.h)
  AC_CHECK_DECLS([stpcpy])

  # In the event that we have to use the replacement ftw.c,
  # see if we'll also need the replacement tsearch.c.
  AC_CHECK_FUNC([tdestroy], , [need_tdestroy=1])

  AC_CACHE_CHECK([for ftw/FTW_CHDIR that informs callback of failed chdir],
                 ac_cv_func_ftw_working,
  [
  # The following test would fail prior to glibc-2.3.2, because `depth'
  # would be 2 rather than 4.  Of course, now that we require FTW_DCH
  # and FTW_DCHP, this test fails even with GNU libc's fixed ftw.
  mkdir -p conftest.dir/a/b/c
  AC_RUN_IFELSE([AC_LANG_SOURCE([[
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
  /* Require these symbols, too.  */
  int d1 = FTW_DCH;
  int d2 = FTW_DCHP;

  int err = nftw ("conftest.dir", cb, 30, FTW_PHYS | FTW_MOUNT | FTW_CHDIR);
  exit ((err == 0 && depth == 4) ? 0 : 1);
}
]])],
               [ac_cv_func_ftw_working=yes],
               [ac_cv_func_ftw_working=no],
               [ac_cv_func_ftw_working=no])])
  rm -rf conftest.dir
  if test $ac_cv_func_ftw_working = no; then
    AC_LIBOBJ([ftw])
    AC_CONFIG_LINKS([$ac_config_libobj_dir/ftw.h:$ac_config_libobj_dir/ftw_.h])
    # Add tsearch.o IFF we have to use the replacement ftw.c.
    if test -n "$need_tdestroy"; then
      AC_LIBOBJ([tsearch])
      # Link search.h to search_.h if we use the replacement tsearch.c.
      AC_CONFIG_LINKS(
        [$ac_config_libobj_dir/search.h:$ac_config_libobj_dir/search_.h])
    fi
  fi
])# AC_FUNC_FTW
