#!/bin/sh
# Test that 'cp', 'mv', and 'install' don't remove just-created files.

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
print_ver_ cp ginstall mv

# Setup the files we expect to see.
echo a > exp-a || framework_failure_
echo b > exp-b || framework_failure_

# Setup the initial files and directories for testing.
reset_files ()
{
  rm -rf a b c &&
  mkdir a b c &&
  cat exp-a > a/f &&
  cat exp-b > b/f || framework_failure_
}

# Check that we don't lose data by overwriting a just-created file.
for prog in cp ginstall mv; do
  cat <<EOF >exp || framework_failure_
$prog: will not overwrite just-created 'c/f' with 'b/f'
EOF
  for backup in '' off nil never; do
    test -n "$backup" && backup=--backup=$backup
    reset_files
    returns_ 1 $prog $backup a/f b/f c >out 2>err || fail=1
    compare /dev/null out || fail=1
    compare exp err || fail=1
    # Check if the file specified by the first argument exists.
    # That should not be the case for 'mv'.
    if test -f a/f; then
      test "$prog" != mv || fail=1
      compare exp-a a/f || fail=1
    else
      test $prog != mv && fail=1
    fi
    # Check the file specified by the second argument and the
    # just-created file.
    compare exp-b b/f || fail=1
    compare exp-a c/f || fail=1
    # No backups should be created.
    set -- c/*
    test -f "$1" || set --
    test $# -eq 1 || fail=1
  done
done

# With numbered backups there is no data loss if we perform the
# requested operation.
for prog in cp ginstall mv; do
  reset_files
  for i in $(seq 3); do
    cat exp-a > a/f || framework_failure_
    cat exp-b > b/f || framework_failure_
    $prog --backup=t a/f b/f c >out 2>err || fail=1
    compare /dev/null out || fail=1
    compare /dev/null err || fail=1
    # When using 'mv' both source files should no longer exist.
    # Otherwise, both files should exist.
    if test -f a/f; then
      test $prog != mv || fail=1
      compare exp-a a/f || fail=1
      compare exp-b b/f || fail=1
    else
      test $prog != mv && fail=1
      test -f b/f && fail=1
    fi
    # Test that the just created file exists.
    compare exp-b c/f || fail=1
    for j in $(seq $i); do
      if test $(($j % 2)) -eq 0; then
        exp=b
      else
        exp=a
      fi
      compare exp-$exp c/f.~$j~ || fail=1
    done
  done
done

Exit $fail
