#!/bin/sh

# Copyright (C) 2012-2015 Free Software Foundation, Inc.

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

. "${srcdir=.}/tests/init.sh"; path_prepend_ ./src
print_ver_ unexpand printf
getlimits_

test "$LOCALE_FR_UTF8" != none || skip_ "French UTF-8 locale not available"
export LC_ALL="$LOCALE_FR_UTF8"

#input containing multibyte characters
cat > in <<\EOF
1234567812345678123456781
.       .       .       .
a       b       c       d
.       .       .       .
ä       ö       ü       ß
.       .       .       .
   äöü  .    öüä.       ä xx
EOF

cat > exp <<\EOF
1234567812345678123456781
.	.	.	.
a	b	c	d
.	.	.	.
ä	ö	ü	ß
.	.	.	.
   äöü	.    öüä.	ä xx
EOF

unexpand -a < in > out || fail=1
compare exp out > /dev/null 2>&1 || fail=1


#multiple files as an input
cat >> exp <<\EOF
1234567812345678123456781
.	.	.	.
a	b	c	d
.	.	.	.
ä	ö	ü	ß
.	.	.	.
   äöü	.    öüä.	ä xx
EOF


unexpand -a ./in ./in > out || fail=1
compare exp out > /dev/null 2>&1 || fail=1

#test characters with a display width larger than 1

env printf '12345678
e       |ascii(1)
\u00E9       |composed(1)
e\u0301       |decomposed(1)
\u3000      |ideo-space(2)
\u3000\u3000\u3000\u3000|ideo-space(2) * 4
\uFF0D      |full-hypen(2)
' > in || framework_failure_

env printf '12345678
e\t|ascii(1)
\u00E9\t|composed(1)
e\u0301\t|decomposed(1)
\t|ideo-space(2)
\t|ideo-space(2) * 4
\uFF0D\t|full-hypen(2)
' > exp || framework_failure_

unexpand -a < in > out || fail=1
compare exp out > /dev/null 2>&1 || fail=1

#test input where a blank of width > 1 is not being substituted
in="$(env printf ' \u3000  ö       ü       ß')"
exp=' 　  ö	     ü	     ß'

unexpand -a < in > out || fail=1
compare exp out > /dev/null 2>&1 || fail=1

#non-Unicode characters interspersed between Unicode ones
env printf '12345678
        \xFF|
\xFF       |
        \xFFä|
ä\xFF      |
        ä\xFF|
\xFF       ä|
äbcde\xFF  |
' > in || framework_failure_

env printf '12345678
\t\xFF|
\xFF\t|
\t\xFFä|
ä\xFF\t|
\tä\xFF|
\xFF\tä|
äbcde\xFF\t|
' > exp || framework_failure_

unexpand -a < in > out || fail=1
compare exp out > /dev/null 2>&1 || fail=1

#BOM header test 1
env printf "\xEF\xBB\xBF" > in; cat <<\EOF >> in || framework_failure_
1234567812345678123456781
.       .       .       .
a       b       c       d
.       .       .       .
ä       ö       ü       ß
.       .       .       .
   äöü  .    öüä.       ä xx
EOF

env printf "\xEF\xBB\xBF" > exp; cat <<\EOF >> exp || framework_failure_
1234567812345678123456781
.	.	.	.
a	b	c	d
.	.	.	.
ä	ö	ü	ß
.	.	.	.
   äöü	.    öüä.	ä xx
EOF

unexpand -a < in > out || fail=1
compare exp out > /dev/null 2>&1 || fail=1

env printf "\xEF\xBB\xBF" > exp; cat <<\EOF >> exp || framework_failure_
1234567812345678123456781
.	.	.	.
a	b	c	d
.	.	.	.
ä	ö	ü	ß
.	.	.	.
   äöü	.    öüä.	ä xx
EOF
env printf "\xEF\xBB\xBF" >> exp; cat <<\EOF >> exp || framework_failure_
1234567812345678123456781
.	.	.	.
a	b	c	d
.	.	.	.
ä	ö	ü	ß
.	.	.	.
   äöü	.    öüä.	ä xx
EOF

unexpand -a ./in ./in > out || fail=1
compare exp out > /dev/null 2>&1 || fail=1

# Ensure overflow is handed gracefully
# coreutils v9.11 induced a buffer overflow with mb_mul=4 (or 16).
for mb_mul in 4 6; do
  printf '   \n' | unexpand -t $(expr $SIZE_MAX / $mb_mul + 1) 2>err; ret=$?
  test "$ret" = 1 || test "$ret" = 0 || { cat err; fail=1; }
done

# A blank whose display width exceeds the tab distance must not overrun
# the pending-blank buffer.  With -t1 every column is a tab stop, so a
# width-2 ideographic space steps over the stop without landing on it;
# the run of blanks then grew pending_blank without bound.
ideo_space=$(env printf '\u3000')
{ yes "$ideo_space" | head -n 40000 | tr -d '\n'; echo; } |
  unexpand -t1 >out 2>err; ret=$?
test "$ret" = 0 || { cat err; fail=1; }

Exit $fail
