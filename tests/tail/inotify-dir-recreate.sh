#!/bin/sh
# Makes sure, inotify will switch to polling mode if directory
# of the watched file was removed and recreated.
# (...instead of getting stuck forever)

# Copyright (C) 2017-2025 Free Software Foundation, Inc.

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

grep '^#define HAVE_INOTIFY 1' "$CONFIG_HEADER" >/dev/null && is_local_dir_ . \
  || skip_ 'inotify is not supported'

# There may be a mismatch between is_local_dir_ (gnulib's remoteness check),
# and coreutils' is_local_fs_type(), so double check we're using inotify.
touch file.strace
require_strace_ 'inotify_add_watch'
returns_ 124 timeout .1 strace -e inotify_add_watch -o strace.out \
  tail -F file.strace || fail=1
grep 'inotify' strace.out || skip_ 'inotify not detected'

# Terminate any background tail process
cleanup_() { kill $pid 2>/dev/null && wait $pid; }

cleanup_fail_ ()
{
  warn_ $1
  cleanup_
  fail=1
}

# $check_re - string to be found
# $check_f - file to be searched
check_tail_output_ ()
{
  local delay="$1"
  grep $check_re $check_f > /dev/null ||
    { sleep $delay ; return 1; }
}

grep_timeout_ ()
{
  check_re="$1"
  check_f="$2"
  retry_delay_ check_tail_output_ .1 5
}

# Prepare the file to be watched
mkdir dir && echo 'inotify' > dir/file || framework_failure_

#tail must print content of the file to stdout, verify
timeout 60 tail --pid=$$ -F dir/file >out 2>&1 & pid=$!
grep_timeout_ 'inotify' 'out' ||
{ cleanup_fail_ 'file to be tailed does not exist'; }

inotify_failed_re='inotify (resources exhausted|cannot be used)'
grep -E "$inotify_failed_re" out &&
  skip_ "inotify can't be used"

# Remove the directory, should get the message about the deletion
rm -r dir || framework_failure_
grep_timeout_ 'polling' 'out' ||
{ cleanup_fail_ 'tail did not switch to polling mode'; }

# Recreate the dir, must get a message about recreation
mkdir dir && touch dir/file || framework_failure_
grep_timeout_ 'appeared' 'out' ||
{ cleanup_fail_ 'previously removed file did not appear'; }

cleanup_

# Expected result for the whole process
cat <<\EOF > exp || framework_failure_
inotify
tail: 'dir/file' has become inaccessible: No such file or directory
tail: directory containing watched file was removed
tail: inotify cannot be used, reverting to polling
tail: 'dir/file' has appeared;  following new file
EOF

compare exp out || fail=1

Exit $fail
