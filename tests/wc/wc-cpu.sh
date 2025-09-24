#!/bin/sh
# Ensure cpu specific code operates correctly

# Copyright (C) 2025 Free Software Foundation, Inc.

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
print_ver_ wc

GLIBC_TUNABLES='glibc.cpu.hwcaps=-AVX2,-AVX512F' \
 wc -l --debug /dev/null 2>debug || fail=1
grep 'using.*hardware support' debug && fail=1

lines=$(shuf -i 0-1000 | head -n1)  || framework_failure_
seq 1000 | head -n "$lines" > lines || framework_failure_

wc_accelerated=$(wc -l < lines) || fail=1
wc_accelerated_no_avx512=$(
          GLIBC_TUNABLES='glibc.cpu.hwcaps=-AVX512F' \
          wc -l < lines
         ) || fail=1
wc_base=$(
          GLIBC_TUNABLES='glibc.cpu.hwcaps=-AVX2,-AVX512F' \
          wc -l < lines
         ) || fail=1

test "$wc_accelerated" = "$wc_base" || fail=1
test "$wc_accelerated_no_avx512" = "$wc_base" || fail=1

Exit $fail
