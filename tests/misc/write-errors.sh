#!/bin/sh
# Make sure all of these programs promptly diagnose write errors.

# Copyright (C) 2023 Free Software Foundation, Inc.

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
print_ver_ timeout

if ! test -w /dev/full || ! test -c /dev/full; then
  skip_ '/dev/full is required'
fi

# Writers that may output data indefinitely
# First word in command line is checked against built programs
echo "\
cat /dev/zero
comm -z /dev/zero /dev/zero
cut -z -c1- /dev/zero
cut -z -f1- /dev/zero
dd if=/dev/zero
expand /dev/zero
factor --version; yes 1 | factor
# TODO: fmt /dev/zero
# TODO: fold -b /dev/zero
head -z -n-1 /dev/zero
join -a 1 -z /dev/zero /dev/null
# TODO: nl --version; yes | nl
# TODO: numfmt --version; yes 1 | numfmt
od -v /dev/zero
paste /dev/zero
# TODO: pr /dev/zero
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
  timeout 10 $SHELL -c "$writer > /dev/full"
  test $? = 124 && { fail=1; echo "$writer: failed to exit" >&2; }
done < built_writers

Exit $fail
