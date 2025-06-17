#!/bin/sh
# Ensure we handle cfsetispeed failing
# which we did not before coreutils v9.1

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
print_ver_ stty
require_gcc_shared_

# Replace each cfsetispeed call with a call to these stubs.
cat > k.c <<'EOF' || framework_failure_
#include <stdio.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>

int cfsetispeed(struct termios *termios_p, speed_t speed)
{
  /* Leave a marker so we can identify if the function was intercepted.  */
  fclose(fopen("preloaded", "w"));

  errno=EINVAL;
  return -1;
}
EOF

# Then compile/link it:
gcc_shared_ k.c k.so \
  || skip_ 'failed to build shared library'

( export LD_PRELOAD=$LD_PRELOAD:./k.so
  returns_ 1 stty ispeed 9600 ) || fail=1

test -e preloaded || skip_ 'LD_PRELOAD interception failed'

Exit $fail
