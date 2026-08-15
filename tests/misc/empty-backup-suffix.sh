#!/bin/sh
# Test that 'cp', 'mv', and 'install' don't have data loss with an empty
# backup suffix.

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

# Remove any previously created backups, then create the source
# and target files.
reset_files ()
{
  rm -f b~ b.~1~ b.~2~ && echo a > a && echo b > b || framework_failure_
}

# Check that we don't have data loss with an empty backup suffix.
for prog in cp ginstall mv; do
  for control in t nil never; do
    for backup_opt in VERSION_CONTROL --backup; do
      case $backup_opt in
        -*) backup_env=; backup_opt=$backup_opt=$control ;;
        *) backup_env=$backup_opt=$control; backup_opt=--backup ;;
      esac
      for suffix_opt in SIMPLE_BACKUP_SUFFIX --suffix; do
        case $suffix_opt in
          -*) suffix_env=; suffix_opt=$suffix_opt= ;;
          *) suffix_env=$suffix_opt=; suffix_opt= ;;
        esac
        reset_files
        env $backup_env $suffix_env $prog $suffix_opt $backup_opt a b \
            >out >err || fail=1
        compare /dev/null out || fail=1
        compare /dev/null err || fail=1
        # Check that the source exists, unless we are using 'mv'.
        if test -f a; then
          test $prog != mv || fail=1
          compare exp-a a || fail=1
        else
          test $prog = mv || fail=1
        fi
        # Check that the backup exists with the correct content.
        if test $control = t; then
          suffix='.~1~';
        else
          suffix='~'
        fi
        compare exp-b b$suffix || fail=1
        # Check that the target exists with the correct content.
        compare exp-a b || fail=1
        if test $control = nil; then
          echo b > a && mv b~ b.~1~ || framework_failure_
          env $backup_env $suffix_env $prog $suffix_opt $backup_opt a b \
            >out >err || fail=1
          compare /dev/null out || fail=1
          compare /dev/null err || fail=1
          # Check that the source exists, unless we are using 'mv'.
          if test -f a; then
            test $prog != mv || fail=1
            compare exp-b a || fail=1
          else
            test $prog = mv || fail=1
          fi
          # Check that the backups exist with the correct content.
          compare exp-b b.~1~ || fail=1
          compare exp-a b.~2~ || fail=1
          # Check that the target exists with the correct content.
          compare exp-b b || fail=1
        fi
      done
    done
  done
done

Exit $fail
