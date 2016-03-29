#!/bin/sh
# Ensure that tail does not ignore data that is appended to a tailed-forever
# file between tail's initial read-to-EOF, and when the inotify watches
# are established in tail_forever_inotify.  That data could be ignored
# indefinitely if no *other* data is appended, but it would be printed as
# soon as any additional appended data is detected.

# Copyright (C) 2009-2016 Free Software Foundation, Inc.

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

. "${srcdir=.}/tests/init.sh"; path_prepend_ ./src
print_ver_ tail sleep

# Terminate any background gdb/tail process
cleanup_() {
  kill $pid 2>/dev/null && wait $pid
  kill $sleep 2>/dev/null && wait $sleep
}

touch file || framework_failure_
touch tail.out || framework_failure_

( timeout 10s gdb --version ) > gdb.out 2>&1
case $(cat gdb.out) in
    *'GNU gdb'*) ;;
    *) skip_ "can't run gdb";;
esac

# Break on a line rather than a symbol, to cater for inline functions
break_src="$abs_top_srcdir/src/tail.c"
break_line=$(grep -n ^tail_forever_inotify "$break_src") || framework_failure_
break_line=$(echo "$break_line" | cut -d: -f1) || framework_failure_


# Note we get tail to monitor a background sleep process
# rather than using timeout(1), as timeout sends SIGCONT
# signals to its monitored process, and gdb (7.9 at least)
# has _intermittent_ issues with this.
# Sending SIGCONT resulted in either delayed child termination,
# or no child termination resulting in a hung test.
# See https://sourceware.org/bugzilla/show_bug.cgi?id=18364

env sleep 10 & sleep=$!

# See if gdb works and
# tail_forever_inotify is compiled and run
gdb -nx --batch-silent \
    --eval-command="break $break_line"             \
    --eval-command="run --pid=$sleep -f file"      \
    --eval-command='quit'                          \
    tail < /dev/null > gdb.out 2>&1

kill $sleep || skip_ 'breakpoint not hit'
wait $sleep

# FIXME: The above is seen to _intermittently_ fail with:
# warning: .dynamic section for "/lib/libc.so.6" is not at the expected address
# warning: difference appears to be caused by prelink, adjusting expectations
compare /dev/null gdb.out || skip_ "can't set breakpoints in tail"

env sleep 10 & sleep=$!

# Run "tail -f file", stopping to append a line just before
# inotify initialization, and then continue.  Before the fix,
# that just-appended line would never be output.
gdb -nx --batch-silent \
    --eval-command="break $break_line"             \
    --eval-command="run --pid=$sleep -f file >> tail.out"       \
    --eval-command='shell echo never-seen-with-tail-7.5 >> file' \
    --eval-command='continue'                      \
    --eval-command='quit'                          \
    tail < /dev/null > /dev/null 2>&1 & pid=$!

tail --pid=$pid -f tail.out | (read REPLY; kill $pid)

# gdb has a bug in Debian's gdb-6.8-3 at least that causes it to not
# cleanup and exit correctly when it receives a SIGTERM, but
# killing sleep, should cause the tail process and thus gdb to exit.
kill $sleep
wait $sleep

wait $pid

compare /dev/null tail.out && fail=1

Exit $fail
