#!/bin/sh
# Test df's behaviour when the mount list cannot be read.
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

# Simulate "mtab" failure.
cat > k.c <<'EOF' || framework_failure_
#include <stdio.h>
#include <errno.h>
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
  /* Now simulate the failure. */
  errno = ENOENT;
  return NULL;
}
EOF

# Then compile/link it:
$CC -shared -fPIC -ldl -O2 k.c -o k.so \
  || framework_failure_ 'failed to build shared library'

# Test if LD_PRELOAD works:
LD_PRELOAD=./k.so df
test -f x || skip_ "internal test failure: maybe LD_PRELOAD doesn't work?"

# These tests are supposed to succeed:
LD_PRELOAD=./k.so df '.' || fail=1
LD_PRELOAD=./k.so df -i '.' || fail=1
LD_PRELOAD=./k.so df -T '.' || fail=1
LD_PRELOAD=./k.so df -Ti '.' || fail=1
LD_PRELOAD=./k.so df --total '.' || fail=1

# These tests are supposed to fail:
LD_PRELOAD=./k.so df && fail=1
LD_PRELOAD=./k.so df -i && fail=1
LD_PRELOAD=./k.so df -T && fail=1
LD_PRELOAD=./k.so df -Ti && fail=1
LD_PRELOAD=./k.so df --total && fail=1

LD_PRELOAD=./k.so df -a && fail=1
LD_PRELOAD=./k.so df -a '.' && fail=1

LD_PRELOAD=./k.so df -l && fail=1
LD_PRELOAD=./k.so df -l '.' && fail=1

LD_PRELOAD=./k.so df -t hello && fail=1
LD_PRELOAD=./k.so df -t hello '.' && fail=1

LD_PRELOAD=./k.so df -x hello && fail=1
LD_PRELOAD=./k.so df -x hello '.' && fail=1

Exit $fail
