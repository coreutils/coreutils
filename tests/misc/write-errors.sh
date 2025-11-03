#!/bin/sh
# Make sure all of these programs promptly diagnose write errors.

# Copyright (C) 2023-2025 Free Software Foundation, Inc.

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
print_ver_ timeout env
getlimits_

if ! test -w /dev/full || ! test -c /dev/full; then
  skip_ '/dev/full is required'
fi

# Writers that may output data indefinitely
# First word in command line is checked against built programs
printf '%s' "\
cat /dev/zero
comm -z /dev/zero /dev/zero
cut -z -c1- /dev/zero
cut -z -f1- /dev/zero
date +%${OFF64_T_MAX}c
date --version; yes 0 | date -f-
dd if=/dev/zero
expand /dev/zero
factor --version; yes 1 | factor
fmt /dev/zero
fmt --version; yes | fmt
fold /dev/zero
fold -b /dev/zero
fold -c /dev/zero
fold --version; yes | tr -d '\\n' | fold
head -z -n-1 /dev/zero
join -a 1 -z /dev/zero /dev/null
nl --version; yes | nl
numfmt --version; yes 1 | numfmt
od -v /dev/zero
paste /dev/zero
pr /dev/zero
pr --version; yes 1 | pr
seq inf
tail -n+1 -z /dev/zero
tee < /dev/zero
tr . . < /dev/zero
unexpand /dev/zero
uniq -z -D /dev/zero
yes
" |
sort -k 1b,1 > all_writers || framework_failure_

printf '%s\n' $built_programs |
sort -k 1b,1 > built_programs || framework_failure_

join all_writers built_programs > built_writers || framework_failure_

while read writer; do
  # Enforce mem usage limits if possible
  cmd=$(printf '%s\n' "$writer" | cut -d ' ' -f1) || framework_failure_
  base_mem=$(get_min_ulimit_v_ $cmd --version) \
    && ulimit="ulimit -v $(($base_mem+12000))" \
    || skip_ 'unable to determine ulimit -v'

  # Check /dev/full handling
  rm -f full.err || framework_failure_
  timeout 10 env --default-signal=PIPE $SHELL -c \
    "($ulimit && $writer 2>full.err >/dev/full)"
  { test $? = 124 || ! grep 'space' full.err >/dev/null; } &&
   { fail=1; cat full.err; echo "$writer: failed to exit" >&2; }

  # Check closed pipe handling
  rm -f pipe.err || framework_failure_
  timeout 10 env --default-signal=PIPE $SHELL -c \
    "($ulimit && $writer 2>pipe.err | :)"
  { test $? = 0 && compare /dev/null pipe.err; } ||
   { fail=1; cat pipe.err; echo "$writer: failed to write to closed pipe" >&2; }
done < built_writers

Exit $fail
