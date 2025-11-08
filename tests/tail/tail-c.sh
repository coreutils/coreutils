#!/bin/sh
# exercise tail -c

# Copyright 2014-2025 Free Software Foundation, Inc.

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

# Make sure it works on funny files in /proc and /sys.
for file in /proc/version /sys/kernel/profiling; do
  if test -r $file; then
    cp -f $file copy &&
    tail -c -1 copy > exp || framework_failure_

    tail -c -1 $file > out || fail=1
    compare exp out || fail=1
  fi
done

# Make sure it works for pipes
printf '123456' | tail -c3 > out || fail=1
printf '456' > exp || framework_failure_
compare exp out || fail=1

# Any part of /dev/zero should be valid for tail -c.
head -c 4096 /dev/zero >exp || fail=1
tail -c 4096 /dev/zero >out || fail=1
compare exp out || fail=1

# Any part of /dev/urandom, if it exists, should be valid for tail -c.
if test -r /dev/urandom; then
  # Or at least it should not read it forever
  timeout --verbose 10 tail -c 4096 /dev/urandom >/dev/null 2>err
  case $? in
      0) ;;
      # Solaris 11 allows negative seek but then gives EINVAL on read
      1) grep 'Invalid argument' err || fail=1;;
      *)
        case $host_triplet in
          *linux-gnu*)
            case "$(uname -r)" in
              [12].*) ;;  # Older Linux versions timeout
              *) fail=1 ;;
            esac ;;
             # GNU/Hurd cannot seek on /dev/urandom.
          *) test "$(uname)" = GNU || fail=1 ;;
        esac ;;
  esac
fi

Exit $fail
