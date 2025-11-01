#!/bin/sh
# Test df's behavior when the mount list cannot be read.
# This test is skipped on systems that lack LD_PRELOAD support; that's fine.

# Copyright (C) 2012-2025 Free Software Foundation, Inc.

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
print_ver_ df
require_gcc_shared_

# Protect against inaccessible remote mounts etc.
timeout 10 df || skip_ "df fails"

grep '^#define HAVE_GETMNTENT 1' $CONFIG_HEADER > /dev/null \
      || skip_ "getmntent is not used on this system"

# Simulate "mtab" failure.
# Replace gnulib streq and C23 nullptr as that are not available here.
sed 's/streq/0==str''cmp/; s/nullptr/NU''LL/' > k.c <<EOF || framework_failure_
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <mntent.h>
#include <string.h>
#include <stdarg.h>
#include <dlfcn.h>

static FILE* (*fopen_func)(const char *, const char *);

FILE* fopen(const char *path, const char *mode)
{

  /* get reference to original (libc provided) fopen */
  if (!fopen_func)
    {
      fopen_func = (FILE*(*)(const char *, const char *))
                   dlsym(RTLD_NEXT, "fopen");
      if (!fopen_func)
        {
          fprintf (stderr, "Failed to find fopen()\n");
          errno = ESRCH;
          return nullptr;
        }
    }

  /* Returning ENOENT here will get read_file_system_list()
     to fall back to using getmntent() below.  */
  if (streq (path, "/proc/self/mountinfo"))
    {
      errno = ENOENT;
      return nullptr;
    }

  return fopen_func(path, mode);
}

int open(const char *path, int flags, ...)
{
  static int (*open_func)(const char *, int, ...);

  /* get reference to original (libc provided) open */
  if (!open_func)
    {
      open_func = (int(*)(const char *, int, ...))
                   dlsym(RTLD_NEXT, "open");
      if (!open_func)
        {
          fprintf (stderr, "Failed to find open()\n");
          errno = ESRCH;
          return -1;
        }
    }

  /* Returning ENOENT here will get read_file_system_list()
     to fall back to using getmntent() below.  */
  if (streq (path, "/proc/self/mountinfo"))
    {
      errno = ENOENT;
      return -1;
    }

  va_list ap;
  va_start (ap, flags);
  mode_t mode = (sizeof (mode_t) < sizeof (int)
                 ? va_arg (ap, int)
                 : va_arg (ap, mode_t));
  va_end (ap);

  return open_func(path, flags, mode);
}

struct mntent *getmntent (FILE *fp)
{
  /* Prove that LD_PRELOAD works. */
  static int done = 0;
  if (!done)
    {
      fclose (fopen_func ("x", "w"));
      ++done;
    }
  /* Now simulate the failure. */
  errno = ENOENT;
  return nullptr;
}
EOF

# Then compile/link it:
gcc_shared_ k.c k.so \
  || skip_ 'failed to build mntent shared library'

cleanup_() { unset LD_PRELOAD; }

export LD_PRELOAD=$LD_PRELOAD:./k.so

# Test if LD_PRELOAD works:
df
test -f x || skip_ "internal test failure: maybe LD_PRELOAD doesn't work?"

# These tests are supposed to succeed:
df '.' || fail=1
df -i '.' || fail=1
df -T '.' || fail=1
df -Ti '.' || fail=1
df --total '.' || fail=1

# These tests are supposed to fail:
returns_ 1 df || fail=1
returns_ 1 df -i || fail=1
returns_ 1 df -T || fail=1
returns_ 1 df -Ti || fail=1
returns_ 1 df --total || fail=1

returns_ 1 df -a || fail=1
returns_ 1 df -a '.' || fail=1

returns_ 1 df -l || fail=1
returns_ 1 df -l '.' || fail=1

returns_ 1 df -t hello || fail=1
returns_ 1 df -t hello '.' || fail=1

returns_ 1 df -x hello || fail=1
returns_ 1 df -x hello '.' || fail=1

Exit $fail
