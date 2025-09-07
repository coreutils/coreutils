#!/bin/sh
# Make sure commands support all aliased options

# Copyright (C) 2025 Free Software Foundation, Inc.

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

# Restrict to GNU sed for now
sed --version | grep GNU >/dev/null || skip_ 'GNU sed required'

rm -f failed || framework_failure_

for c in $abs_top_srcdir/src/*.c; do
  cmd=$(basename "$c" '.c')
  # extract option definitions
  sed -n '/ long_options\[\]/,/};/p' "$c" | grep ', ' |
  # ensure definitions on one line
  sed ':a;/[^}],$/{N;s/[^}],\n/,/;ta}' |
  # Strip comments
  sed 's| */\*.*||' |
  # append blank line after each repeated group
  { uniq --all-repeated=separate -f3; echo; } |
  # extract long options
  cut -d'"' -f2 |
  while read opt; do
    if test "$opt"; then
      opts="$opts --$opt "
    else
      if test "$opts"; then
        case " $built_programs " in
          *" $cmd "*)
            $cmd $opts --version >/dev/null || touch failed ;;
          *)
            echo $cmd >> skipped ;;
        esac
      fi
      opts=''
    fi
  done
done

if test -f failed; then
  fail=1
elif test -f skipped; then
  skip_ $(sort -u skipped | paste -s -d,) not built
fi

Exit $fail
