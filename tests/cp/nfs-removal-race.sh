#!/bin/sh
# Running cp S D on an NFS client while another client has just removed D
# would lead (w/coreutils-8.16 and earlier) to cp's initial stat call
# seeing (via stale NFS cache) that D exists, so that cp would then call
# open without the O_CREAT flag.  Yet, the open must actually consult
# the server, which confesses that D has been deleted, thus causing the
# open call to fail with ENOENT.
#
# This test simulates that situation by intercepting stat for a nonexistent
# destination, D, and making the stat fill in the result struct for another
# file and return 0.
#
# This test is skipped on systems that lack LD_PRELOAD support; that's fine.
# Similarly, on a system that lacks <dlfcn.h>, skipping it is fine.
# Note: glibc 2.33+ removed __xstat, so we intercept both __xstat (old glibc)
# and stat (new glibc) to support all systems.

# Copyright (C) 2012-2026 Free Software Foundation, Inc.

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

. "${srcdir=.}/tests/init.sh"; path_prepend_ ./src
print_ver_ cp
require_gcc_shared_

# Replace each stat call with a call to this wrapper.
# We intercept both __xstat (glibc < 2.33) and stat (glibc >= 2.33)
# to support all glibc versions.
cat > k.c <<'EOF' || framework_failure_
#define _GNU_SOURCE
#include <stdio.h>
#include <sys/types.h>
#include <dlfcn.h>

#define __xstat __xstat_orig

#include <sys/stat.h>
#include <stddef.h>

#undef __xstat

static int
is_dest_path (const char *path)
{
  return *path == 'd' && path[1] == 0;
}

static const char *
redirect_path (const char *path)
{
  /* When asked to stat nonexistent "d",
     return results suggesting it exists. */
  if (is_dest_path (path))
    {
      /* Only mark preloaded when we intercept stat on the destination "d".
         This ensures the test verifies that cp actually calls stat on
         the destination, not just any file. */
      fclose (fopen ("preloaded", "w"));
      return "d2";
    }
  return path;
}

/* For glibc < 2.33: stat() calls __xstat() internally */
int
__xstat (int ver, const char *path, struct stat *st)
{
  static int (*real_xstat) (int, const char *, struct stat *) = NULL;
  if (!real_xstat)
    real_xstat = dlsym (RTLD_NEXT, "__xstat");
  if (!real_xstat)
    return -1;
  return real_xstat (ver, redirect_path (path), st);
}

/* For glibc >= 2.33: stat() is a direct symbol */
int
stat (const char *path, struct stat *st)
{
  static int (*real_stat) (const char *, struct stat *) = NULL;
  if (!real_stat)
    real_stat = dlsym (RTLD_NEXT, "stat");
  return real_stat (redirect_path (path), st);
}

/* Since coreutils v8.32 we use fstatat() rather than stat()  */
int fstatat(int dirfd, const char *restrict path,
            struct stat *restrict statbuf, int flags)
{
  static int (*real_fstatat) (int dirfd, const char *restrict pathname,
              struct stat *restrict statbuf, int flags) = NULL;
  if (!real_fstatat)
    real_fstatat = dlsym (RTLD_NEXT, "fstatat");
  return real_fstatat (dirfd, redirect_path (path), statbuf, flags);
}

EOF

# Then compile/link it:
gcc_shared_ k.c k.so \
  || framework_failure_ 'failed to build shared library'

touch d2 || framework_failure_
echo xyz > src || framework_failure_

# Finally, run the test:
LD_PRELOAD=$LD_PRELOAD:./k.so cp -T src d || fail=1

test -f preloaded || skip_ 'LD_PRELOAD was ineffective?'

compare src d || fail=1
Exit $fail
