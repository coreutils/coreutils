#!/bin/sh
# Test 'du --block-size' and the BLOCK_SIZE environment variable.

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
print_ver_ du

# Kilobytes.
truncate -s 1000 1kb &&
truncate -s 10000 10kb &&
truncate -s 100000 100kb || framework_failure_

# Kibibytes.
truncate -s 1024 1kib &&
truncate -s 10240 10kib &&
truncate -s 102400 100kib || framework_failure_

# Megabytes.
truncate -s 1000000 1mb &&
truncate -s 10000000 10mb &&
truncate -s 100000000 100mb || framework_failure_

# Mebibytes.
truncate -s 1048576 1mib &&
truncate -s 10485760 10mib &&
truncate -s 104857600 100mib || framework_failure_

# 'tr', but only operating on the first character.
tr_first ()
{
  first=$(printf '%s' "$1" | cut -c1) || framework_failure_
  rest=$(printf '%s' "$1" | cut -c2-) || framework_failure_
  printf '%s%s' "$(printf '%s' "$first" | tr "$2" "$3")" "$rest" ||
    framework_failure_
}

# Test a unit, both with and without the 1 prefix.
test_unit ()
{
  lower=$(tr_first "$1" '[:upper:]' '[:lower:]')
  upper=$(tr_first "$1" '[:lower:]' '[:upper:]')
  # When there is a prefix of 1, the unit is not printed.  Setup a file
  # to compare to.
  sed 's/^\([0-9][0-9]*\)'"$upper"'/\1/g' <exp >exp1 || framework_failure_
  for opt in BLOCK_SIZE -B --block-size=; do
    for c in $lower $upper; do
      for prefix in '' 1; do
        case $opt in
          -*) env=; arg=$opt$prefix$c ;;
          *) arg=; env=$opt=$prefix$c ;;
        esac
        env $env du -A $arg 1* >out 2>err || fail=1
        compare exp$prefix out || fail=1
        compare /dev/null err || fail=1
      done
    done
  done
  # The '-k' option is equivalent to --block-size=1K, but doesn't fit
  # well in the above loop.  Just test it here.
  if test $lower = k; then
    du -A -k 1* >out 2>err || fail=1
    compare exp1 out || fail=1
    compare /dev/null err || fail=1
  fi
}

cat <<\EOF >exp || framework_failure_
98KiB	100kb
100KiB	100kib
97657KiB	100mb
102400KiB	100mib
10KiB	10kb
10KiB	10kib
9766KiB	10mb
10240KiB	10mib
1KiB	1kb
1KiB	1kib
977KiB	1mb
1024KiB	1mib
EOF

# Kilobytes.
test_unit KiB

cat <<\EOF >exp || framework_failure_
98K	100kb
100K	100kib
97657K	100mb
102400K	100mib
10K	10kb
10K	10kib
9766K	10mb
10240K	10mib
1K	1kb
1K	1kib
977K	1mb
1024K	1mib
EOF

# Kibibytes.
test_unit k

cat <<\EOF >exp || framework_failure_
1MiB	100kb
1MiB	100kib
96MiB	100mb
100MiB	100mib
1MiB	10kb
1MiB	10kib
10MiB	10mb
10MiB	10mib
1MiB	1kb
1MiB	1kib
1MiB	1mb
1MiB	1mib
EOF

# Megabytes.
test_unit MiB

cat <<\EOF >exp || framework_failure_
1M	100kb
1M	100kib
96M	100mb
100M	100mib
1M	10kb
1M	10kib
10M	10mb
10M	10mib
1M	1kb
1M	1kib
1M	1mb
1M	1mib
EOF

# Mebibytes.
test_unit m

Exit $fail
