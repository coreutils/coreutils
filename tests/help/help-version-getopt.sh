#!/bin/sh
# Ensure --version and --help options are processed before
# any other options by some programs.

# Copyright (C) 2019-2025 Free Software Foundation, Inc.

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
print_ver_ yes

programs="cksum dd hostid hostname link logname nohup
sleep tsort unlink uptime users whoami yes"

# All these variations should show the help/version screen
# regardless of their position on the command line arguments.
for prog in $programs; do
  # skip if the program is not built (e.g., hostname)
  case " $built_programs " in
  *" $prog "*) ;;
  *) continue;;
  esac

  for opt in --help --version; do
    rm -f exp out1 out2 out3 || framework_failure_

    # Get the reference output
    env $prog $opt >exp || fail=1

    # Test: some other argument AFTER --help/--version.
    env $prog $opt AFTER > out1 || fail=1
    compare exp out1 || fail=1

    # nohup is an exception: stops processing after first non-option parameter.
    # E.g., "nohup df --help" should run "df --help", not "df --help".
    if [ "$prog" = nohup ]; then
      rm -f exp  || framework_failure_
      df $opt > exp || framework_failure_
      BEFORE=df
    else
      BEFORE=BEFORE
    fi

    # Test: some other argument BEFORE --help/--version.
    env $prog $BEFORE $opt > out2 || fail=1
    compare exp out2 || fail=1

    # Test: some other arguments BEFORE and AFTER --help/--version.
    env $prog $BEFORE $opt AFTER > out3 || fail=1
    compare exp out3 || fail=1
  done
done

# After end-of-options marker (--), the options should not be parsed.
# Test with 'yes', and assume the common code will work the
# same for the other programs.
cat >exp <<\EOF || framework_failure_
--help
EOF
env yes -- --help | head -n1 > out
compare exp out || fail=1

Exit $fail
