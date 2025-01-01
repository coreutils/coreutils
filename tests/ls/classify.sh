#!/bin/sh
# Test --classify processing

# Copyright (C) 2020-2025 Free Software Foundation, Inc.

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
print_ver_ ls

mkdir testdir || framework_failure_
cd testdir || framework_failure_
mkdir dir || framework_failure_
touch regular executable || framework_failure_
chmod a+x executable || framework_failure_
ln -s regular slink-reg || framework_failure_
ln -s dir slink-dir || framework_failure_
ln -s nowhere slink-dangle || framework_failure_
mknod block b 20 20 2> /dev/null && block="block
"
mknod char c 10 10 2> /dev/null && char="char
"
mkfifo_or_skip_ fifo
cd ..

cat <<EOF > exp
$block${char}dir/
executable*
fifo|
regular
slink-dangle@
slink-dir@
slink-reg@
EOF
sed 's/[*/@|]//' exp > exp2 || framework_failure_

ls --classify testdir > out || fail=1
ls --classify=always testdir > out2 || fail=1
ls --classify=auto testdir > out3 || fail=1
ls --classify=never testdir > out4 || fail=1

compare exp out || fail=1

compare exp out2 || fail=1

compare exp2 out3 || fail=1

compare exp2 out4 || fail=1

returns_ 1 ls --classify=invalid || fail=1

Exit $fail
