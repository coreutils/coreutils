#!/bin/sh
# Count GB18030 characters spanning read buffers.

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
print_ver_ wc printf
getlimits_

export LC_ALL=zh_CN.gb18030
test "$(locale charmap 2>/dev/null | sed 's/gb/GB/')" = GB18030 ||
  skip_ 'GB18030 charset support not detected'

# U+10000 is encoded as 90 30 81 30.  Its ASCII bytes must not be
# counted again when the character is split across read buffers.
padding=$(($IO_BUFSIZE - 1))
head -c "$padding" /dev/zero | tr '\000' a > in || framework_failure_
env printf '\x90\x30\x81\x30' >> in || framework_failure_
printf '%s\n' "$IO_BUFSIZE" > exp || framework_failure_
wc -m < in > out || fail=1
compare exp out || fail=1

Exit $fail
