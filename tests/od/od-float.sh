#!/bin/sh
# Test od on floating-point values.

# Copyright (C) 2010-2025 Free Software Foundation, Inc.

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
print_ver_ od printf

export LC_ALL=C

# Test for a bug in coreutils up through 8.7: od was losing
# information when asked to parse floating-point values.  The numeric
# tests are valid only on Intel-like hosts, but that should be good
# enough to detect regressions, as they are designed to succeed on
# non-Intel-like hosts.  Also, test for another bug in coreutils 8.7
# on x86: sometimes there was no space between the columns.

set x $(echo aaaabaaa | tr ab '\376\377' | od -t fF) ||
  fail=1
case "$*" in
*0-*) fail=1;;
esac
case $3,$4 in
-1.694740e+38,-1.694740e+38) fail=1;;
esac

set x $(echo aaaaaaaabaaaaaaa | tr ab '\376\377' | od -t fD) ||
  fail=1
case "$*" in
*0-*) fail=1;;
esac
case $3,$4 in
-5.314010372517808e+303,-5.314010372517808e+303) fail=1;;
esac

set x $(echo aaaaaaaaaaaaaaaabaaaaaaaaaaaaaaa | tr ab '\376\377' | od -t fL) ||
  fail=1
case "$*" in
*0-*) fail=1;;
esac
case $3,$4 in
-1.023442870282055988e+4855,-1.023442870282055988e+4855) fail=1;;
esac

# Ensure od doesn't crash as it did on glibc <= 2.5:
# https://sourceware.org/PR4586
set x $(printf 00000000ff000000 | tr 0f '\000\377' | od -t fL) || fail=1
# With coreutils <= 8.7 we used to print "nan" for the above invalid value.
# However since v8.7-22-ga71c22f we deferred to the system printf routines
# through the use of the ftoastr module.  So the following check would only
# be valid on x86_64 if we again handle the conversion internally or
# if this glibc bug is resolved:
# https://sourceware.org/PR17661
#case "$*" in
#*nan*) ;;
#*) fail=1;;
#esac

# Check Half precision IEEE 16 bit float
if grep '^#define FLOAT16_SUPPORTED 1' "$CONFIG_HEADER" >/dev/null; then
  for fmt in '-tfH' '-tf2'; do
    od_out=$(env printf '\x3C\x00\x3C\x00' | od --end=big -An $fmt | tr -d ' ')
    test "$od_out" = '11' || fail=1
  done
else
  echo "od: this system doesn't provide a 'fH' floating point type" > exp_err
  returns_ 1 od -tfH /dev/null 2>err || fail=1
  compare exp_err err || fail=1
fi
# Check Half precision Brain 16 bit float
if grep '^#define BF16_SUPPORTED 1' "$CONFIG_HEADER" >/dev/null; then
  od_out=$(env printf '\x3F\x80\x3F\x80' | od --end=big -An -tfB | tr -d ' ')
  test "$od_out" = '11' || fail=1
else
  echo "od: this system doesn't provide a 'fB' floating point type" > exp_err
  returns_ 1 od -tfB /dev/null 2>err || fail=1
  compare exp_err err || fail=1
fi

Exit $fail
