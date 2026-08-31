#!/bin/sh
# Exercise Follow_name directory-event lookup keyed by (parent_wd, basename).
# Covers several names in one directory and the same basename in two dirs.

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
  grep -E "$tail_re" out > /dev/null ||
    { sleep $delay; return 1; }
}

cleanup_() { kill $pid 2>/dev/null && wait $pid; }

cleanup_fail()
{
  cat out
  warn_ "$1"
  cleanup_
  fail=1
}

# Ensure this run actually uses inotify (not a silent fallback to polling).
assert_notification_mode_()
{
  inotify_failed_re='inotify (resources exhausted|cannot be used)'
  tail_re="using notification mode|$inotify_failed_re" \
    retry_delay_ check_tail_output .1 7 ||
    cleanup_fail 'tail did not use notification mode'
  if grep -E "$inotify_failed_re" out; then
    skip_ "inotify can't be used"
  fi
}

# --- Several basenames in the same parent directory. ---
mkdir d || framework_failure_
touch d/a d/b d/c || framework_failure_

rm -f out
timeout 60 tail --debug -qF d/a d/b d/c > out 2>&1 & pid=$!

assert_notification_mode_

echo a1 > d/a || framework_failure_
tail_re='^a1$' retry_delay_ check_tail_output .1 7 ||
  cleanup_fail 'missing a1'

echo b1 > d/b || framework_failure_
tail_re='^b1$' retry_delay_ check_tail_output .1 7 ||
  cleanup_fail 'missing b1'

echo c1 > d/c || framework_failure_
tail_re='^c1$' retry_delay_ check_tail_output .1 7 ||
  cleanup_fail 'missing c1'

# Recreate each name so directory CREATE events must resolve the right
# File_spec via (parent_wd, basename), not a linear scan of the wrong peer.
rm -f d/a d/b d/c || framework_failure_
echo a2 > d/a || framework_failure_
tail_re='^a2$' retry_delay_ check_tail_output .1 7 ||
  cleanup_fail 'missing a2'
echo b2 > d/b || framework_failure_
tail_re='^b2$' retry_delay_ check_tail_output .1 7 ||
  cleanup_fail 'missing b2'
echo c2 > d/c || framework_failure_
tail_re='^c2$' retry_delay_ check_tail_output .1 7 ||
  cleanup_fail 'missing c2'

kill -0 $pid || fail=1
cleanup_

# --- Same basename in two different parent directories. ---
mkdir e1 e2 || framework_failure_
touch e1/log e2/log || framework_failure_

rm -f out
timeout 60 tail --debug -qF e1/log e2/log > out 2>&1 & pid=$!

assert_notification_mode_

echo e1x > e1/log || framework_failure_
tail_re='^e1x$' retry_delay_ check_tail_output .1 7 ||
  cleanup_fail 'missing e1x'

echo e2x > e2/log || framework_failure_
tail_re='^e2x$' retry_delay_ check_tail_output .1 7 ||
  cleanup_fail 'missing e2x'

rm -f e1/log e2/log || framework_failure_
echo e1y > e1/log || framework_failure_
tail_re='^e1y$' retry_delay_ check_tail_output .1 7 ||
  cleanup_fail 'missing e1y'
echo e2y > e2/log || framework_failure_
tail_re='^e2y$' retry_delay_ check_tail_output .1 7 ||
  cleanup_fail 'missing e2y'

kill -0 $pid || fail=1
cleanup_

Exit $fail
