#!/bin/sh
# ensure that --follow=name does not imply --retry

# Copyright (C) 2011-2024 Free Software Foundation, Inc.

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

cat <<\EOF > exp || framework_failure_
tail: cannot open 'no-such' for reading: No such file or directory
tail: no files remaining
EOF
returns_ 1 timeout 10 tail --follow=name no-such > out 2> err || fail=1
# Remove an inconsequential inotify warning so
# we can compare against the above error
sed '/inotify cannot be used/d' err > k && mv k err
compare exp err || fail=1

# Between coreutils 8.34 and 9.5 inclusive, tail would have
# waited indefinitely when a file was moved to the same file system
cleanup_() { kill $pid 2>/dev/null && wait $pid; }
touch file || framework_failure_
timeout 10 tail --follow=name file & pid=$!
sleep .1 # Usually in inotify loop here
mv file file.unfollow || framework_failure_
wait $pid
test $? = 1 || fail=1

Exit $fail
