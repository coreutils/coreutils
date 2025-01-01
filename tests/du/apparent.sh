#!/bin/sh
# Exercise du's --apparent-size option.

# Copyright 2023-2025 Free Software Foundation, Inc.

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
print_ver_ du

mkdir -p d || framework_failure_
for f in $(seq 100); do
  echo foo >d/$f || framework_failure_
done

du -b d/* >separate || fail=1
du -b d   >together || fail=1
separate_sum=$($AWK '{sum+=$1}END{print sum}' separate) || framework_failure_
together_sum=$($AWK '{sum+=$1}END{print sum}' together) || framework_failure_
test $separate_sum -eq $together_sum || fail=1

Exit $fail
