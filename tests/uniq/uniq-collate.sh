#!/bin/sh
# before coreutils-8.32, uniq would not distinguish
# items which compared equal with strcoll()
# So ensure we avoid strcoll() for the following cases.

# Copyright (C) 2020-2025 Free Software Foundation, Inc.

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
print_ver_ uniq printf

gen_input()
{
  env LC_ALL=$LOCALE_FR_UTF8 printf "$@" > in || framework_failure_
}

# strcoll() used to return 0 comparing the following strings
# which was fixed somewhere between glibc-2.22 and glibc-2.30
gen_input '%s\n' 'ⁿᵘˡˡ' 'ܥܝܪܐܩ'
test $(LC_ALL=$LOCALE_FR_UTF8 uniq < in | wc -l) = 2 || fail=1

# normalization in strcoll is inconsistent across platforms.
# glibc based systems at least do _not_ normalize in strcoll,
# while cygwin systems for example may do so.
# á composed and decomposed, are generally not compared equal
gen_input '\u00E1\na\u0301\n'
test $(LC_ALL=$LOCALE_FR_UTF8 uniq < in | wc -l) = 2 || fail=1
# Similarly with the following equivalent hangul characters
gen_input '\uAC01\n\u1100\u1161\u11A8\n'
test $(LC_ALL=ko_KR.utf8 uniq < in | wc -l) = 2 || fail=1

# Note if running in the wrong locale,
# strcoll may indicate the strings match when they don't.
# I.e., cjk and hangul will now work even if
# uniq is running in the wrong locale
# hangul (ko_KR.utf8)
gen_input '\uAC00\n\uAC01\n'
test $(LC_ALL=en_US.utf8 uniq < in | wc -l) = 2 || fail=1
# CJK (zh_CN.utf8)
gen_input '\u3400\n\u3401\n'
test $(LC_ALL=en_US.utf8 uniq < in | wc -l) = 2 || fail=1

# Note strcoll() ignores certain characters,
# but not if the strings are otherwise equal.
# I.e., the following on glibc-2.30 at least,
# as expected, does not print a single item,
# but testing here for illustration
gen_input ',a\n.a\n'
test $(LC_ALL=$LOCALE_FR_UTF8 uniq < in | wc -l) = 2 || fail=1

Exit $fail
