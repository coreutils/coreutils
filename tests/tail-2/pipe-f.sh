#!/bin/sh
# ensure that tail -f doesn't hang in various cases

# Copyright (C) 2009-2018 Free Software Foundation, Inc.

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

# Speedup the non inotify case
fastpoll='-s.1 --max-unchanged-stats=1'

for mode in '' '---disable-inotify'; do
# Ensure :|tail -f doesn't hang, per POSIX
echo oo > exp || framework_failure_
echo foo | timeout 10 tail -f $mode $fastpoll -c3 > out || fail=1
compare exp out || fail=1
cat <<\EOF > exp || framework_failure_
==> standard input <==
ar
EOF
echo bar | returns_ 1 \
 timeout 10 tail -f $mode $fastpoll -c3 - missing > out || fail=1
compare exp out || fail=1

# This would wait indefinitely before v8.28 due to no EPIPE being
# generated due to no data written after the first small amount
timeout 10 tail -f $mode $fastpoll out | sleep .1 || fail=1

# This would wait indefinitely before v8.28 (until first write)
(returns_ 1 timeout 10 tail -f $mode $fastpoll /dev/null >&-) || fail=1
done

Exit $fail
