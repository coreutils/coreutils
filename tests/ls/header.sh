#!/bin/sh
# test ls --header

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
print_ver_ ls

touch file1 file2 || framework_failure_
mkdir subdir || framework_failure_
touch subdir/subfile || framework_failure_

# 1. Test basic ls -l --header on files
ls -l --header file1 file2 > out || fail=1
head -n 1 out > header_line || fail=1
grep '^Mode *Links *Owner *Group *Size *Date *Name$' \
  header_line > /dev/null || { cat out; fail=1; }

# Verify Name column in data starts at the exact offset of "Name" in header
#name_col=$(expr "$(head -n 1 out)" : '.*\(Name\)')
header_prefix=$(head -n 1 out | sed 's/Name.*//')
prefix_len=${#header_prefix}
file1_name=$(sed -n '2p' out | cut -c $((prefix_len + 1))-)
test "$file1_name" = "file1" || {
  echo "Name column misaligned:"
  cat out; fail=1
}

# 2. Test directory listing: header should appear after "total" line
ls -l --header subdir > out || fail=1
sed -n '1p' out | grep '^total' > /dev/null || { cat out; fail=1; }
sed -n '2p' out | \
    grep '^Mode *Links *Owner *Group *Size *Date *Name$' > /dev/null || {
        cat out
        fail=1
    }

# 3. Test -i with --header
ls -il --header file1 file2 > out || fail=1
head -n 1 out | \
    grep '^ *Inode *Mode *Links *Owner *Group *Size *Date *Name$' \
    > /dev/null || {
        cat out
        fail=1
    }

# 4. Test -s with --header
ls -sl --header file1 file2 > out || fail=1
head -n 1 out | \
    grep '^ *Blocks *Mode *Links *Owner *Group *Size *Date *Name$' \
    > /dev/null || {
        cat out
        fail=1
    }

# 5. Test -is with --header
ls -isl --header file1 file2 > out || fail=1
head -n 1 out | \
    grep '^ *Inode *Blocks *Mode *Links *Owner *Group *Size *Date *Name$' \
    > /dev/null || {
        cat out
        fail=1
    }

# 6. Test -g (no owner) with --header
ls -g --header file1 file2 > out || fail=1
head -n 1 out | \
    grep '^Mode *Links *Group *Size *Date *Name$' > /dev/null || {
        cat out
        fail=1
    }

# 7. Test -o (no group) with --header
ls -o --header file1 file2 > out || fail=1
head -n 1 out | \
    grep '^Mode *Links *Owner *Size *Date *Name$' \
    > /dev/null || { cat out; fail=1; }

# 8. Test -og (no owner, no group) with --header
ls -og --header file1 file2 > out || fail=1
head -n 1 out | \
    grep '^Mode *Links *Size *Date *Name$' > /dev/null || { cat out; fail=1; }

# 9. Test -n (numeric IDs) with --header
ls -n --header file1 file2 > out || fail=1
head -n 1 out | \
    grep '^Mode *Links *Owner *Group *Size *Date *Name$' > /dev/null || {
        cat out
        fail=1
    }

# 10. Test --author with --header
ls -l --author --header file1 file2 > out || fail=1
head -n 1 out | \
    grep '^Mode *Links *Owner *Group *Author *Size *Date *Name$' \
    > /dev/null || { cat out; fail=1; }

# 11. Test --header without -l should NOT print headers
ls --header file1 file2 > out || fail=1
grep 'Mode' out > /dev/null && { cat out; fail=1; }

Exit "$fail"
