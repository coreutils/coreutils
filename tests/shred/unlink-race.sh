#!/bin/sh
# Test for races when 'shred' removes the file.

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
print_ver_ shred

mkfifo_or_skip_ fifo

# Setup d1/f, which will be removed, and d2/f,
# which will remain if there are no races.
mkdir d1 d2 &&
echo deleted >d1/f &&
echo saved >d2/f &&
cp d2/f exp-saved || framework_failure_

# Make sure the 'shred' process gets cleaned up.
cleanup_() { kill "$pid" 2>/dev/null; wait "$pid" 2>/dev/null; pid=; }

# Use the FIFO as the random source.  After opening d1/f1, it
# will wait for bytes from the FIFO during its random pass.
shred -v -x -n1 -u --random-source=fifo d1/f 2>err & pid=$!
exec 3>fifo || framework_failure_

# Once the FIFO is opened by a reader, 'shred' will begin its random pass.
opened_() { sleep $1; grep 'pass 1/1 (random)\.\.\.$' err >/dev/null; }
retry_delay_ opened_ .1 6 || framework_failure_

# Move the directory containing the file being overwritten,
# and create a symbolic link in its place.
mv d1 d1.orig && ln -s d2 d1 || framework_failure_

# Fill the random source to allow 'shred' to complete.
timeout 10 cat /dev/zero >&3
exec 3>&- || framework_failure_

wait "$pid" || fail=1
pid=

# Verify that the 'shred' removed the file from the original
# directory and not the one now referenced by the symbolic link.
compare exp-saved d2/f || fail=1
test -f d1.orig/f && fail=1

Exit $fail
