#!/bin/sh
# Test that 'shred --verbose' quotes file names in its output.

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
print_ver_ shred printf

NL='
'

# Test quoting when the file resides in the current working directory.
qname=$(env printf '%q' file"$NL"1) &&
cat <<EOF >exp-err &&
shred: $qname: pass 1/3 (random)...
shred: $qname: pass 2/3 (random)...
shred: $qname: pass 3/3 (random)...
shred: $qname: removing
shred: $qname: renamed to 000000
shred: 000000: renamed to 00000
shred: 00000: renamed to 0000
shred: 0000: renamed to 000
shred: 000: renamed to 00
shred: 00: renamed to 0
shred: $qname: removed
EOF
echo a >file"$NL"1 || framework_failure_

shred --verbose --remove=wipe file"$NL"1 >out 2>err || fail=1
compare /dev/null out || fail=1
compare exp-err err || fail=1
returns_ 1 test -f file"$NL"1 || fail=1

# Test quoting when the directory containing the file requires quoting.
qname=$(env printf '%q' dir"$NL"1/abc) &&
qname000=$(env printf '%q' dir"$NL"1/000) &&
qname00=$(env printf '%q' dir"$NL"1/00) &&
qname0=$(env printf '%q' dir"$NL"1/0) &&
cat <<EOF >exp-err &&
shred: $qname: pass 1/3 (random)...
shred: $qname: pass 2/3 (random)...
shred: $qname: pass 3/3 (random)...
shred: $qname: removing
shred: $qname: renamed to $qname000
shred: $qname000: renamed to $qname00
shred: $qname00: renamed to $qname0
shred: $qname: removed
EOF
mkdir -p dir"$NL"1 &&
echo a >dir"$NL"1/abc || framework_failure_

shred --verbose --remove=wipe dir"$NL"1/abc >out 2>err || fail=1
compare /dev/null out || fail=1
compare exp-err err || fail=1
returns_ 1 test -f dir"$NL"1/abc || fail=1

Exit $fail
