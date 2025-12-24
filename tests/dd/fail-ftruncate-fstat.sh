#!/bin/sh
# Check that 'dd' does not continue copying if ftruncate and fstat fail.

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

cat > k.c <<'EOF' || framework_failure_
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

int
ftruncate (int fd, off_t length)
{
  /* Prove that LD_PRELOAD works: create the evidence file "x".  */
  fclose (fopen ("x", "w"));

  /* Pretend failure.  */
  errno = EPERM;
  return -1;
}

int
fstat (int fd, struct stat *statbuf)
{
  /* Prove that LD_PRELOAD works: create the evidence file "y".  */
  fclose (fopen ("y", "w"));

  /* Pretend failure.  */
  errno = EPERM;
  return -1;
}
EOF

# Then compile/link it:
gcc_shared_ k.c k.so \
  || framework_failure_ 'failed to build shared library'

# Setup the file "out" and preserve it's original contents in "exp-out".
yes | head -n 2048 | tr -d '\n' > out || framework_failure_
cp out exp-out || framework_failure_

LD_PRELOAD=$LD_PRELOAD:./k.so dd if=/dev/zero of=out count=1 \
                              seek=1 status=none 2>err
ret=$?

test -f x && test -f y \
  || skip_ "internal test failure: maybe LD_PRELOAD doesn't work?"

# After ftruncate fails, we use fstat to get the file type.
echo "dd: cannot fstat 'out': Operation not permitted" > exp
compare exp err || fail=1

# coreutils 9.1 to 9.9 would mistakenly continue copying after ftruncate
# failed and exit successfully.
test "$ret" = 1 || fail=1
compare exp-out out || fail=1

Exit $fail
