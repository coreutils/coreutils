#!/bin/sh
# Make sure failure to write warnings is handled gracefully

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

if ! test -w /dev/full || ! test -c /dev/full; then
  skip_ '/dev/full is required'
fi

touch foo || framework_failure_

# Commands that write to stderr but don't exit immediately
# Escapes must be double escaped.
printf '%s' "\
cksum --debug /dev/null
date --debug -d 'now'
du --max-depth=0 --summarize /dev/null
env --debug true
ginstall --strip-program=foo foo bar
numfmt --debug 1
od -w1 /dev/null
printf 'foo' 'bar'
sort --debug /dev/null
stat --printf '\\\\' .
tail -n0 --retry foo
tr . '\\\\' < /dev/null
wc -l --debug /dev/null
" |
sort -k 1b,1 > all_writers || framework_failure_

printf '%s\n' $built_programs |
sort -k 1b,1 > built_programs || framework_failure_

join all_writers built_programs > built_writers || framework_failure_

expected_failure_status_sort=2
expected_failure_status_env=0  # env's exec resets default exit handlers

while read writer; do
  cmd=$(printf '%s\n' "$writer" | cut -d ' ' -f1) || framework_failure_
  sanitizer_build_ $cmd && continue  # standard error not closed/checked
  eval "expected=\$expected_failure_status_$cmd"
  test x$expected = x && expected=1
  $SHELL -c "env $writer" 2>err || fail=1
  test -s err || { echo "no standard error output from: $writer"; continue; }
  returns_ $expected \
   $SHELL -c "env $writer" 2>/dev/full || fail=1
done < built_writers

Exit $fail
