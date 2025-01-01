#!/bin/sh
# Verify that all options mentioned in usage are recognized by getopt.

# Copyright (C) 2017-2025 Free Software Foundation, Inc.

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

checkprg () {
  prg="$1"

  # Get stderr output for unrecognized options for later use as a pattern.
  # Also consider the expected exit status of each program.
  rcexp=1
  case "$prg" in
    dir | ls | printenv | sort | tty | vdir ) rcexp=2 ;;
    env | chroot | nice | nohup | runcon | stdbuf | timeout ) rcexp=125 ;;
  esac
  # Write the pattern for a long, unknown option into a pattern file.
  o='thisoptiondoesnotexist'
  returns_ $rcexp $prg --$o >/dev/null 2> err || fail=1
  grep -F "$o" err || framework_failure_
  sed -n "1s/--$o/OPT/p" < err > pat || framework_failure_

  # Append the pattern for a short unknown option.
  returns_ $rcexp $prg -/ >/dev/null 2> err || fail=1
  grep " '*/'*" err || framework_failure_
  # The following only handles the common case that has single quotes,
  # as it simplifies to identify missing options only on common systems.
  sed -n "1s/'\/'/'OPT'/p" < err >> pat || framework_failure_

  # Get output for --help.
  $prg --help > help || fail=1

  # Extract all options mention in the above --help output.
  nl="
  "
  sed -n -e '/--version/q' \
    -e 's/^ \{2,6\}-/-/; s/  .*//; s/[=[].*//; s/, /\'"$nl"'/g; s/^-/-/p' help \
    > opts || framework_failure_
  cat opts  # for debugging purposes

  # Test all options mentioned in usage (but --version).
  while read opt; do
    test "x$opt" = 'x--help' \
      && continue
    # Append --help to be on the safe side: the option under test either
    # requires a further argument, or --help triggers usage(); so $prg should
    # exit without performing its regular operation.
    # Add a 2nd --help for the 'env -u --help' case.
    $prg "$opt" --help --help </dev/null >out 2>err1
    rc=$?
    # In the --help case, i.e., when the option under test has been accepted,
    # the exit code should be Zero.
    if [ $rc = 0 ]; then
      compare help out || fail=1
    else
      # Else $prg should have complained about a missing argument.
      # Catch false positives.
      case "$prg/$opt" in
        'pr/-COLUMN') continue;;
      esac
      # Replace $opt in stderr output by the neutral placeholder.
      # Handle both long and short option error messages.
      sed -e "s/$opt/OPT/" -e "s/'.'/'OPT'/" < err1 > err || framework_failure_
      # Fail if the stderr output matches the above provoked error.
      grep -Ff pat err && { fail=1; cat err1; }
    fi
  done < opts
}

for prg in $built_programs; do
  case "$prg" in
    # Skip utilities entirely which have special option parsing.
    '[' | expr | stty )
      continue;;
    # Wrap some utilities known by the shell by env.
    echo | false | kill | printf | pwd | sleep | test | true )
      prg="env $prg";;
  esac
  checkprg $prg
done

Exit $fail
