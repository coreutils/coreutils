#!/bin/sh
# Ensure --quoting-style=locale/clocale uses Unicode quotes in UTF-8 locales

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

. "${srcdir=.}/tests/init.sh";
print_ver_ ls

test "$(LC_ALL=en_US.UTF-8 locale charmap 2>/dev/null)" = UTF-8 ||
  skip_ 'en_US.UTF-8 locale not available'

touch 'hello world' "it's" 'say "hi"' 'tab	here' || framework_failure_

# Note we use en_US as there are no translations provided,
# and so locale quoting for UTF-8 is hardcoded to these quoting characters:
# U+2018 = \xe2\x80\x98 (LEFT SINGLE QUOTATION MARK)
# U+2019 = \xe2\x80\x99 (RIGHT SINGLE QUOTATION MARK)
lq=$(printf '\342\200\230')
rq=$(printf '\342\200\231')

# In UTF-8 locales, both locale and clocale should use Unicode quotes
for style in locale clocale; do
  LC_ALL=en_US.UTF-8 ls --quoting-style=$style > out_${style}_utf8 || fail=1

  # Verify Unicode left/right quotes are present
  grep "$lq" out_${style}_utf8 > /dev/null 2>&1 \
    || { echo "$style UTF-8: missing Unicode left quote"; fail=1; }
  grep "$rq" out_${style}_utf8 > /dev/null 2>&1 \
    || { echo "$style UTF-8: missing Unicode right quote"; fail=1; }

  # Verify 'hello world' is quoted with Unicode quotes
  grep "^${lq}hello world${rq}\$" out_${style}_utf8 > /dev/null 2>&1 \
    || { echo "$style UTF-8: 'hello world' not properly quoted"; fail=1; }

  # Embedded apostrophe should NOT be escaped (delimiters are different chars)
  grep "^${lq}it's${rq}\$" out_${style}_utf8 > /dev/null 2>&1 || \
    { echo "$style UTF-8: embedded apostrophe shouldn't be escaped"; fail=1; }

  # Embedded double quote should NOT be escaped
  grep "^${lq}say \"hi\"${rq}\$" out_${style}_utf8 > /dev/null 2>&1 || \
    { echo "$style UTF-8: embedded double quote shouldn't be escaped"; fail=1; }

  # Control characters should still be C-escaped
  grep "tab\\\\there" out_${style}_utf8 > /dev/null 2>&1 \
    || { echo "$style UTF-8: tab should be escaped as \\t"; fail=1; }
done

# In C locale, locale uses ASCII single quotes, clocale uses ASCII double quotes
LC_ALL=C ls --quoting-style=locale > out_locale_c || fail=1
grep "^'hello world'\$" out_locale_c > /dev/null 2>&1 \
  || { echo "locale C: expected ASCII single quotes"; fail=1; }

LC_ALL=C ls --quoting-style=clocale > out_clocale_c || fail=1
grep '^"hello world"$' out_clocale_c > /dev/null 2>&1 \
  || { echo "clocale C: expected ASCII double quotes"; fail=1; }

Exit $fail
