#!/bin/sh
# Test warnings for sort options

# Copyright (C) 2010-2016 Free Software Foundation, Inc.

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
print_ver_ sort

cat <<\EOF > exp
sort: using simple byte comparison
sort: key 1 has zero width and will be ignored
sort: using simple byte comparison
sort: key 1 has zero width and will be ignored
sort: using simple byte comparison
sort: key 1 is numeric and spans multiple fields
sort: using simple byte comparison
sort: options '-bghMRrV' are ignored
sort: using simple byte comparison
sort: options '-bghMRV' are ignored
sort: option '-r' only applies to last-resort comparison
sort: using simple byte comparison
sort: option '-r' only applies to last-resort comparison
sort: using simple byte comparison
sort: options '-bg' are ignored
sort: using simple byte comparison
sort: using simple byte comparison
sort: option '-b' is ignored
sort: using simple byte comparison
sort: using simple byte comparison
sort: using simple byte comparison
sort: leading blanks are significant in key 1; consider also specifying 'b'
sort: using simple byte comparison
sort: leading blanks are significant in key 1; consider also specifying 'b'
sort: option '-d' is ignored
sort: using simple byte comparison
sort: leading blanks are significant in key 1; consider also specifying 'b'
sort: option '-i' is ignored
sort: using simple byte comparison
sort: using simple byte comparison
sort: using simple byte comparison
sort: failed to set locale; using simple byte comparison
EOF

sort -s -k2,1 --debug /dev/null 2>>out
sort -s -k2,1n --debug /dev/null 2>>out
sort -s -k1,2n --debug /dev/null 2>>out
sort -s -rRVMhgb -k1,1n --debug /dev/null 2>>out
sort -rRVMhgb -k1,1n --debug /dev/null 2>>out
sort -r -k1,1n --debug /dev/null 2>>out
sort -gbr -k1,1n -k1,1r --debug /dev/null 2>>out
sort -b -k1b,1bn --debug /dev/null 2>>out # no warning
sort -b -k1,1bn --debug /dev/null 2>>out
sort -b -k1,1bn -k2b,2 --debug /dev/null 2>>out # no warning
sort -r -k1,1r --debug /dev/null 2>>out # no warning for redundant options
sort -i -k1,1i --debug /dev/null 2>>out # no warning
sort -d -k1,1b --debug /dev/null 2>>out
sort -i -k1,1d --debug /dev/null 2>>out
sort -r --debug /dev/null 2>>out #no warning
sort -rM --debug /dev/null 2>>out #no warning
sort -rM -k1,1 --debug /dev/null 2>>out #no warning
LC_ALL=missing sort --debug /dev/null 2>>out

compare exp out || fail=1

cat <<\EOF > exp
sort: using simple byte comparison
sort: key 1 is numeric and spans multiple fields
sort: obsolescent key '+2 -1' used; consider '-k 3,1' instead
sort: key 2 has zero width and will be ignored
sort: leading blanks are significant in key 2; consider also specifying 'b'
sort: option '-b' is ignored
sort: option '-r' only applies to last-resort comparison
EOF

sort --debug -rb -k2n +2.2 -1b /dev/null 2>out

compare exp out || fail=1

Exit $fail
