#!/bin/sh
# Exercise the fmt -g option.

# Copyright (C) 2012-2026 Free Software Foundation, Inc.

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
print_ver_ fmt

cat <<\_EOF_ > base || framework_failure_

@command{fmt} prefers breaking lines at the end of a sentence, and tries to
avoid line breaks after the first word of a sentence or before the last word
of a sentence.  A @dfn{sentence break} is defined as either the end of a
paragraph or a word ending in any of @samp{.?!}, followed by two spaces or end
of line, ignoring any intervening parentheses or quotes.  Like @TeX{},
@command{fmt} reads entire ''paragraphs'' before choosing line breaks; the
algorithm is a variant of that given by
Donald E. Knuth and Michael F. Plass
in ''Breaking Paragraphs Into Lines'',
@cite{Software---Practice & Experience}
@b{11}, 11 (November 1981), 1119--1184.
_EOF_

fmt -g 60 -w 72 base > out || fail=1

cat <<\_EOF_ > exp || framework_failure_

@command{fmt} prefers breaking lines at the end of a sentence,
and tries to avoid line breaks after the first word of a sentence
or before the last word of a sentence.  A @dfn{sentence break}
is defined as either the end of a paragraph or a word ending
in any of @samp{.?!}, followed by two spaces or end of line,
ignoring any intervening parentheses or quotes.  Like @TeX{},
@command{fmt} reads entire ''paragraphs'' before choosing line
breaks; the algorithm is a variant of that given by Donald
E. Knuth and Michael F. Plass in ''Breaking Paragraphs Into
Lines'', @cite{Software---Practice & Experience} @b{11}, 11
(November 1981), 1119--1184.
_EOF_

compare exp out || fail=1

# When only -g is given, the maximum width defaults to the goal plus 10,
# so "-g G" must lay a paragraph out exactly as "-g G -w G+10" does.
for g in 5 20 40 60 75; do
  fmt -g $g base > out || fail=1
  fmt -g $g -w $(expr $g + 10) base > exp || fail=1
  compare exp out || fail=1
done

# Check that derived maximum width directly: with -g 20 the limit is 30,
# so a 30 character paragraph fits on one line while a 31 character one
# does not.
printf '%s %s\n' aaaaaaaaaaaaaa bbbbbbbbbbbbbbb | fmt -g 20 > out || fail=1
cat <<\_EOF_ > exp || framework_failure_
aaaaaaaaaaaaaa bbbbbbbbbbbbbbb
_EOF_
compare exp out || fail=1

printf '%s %s\n' aaaaaaaaaaaaaa bbbbbbbbbbbbbbbb | fmt -g 20 > out || fail=1
cat <<\_EOF_ > exp || framework_failure_
aaaaaaaaaaaaaa
bbbbbbbbbbbbbbbb
_EOF_
compare exp out || fail=1

# A goal of zero is accepted, and gives a maximum width of 10.
printf 'aaaa bbbb cccc\n' | fmt -g 0 > out || fail=1
cat <<\_EOF_ > exp || framework_failure_
aaaa
bbbb cccc
_EOF_
compare exp out || fail=1

# Without -w the goal is limited to the default maximum width of 75.
fmt -g 75 base > /dev/null || fail=1
returns_ 1 fmt -g 76 base > /dev/null 2>&1 || fail=1

# With -w a larger goal is fine.
fmt -g 76 -w 100 base > /dev/null || fail=1

Exit $fail
