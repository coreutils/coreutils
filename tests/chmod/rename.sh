#!/bin/sh
# Check that chmod finishes on the file it opened if the name goes away.

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
print_ver_ chmod timeout
require_strace_ openat

touch file && chmod 600 file || framework_failure_
# Older kernels or libraries can require a second lookup of the name.
strace --quiet=all -o probe -e trace=fchmodat2 chmod -h 600 file \
  2>/dev/null || skip_ 'cannot trace fchmodat2'
grep 'fchmodat2(.* = 0' probe >/dev/null || skip_ 'fchmodat2 unavailable'
grep 'AT_EMPTY_PATH.* = -1' probe >/dev/null && skip_ 'no empty-path chmod'

inject=$(get_open_syscalls_):signal=SIGSTOP:when=1
strace --quiet=all -o /dev/null -P file -e inject=$inject true 2>/dev/null \
  || skip_ 'strace does not support the required injection options'

tracer=
cleanup_()
{
  if test -n "$tracer" && kill -0 "$tracer" 2>/dev/null; then
    test ! -s pid || kill -KILL "$(cat pid)" 2>/dev/null
    kill "$tracer" 2>/dev/null
    wait "$tracer" 2>/dev/null
  fi
}

# Stop after opening file.  Its new name is the only one left when chmod
# resumes.  Request the sticky bit so -c also verifies the resulting mode.
timeout -k 1 60 strace --quiet=all -o trace -P file -e inject=$inject \
  $SHELL -c 'echo $$ >pid; exec chmod -c g+r,o+t file' >out 2>err & tracer=$!
stopped_() { sleep "$1"; grep 'stopped by SIGSTOP' trace >/dev/null; }
retry_delay_ stopped_ .1 6 || { cat trace; Exit 1; }

mv file moved || framework_failure_
kill -CONT "$(cat pid)" || framework_failure_
wait "$tracer" || { cat err; fail=1; }
tracer=
rm -f pid
case $(stat -c %a moved) in 640|1640) ;; *) fail=1 ;; esac
grep -F "mode of 'file' changed from 0600 " out >/dev/null || fail=1
compare /dev/null err || fail=1

Exit $fail
