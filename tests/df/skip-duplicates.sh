#!/bin/sh
# Test df's behavior when the mount list contains duplicate entries.
# This test is skipped on systems that lack LD_PRELOAD support; that's fine.

# Copyright (C) 2012-2014 Free Software Foundation, Inc.

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

# We use --local here so as to not activate
# potentially very many remote mounts.
df --local || skip_ "df fails"

export CU_NONROOT_FS=$(df --local --output=target 2>&1 | grep /. | head -n1)
test -z "$CU_NONROOT_FS" && unique_entries=1 || unique_entries=2

grep '^#define HAVE_MNTENT_H 1' $CONFIG_HEADER > /dev/null \
      || skip_ "no mntent.h available to confirm the interface"

grep '^#define HAVE_GETMNTENT 1' $CONFIG_HEADER > /dev/null \
      || skip_ "getmntent is not used on this system"

# Simulate an mtab file to test various cases.
cat > k.c <<'EOF' || framework_failure_
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mntent.h>

#define STREQ(a, b) (strcmp (a, b) == 0)

struct mntent *getmntent (FILE *fp)
{
  static char *nonroot_fs;
  static int done;

  /* Prove that LD_PRELOAD works. */
  if (!done)
    {
      fclose (fopen ("x", "w"));
      ++done;
    }

  static struct mntent mntents[] = {
    {.mnt_fsname="/short",  .mnt_dir="/invalid/mount/dir"},
    {.mnt_fsname="fsname",  .mnt_dir="/",},
    {.mnt_fsname="/fsname", .mnt_dir="/."},
    {.mnt_fsname="/fsname", .mnt_dir="/"},
    {.mnt_fsname="virtfs",  .mnt_dir="/NONROOT", .mnt_type="fstype1"},
    {.mnt_fsname="virtfs2", .mnt_dir="/NONROOT", .mnt_type="fstype2"},
    {.mnt_fsname="netns",   .mnt_dir="net:[1234567]"},
  };

  if (done == 1)
    {
      nonroot_fs = getenv ("CU_NONROOT_FS");
      if (!nonroot_fs || !*nonroot_fs)
        nonroot_fs = "/"; /* merge into / entries.  */
    }

  if (done == 1 && !getenv ("CU_TEST_DUPE_INVALID"))
    done++;  /* skip the first entry.  */

  while (done++ <= 7)
    {
      if (!mntents[done-2].mnt_type)
        mntents[done-2].mnt_type = "-";
      if (STREQ (mntents[done-2].mnt_dir, "/NONROOT"))
        mntents[done-2].mnt_dir = nonroot_fs;
      return &mntents[done-2];
    }

  return NULL;
}
EOF

# Then compile/link it:
gcc_shared_ k.c k.so \
  || framework_failure_ 'failed to build shared library'

# Test if LD_PRELOAD works:
LD_PRELOAD=./k.so df
test -f x || skip_ "internal test failure: maybe LD_PRELOAD doesn't work?"

# The fake mtab file should only contain entries
# having the same device number; thus the output should
# consist of a header and unique entries.
LD_PRELOAD=./k.so df -T >out || fail=1
test $(wc -l <out) -eq $(expr 1 + $unique_entries) || { fail=1; cat out; }

# Ensure we don't fail when unable to stat (currently) unavailable entries
LD_PRELOAD=./k.so CU_TEST_DUPE_INVALID=1 df -T >out || fail=1
test $(wc -l <out) -eq $(expr 1 + $unique_entries) || { fail=1; cat out; }

# df should also prefer "/fsname" over "fsname"
if test "$unique_entries" = 2; then
  test $(grep -c '/fsname' <out) -eq 1 || { fail=1; cat out; }
  # ... and "/fsname" with '/' as Mounted on over '/.'
  test $(grep -cF '/.' <out) -eq 0 || { fail=1; cat out; }
fi

# df should use the last seen devname (mnt_fsname) and devtype (mnt_type)
test $(grep -c 'virtfs2.*fstype2' <out) -eq 1 || { fail=1; cat out; }

# Ensure that filtering duplicates does not affect -a processing.
LD_PRELOAD=./k.so df -a >out || fail=1
test $(wc -l <out) -eq 6 || { fail=1; cat out; }
# Ensure placeholder "-" values used for the eclipsed "virtfs"
test $(grep -c 'virtfs *-' <out) -eq 1 || { fail=1; cat out; }

# Ensure that filtering duplicates does not affect
# argument processing (now without the fake getmntent()).
df '.' '.' >out || fail=1
test $(wc -l <out) -eq 3 || { fail=1; cat out; }

Exit $fail
