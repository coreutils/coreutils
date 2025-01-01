#!/bin/sh
# ensure that tail -f only uses inotify for regular files or fifos

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

grep '^#define HAVE_INOTIFY 1' "$CONFIG_HEADER" >/dev/null \
  || skip_ 'inotify support required'

require_strace_ 'inotify_add_watch'

returns_ 124 timeout .1 strace -e inotify_add_watch -o strace.out \
  tail -f /dev/null || fail=1

grep 'inotify' strace.out && fail=1

Exit $fail
