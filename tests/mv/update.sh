#!/bin/sh
# make sure --update works as advertised

# Copyright (C) 2001-2025 Free Software Foundation, Inc.

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
print_ver_ cp mv

test_reset() {
  echo old > old || framework_failure_
  touch -d yesterday old || framework_failure_
  echo new > new || framework_failure_
}

test_reset
for interactive in '' -i; do
  for cp_or_mv in cp mv; do
    # This is a no-op, with no prompt.
    # With coreutils-6.9 and earlier, using --update with -i would
    # mistakenly elicit a prompt.
    $cp_or_mv $interactive --update old new < /dev/null > out 2>&1 || fail=1
    compare /dev/null out || fail=1
    case "$(cat new)" in new) ;; *) fail=1 ;; esac
    case "$(cat old)" in old) ;; *) fail=1 ;; esac
  done
done

# This should prompt. coreutils 9.3-9.5 mistakenly did not
echo n | returns_ 1 mv -vi -u new old >/dev/null 2>&1 || fail=1

# These should accept all options
for update_option in '--update' '--update=older' '--update=all' \
 '--update=none' '--update=none-fail'; do

  touch file1 || framework_failure_
  mv $update_option file1 file2 || fail=1
  test -f file1 && fail=1
  cp $update_option file2 file1 || fail=1
  rm file1 file2 || framework_failure_
done

# These should perform the rename / copy
for update_option in '--update' '--update=older' '--update=all' \
 '--update=none --update=all'; do
  test_reset
  mv $update_option new old || fail=1
  test -f new && fail=1
  case "$(cat old)" in new) ;; *) fail=1 ;; esac

  test_reset
  cp $update_option new old || fail=1
  case "$(cat old)" in new) ;; *) fail=1 ;; esac
  case "$(cat new)" in new) ;; *) fail=1 ;; esac
done

# These should not perform the rename / copy
for update_option in '--update=none' '--update=none-fail' \
 '--update=all --update=none' \
 '--update=all --no-clobber' \
 '--no-clobber --update=all'; do

  echo "$update_option" | grep 'fail' >/dev/null && ret=1 || ret=0

  test_reset
  returns_ $ret mv $update_option new old || fail=1
  case "$(cat new)" in new) ;; *) fail=1 ;; esac
  case "$(cat old)" in old) ;; *) fail=1 ;; esac

  test_reset
  returns_ $ret cp $update_option new old || fail=1
  case "$(cat new)" in new) ;; *) fail=1 ;; esac
  case "$(cat old)" in old) ;; *) fail=1 ;; esac
done

Exit $fail
