#!/bin/sh
# test conv=lcase and conv=ucase

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
print_ver_ dd

printf 'abcdefghijklmnopqrstuvwxyz\n' > input-lower || framework_failure_
printf 'ABCDEFGHIJKLMNOPQRSTUVWXYZ\n' > input-upper || framework_failure_

# Check the output when all input characters are already the correct case.
dd if=input-lower of=output-lower conv=lcase || fail=1
cmp input-lower output-lower || fail=1
dd if=input-upper of=output-upper conv=ucase || fail=1
cmp input-upper output-upper || fail=1

# Check the output when all input characters need case conversion.
dd if=input-upper of=output-lower conv=lcase || fail=1
cmp input-lower output-lower || fail=1
dd if=input-lower of=output-upper conv=ucase || fail=1
cmp input-upper output-upper || fail=1

export LC_ALL=en_US.iso8859-1  # only lowercase form works on macOS 10.15.7
if test "$(locale charmap 2>/dev/null | sed 's/iso/ISO-/')" = ISO-8859-1; then
  # Case conversion should work on all single byte locales.
  # Check it with é and É in ISO 8859-1.
  printf '\xe9\n' > input-lower
  printf '\xc9\n' > input-upper

  # Check the output when all input characters are already the correct case.
  dd if=input-lower of=output-lower conv=lcase || fail=1
  cmp input-lower output-lower || fail=1
  dd if=input-upper of=output-upper conv=ucase || fail=1
  cmp input-upper output-upper || fail=1

  # Check the output when all input characters need case conversion.
  dd if=input-upper of=output-lower conv=lcase || fail=1
  cmp input-lower output-lower || fail=1
  dd if=input-lower of=output-upper conv=ucase || fail=1
  cmp input-upper output-upper || fail=1
fi

Exit $fail
