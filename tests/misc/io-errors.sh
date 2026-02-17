#!/bin/sh
# Make sure all of these programs promptly diagnose write errors.

# Copyright (C) 2025-2026 Free Software Foundation, Inc.

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
print_ver_ env
getlimits_

if ! test -w /dev/full || ! test -c /dev/full; then
  skip_ '/dev/full is required'
fi

echo foo > foo || framework_failure_

# Specific write paths to check in addition to --version.
# Note writers that may output data indefinitely
# are handled in write-errors.sh
# First word in command line is checked against built programs
{
printf '%s' "\
cat foo
comm foo foo
cut -c1- foo
cut -f1- foo
date +%s
dd status=none if=foo
expand foo
factor 1
fmt foo
fold foo
fold -b foo
fold -c foo
head -n1 foo
join foo foo
nl foo
numfmt --invalid=ignore < foo
od -v foo
paste foo
pr foo
seq 1
tail -n1 foo
tee < foo
tr . . < foo
unexpand foo
uniq foo
";
printf '%s --version\n' $built_programs;
} |
sort -k 1b,1 > all_writers || framework_failure_

printf '%s\n' $built_programs |
sort -k 1b,1 > built_programs || framework_failure_

join all_writers built_programs > built_writers || framework_failure_

while read writer; do

  # Skip edge cases
  printf '%s' "$writer" | grep -E '^(test|false|true|install)' &&
    { echo "Skipping $writer" >&2; continue; }

  # Check /dev/full diagnosed.
  # Note we usually give a specific diagnostic (ENOSPC),
  # but that's not guaranteed in the generic close_stream() handling.
  # For e.g. with _IOLBF etc, stdio will discard pending data at each line,
  # thus only giving a generic error upon ferror() in close_stream().
  rm -f full.err || framework_failure_
  timeout 10 env --default-signal=PIPE $SHELL -c \
    "(env $writer 2>full.err >/dev/full)"
  { test $? = 124 || ! grep -E "write error|$ENOSPC" full.err >/dev/null; } &&
   { fail=1; cat full.err; echo "$writer: failed to exit" >&2; }

  # Check closed pipe handling
  rm -f pipe.err || framework_failure_
  timeout 10 env --default-signal=PIPE $SHELL -c \
    "(env $writer 2>pipe.err | :)"
  { test $? = 0 && compare /dev/null pipe.err; } ||
   { fail=1; cat pipe.err; echo "$writer: failed writing to closed pipe" >&2; }
done < built_writers

Exit $fail
