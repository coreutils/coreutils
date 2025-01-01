#!/bin/sh
# exercise nl functionality

# Copyright (C) 2002-2025 Free Software Foundation, Inc.

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
print_ver_ nl
getlimits_

echo a | nl > out || fail=1
echo b | nl -s%n >> out || fail=1
echo c | nl -n ln >> out || fail=1
echo d | nl -n rn >> out || fail=1
echo e | nl -n rz >> out || fail=1
echo === >> out
printf 'a\n\n' | nl > t || fail=1; cat -A t >> out
cat <<\EOF > exp
     1	a
     1%nb
1     	c
     1	d
000001	e
===
     1^Ia$
       $
EOF
compare exp out || fail=1

# Ensure numbering reset at each delimiter.
# coreutils <= v8.25 only reset at a page header.
printf '%s\n' '\:\:\:' a '\:\:' b '\:' c > in.txt || framework_failure_
nl -ha -fa in.txt > out.tmp || fail=1
nl -p -ha -fa in.txt >> out.tmp || fail=1
sed '/^$/d' < out.tmp > out || framework_failure_
cat <<\EOF > exp
     1	a
     1	b
     1	c
     1	a
     2	b
     3	c
EOF
compare exp out || fail=1

# Ensure we only indicate overflow when needing to output overflowed numbers
returns_ 1 nl -v$INTMAX_OFLOW /dev/null || fail=1
printf '%s\n' a \\:\\: b > in.txt || framework_failure_
nl -v$INTMAX_MAX in.txt > out || fail=1
cat <<EOF > exp
$INTMAX_MAX	a

$INTMAX_MAX	b
EOF
compare exp out || fail=1
returns_ 1 nl -p -v$INTMAX_MAX in.txt > out || fail=1

# Test negative iteration
returns_ 1 nl -i$INTMAX_UFLOW /dev/null || fail=1
printf '%s\n' a b > in.txt || framework_failure_
nl -v$INTMAX_MAX -i$INTMAX_MIN in.txt > out || fail=1
cat <<EOF > exp
$INTMAX_MAX	a
    -1	b
EOF
compare exp out || fail=1
printf '%s\n' a b c > in.txt || framework_failure_
returns_ 1 nl -v$INTMAX_MAX -i$INTMAX_MIN in.txt > out || fail=1

# Test GNU extension to --section-delimiter, of disabling section matching
printf '%s\n' a '\:\:' c > in.txt || framework_failure_
nl -d '' in.txt > out || fail=1
cat <<\EOF > exp
     1	a
     2	\:\:
     3	c
EOF
compare exp out || fail=1

# Test GNU extension to --section-delimiter, of supporting strings longer than 2
printf '%s\n' a foofoo c > in.txt || framework_failure_
nl -d 'foo' in.txt > out || fail=1
cat <<EOF > exp
     1	a

     1	c
EOF
compare exp out || fail=1

# Ensure single char delimiters assume a following ':' character (as per POSIX)
# coreutils <= v8.32 didn't match single char delimiters at all
printf '%s\n' a x:x: c > in.txt || framework_failure_
nl -d 'x' in.txt > out || fail=1
cat <<EOF > exp
     1	a

     1	c
EOF
compare exp out || fail=1

Exit $fail
