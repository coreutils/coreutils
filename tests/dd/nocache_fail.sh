#!/bin/sh
# Ensure we diagnose failure to drop caches
# We didn't check the return from posix_fadvise() correctly before v9.7

# Copyright (C) 2025 Free Software Foundation, Inc.

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
print_ver_ dd
require_gcc_shared_

# Replace each getxattr and lgetxattr call with a call to these stubs.
# Count those and write the total number of calls to the file "x"
# via a global destructor.
cat > k.c <<'EOF' || framework_failure_
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>

int posix_fadvise (int fd, off_t offset, off_t len, int advice)
{
  fopen ("called", "w");
  return ENOTSUP;  /* Simulate non standard error indication.  */
}
EOF

# Then compile/link it:
gcc_shared_ k.c k.so \
  || framework_failure_ 'failed to build shared library'

touch ifile || framework_failure_

LD_PRELOAD=$LD_PRELOAD:./k.so dd if=ifile iflag=nocache count=0 2>err
ret=$?

test -f called || skip_ "internal test failure: maybe LD_PRELOAD doesn't work?"

grep 'dd: failed to discard cache for: ifile' err || fail=1

# Ensure that the dd command failed
test "$ret" = 1 || fail=1

Exit $fail
