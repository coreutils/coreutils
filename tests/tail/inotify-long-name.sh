#!/bin/sh
# Check that the inotify event buffers are allocated with a large enough size.

# Copyright (C) 2026 Free Software Foundation, Inc.

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
print_ver_ tail
require_inotify_supported_

check_tail_output()
{
  local delay="$1"
  grep "$tail_re" out > /dev/null ||
    { sleep $delay; return 1; }
}

# Make sure we can create a long file name.
long_name=$(printf '%0255d' | tr '0' 'a') || framework_failure_
touch "$long_name" || skip_ 'cannot create long file name'
rm -f "$long_name" || framework_failure_

# Create an empty file to tail.
touch file || framework_failure_

cleanup_() { kill "$pid" 2>/dev/null; wait "$pid" 2>/dev/null; pid=; }

# Now tail it and make sure we are using notification mode.
timeout 60 tail --debug -F file >out 2>&1 & pid=$!
tail_re='tail: using notification mode' retry_delay_ check_tail_output .1 7 ||
  { cat out; skip_ 'tail did not switch to notification mode'; }

# Create a long file name and then write to the file being tailed.
# coreutils-9.12 and earlier only allocated enough memory to follow
# events for its operands.  Creating a file name longer than that would
# cause it to fail and exit.
touch "$long_name" &&
echo abc >file || framework_failure_
tail_re='^abc$' retry_delay_ check_tail_output .1 7 || { cat out; fail=1; }

echo 1 >"$long_name" &&
echo def >>file || framework_failure_
tail_re='^def$' retry_delay_ check_tail_output .1 7 || { cat out; fail=1; }

mv "$long_name" new  &&
echo ghi >>file || framework_failure_
tail_re='^ghi$' retry_delay_ check_tail_output .1 7 || { cat out; fail=1; }

Exit $fail
