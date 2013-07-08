#!/bin/sh
# Test df's behavior when the mount list contains duplicate entries.
# This test is skipped on systems that lack LD_PRELOAD support; that's fine.

# Copyright (C) 2012-2013 Free Software Foundation, Inc.

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

. "${srcdir=.}/tests/init.sh"; path_prepend_ ./src
print_ver_ df
require_gcc_shared_

df || skip_ "df fails"

# Simulate an mtab file with two entries of the same device number.
# Also add entries with unstatable mount dirs to ensure that's handled.
cat > k.c <<'EOF' || framework_failure_
#include <stdio.h>
#include <stdlib.h>
#include <mntent.h>

struct mntent *getmntent (FILE *fp)
{
  /* Prove that LD_PRELOAD works. */
  static int done = 0;
  if (!done)
    {
      fclose (fopen ("x", "w"));
      ++done;
    }

  static struct mntent mntents[] = {
    {.mnt_fsname="/short",  .mnt_dir="/invalid/mount/dir"},
    {.mnt_fsname="fsname",  .mnt_dir="/",},
    {.mnt_fsname="/fsname", .mnt_dir="/root"},
    {.mnt_fsname="/fsname", .mnt_dir="/"},
  };

  if (!getenv ("CU_TEST_DUPE_INVALID") && done == 1)
    done++;  /* skip the first entry.  */

  while (done++ <= 4)
    {
      mntents[done-2].mnt_type = "-";
      return &mntents[done-2];
    }
  return NULL;
}
EOF

# Then compile/link it:
gcc --std=gnu99 -shared -fPIC -ldl -O2 k.c -o k.so \
  || framework_failure_ 'failed to build shared library'

# Test if LD_PRELOAD works:
LD_PRELOAD=./k.so df
test -f x || skip_ "internal test failure: maybe LD_PRELOAD doesn't work?"

# The fake mtab file should only contain entries
# having the same device number; thus the output should
# consist of a header and one entry.
LD_PRELOAD=./k.so df >out || fail=1
test $(wc -l <out) -eq 2 || { fail=1; cat out; }

# Ensure we fail when unable to stat invalid entries
LD_PRELOAD=./k.so CU_TEST_DUPE_INVALID=1 df >out && fail=1
test $(wc -l <out) -eq 2 || { fail=1; cat out; }

# df should also prefer "/fsname" over "fsname"
test $(grep -c '/fsname' <out) -eq 1 || { fail=1; cat out; }
# ... and "/fsname" with '/' as Mounted on over '/root'
test $(grep -c '/root' <out) -eq 0 || { fail=1; cat out; }

# Ensure that filtering duplicates does not affect -a processing.
LD_PRELOAD=./k.so df -a >out || fail=1
test $(wc -l <out) -eq 4 || { fail=1; cat out; }

# Ensure that filtering duplicates does not affect
# argument processing (now without the fake getmntent()).
df '.' '.' >out || fail=1
test $(wc -l <out) -eq 3 || { fail=1; cat out; }

Exit $fail
