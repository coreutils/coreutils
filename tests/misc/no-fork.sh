#!/bin/sh
# Test that various utilities do not fork a child process before
# executing another program.

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
print_ver_ true

case $host_os in
  cygwin* | mingw* | windows*)
    skip_ 'this system cannot exec without creating a new process' ;;
esac

# List of programs that should not fork a child process.
printf '%s' '\
chroot --skip-chdir / true
env true
nice true
nohup true
runcon "$(id -Z)" true
stdbuf -oL true
' |
sort -k 1b,1 > all_executors || framework_failure_

printf '%s\n' $built_programs |
sort -k 1b,1 > built_programs || framework_failure_

join all_executors built_programs > built_executors || framework_failure_

while read executor; do
  executor=$(eval $executor)
  (ulimit -u 0; exec $executor) 2>err ||
    case "$executor" in
      runcon*)
        grep 'runcon: runcon may be used only on a SELinux kernel' err
        ;;
      chroot*)
        ! uid_is_privileged_
        ;;
      *)
        false
        ;;
    esac || { cat err; fail=1; }
done < built_executors

Exit $fail
