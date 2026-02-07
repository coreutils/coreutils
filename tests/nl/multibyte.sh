#!/bin/sh
# Test nl with multibyte section delimiters.

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
print_ver_ nl printf

test "$LOCALE_FR_UTF8" != none || skip_ "French UTF-8 locale not available"

cat <<\EOF > exp || framework_failure_

     1	a

     2	b

     3	c
EOF

test_nl_multibyte ()
{
  {
    export LC_ALL="$LOCALE_FR_UTF8"
    # A missing second character implies ':'.
    env printf "$2$2$2\na\n$2$2\nb\n$2\nc\n" > inp || framework_failure_
    nl -p -ha -fa -d $(env printf "$1") < inp > out || fail=1
  }
  compare exp out
}

# Implied ':' character.
test_nl_multibyte '\xc3' '\xc3:' || fail=1
test_nl_multibyte '\uB250' '\uB250:' || fail=1

# Two characters.
test_nl_multibyte '\xc3\xc3' '\xc3\xc3' || fail=1
test_nl_multibyte '\uB250\uB250' '\uB250\uB250' || fail=1

# More than 2 characters is a GNU extension.
test_nl_multibyte '\uB250\uB250\uB250' '\uB250\uB250\uB250' || fail=1
test_nl_multibyte "$(bad_unicode)" "$(bad_unicode)" || fail=1

Exit $fail
