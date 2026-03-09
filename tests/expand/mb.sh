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
print_ver_ expand printf

test "$LOCALE_FR_UTF8" != none || skip_ "French UTF-8 locale not available"
export LC_ALL="$LOCALE_FR_UTF8"

#input containing multibyte characters
cat <<\EOF > in || framework_failure_
1234567812345678123456781
.       .       .       .
a	b	c	d
.       .       .       .
ä	ö	ü	ß
.       .       .       .
EOF
env printf '   äöü\t.    öüä.   \tä xx\n' >> in || framework_failure_

cat <<\EOF > exp || framework_failure_
1234567812345678123456781
.       .       .       .
a       b       c       d
.       .       .       .
ä       ö       ü       ß
.       .       .       .
   äöü  .    öüä.       ä xx
EOF

expand < in > out || fail=1
compare exp out > /dev/null 2>&1 || fail=1

#multiple files as an input
cat <<\EOF >> exp || framework_failure_
1234567812345678123456781
.       .       .       .
a       b       c       d
.       .       .       .
ä       ö       ü       ß
.       .       .       .
   äöü  .    öüä.       ä xx
EOF

expand ./in ./in > out || fail=1
compare exp out > /dev/null 2>&1 || fail=1

#test characters with display widths != 1
env printf '12345678
e\t|ascii(1)
\u00E9\t|composed(1)
e\u0301\t|decomposed(1)
\u3000\t|ideo-space(2)
\uFF0D\t|full-hypen(2)
' > in || framework_failure_

env printf '12345678
e       |ascii(1)
\u00E9       |composed(1)
e\u0301       |decomposed(1)
\u3000      |ideo-space(2)
\uFF0D      |full-hypen(2)
' > exp || framework_failure_

expand < in > out || fail=1
compare exp out > /dev/null 2>&1 || fail=1

#shouldn't fail with "input line too long"
#when a line starts with a control character
env printf '\n' > in || framework_failure_

expand < in > out || fail=1
compare in out > /dev/null 2>&1 || fail=1

#non-Unicode characters interspersed between Unicode ones
env printf '12345678
\t\xFF|
\xFF\t|
\t\xFFä|
ä\xFF\t|
\tä\xFF|
\xFF\tä|
äbcdef\xFF\t|
' > in || framework_failure_

env printf '12345678
        \xFF|
\xFF       |
        \xFFä|
ä\xFF      |
        ä\xFF|
\xFF       ä|
äbcdef\xFF |
' > exp || framework_failure_

expand < in > out || fail=1
compare exp out > /dev/null 2>&1 || fail=1



#BOM header test 1
env printf "\xEF\xBB\xBF" > in; cat <<\EOF >> in || framework_failure_
1234567812345678123456781
.       .       .       .
a	b	c	d
.       .       .       .
ä	ö	ü	ß
.       .       .       .
EOF
env printf '   äöü\t.    öüä.   \tä xx\n' >> in || framework_failure_

env printf "\xEF\xBB\xBF" > exp; cat <<\EOF >> exp || framework_failure_
1234567812345678123456781
.       .       .       .
a       b       c       d
.       .       .       .
ä       ö       ü       ß
.       .       .       .
   äöü  .    öüä.       ä xx
EOF

expand < in > out || fail=1
compare exp out > /dev/null 2>&1 || fail=1

env printf '\xEF\xBB\xBF' > in1; cat <<\EOF >> in1 || framework_failure_
1234567812345678123456781
.       .       .       .
a	b	c	d
.       .       .       .
ä	ö	ü	ß
.       .       .       .
EOF
env printf '   äöü\t.    öüä.   \tä xx\n' >> in1 || framework_failure_


env printf '\xEF\xBB\xBF' > exp; cat <<\EOF >> exp || framework_failure_
1234567812345678123456781
.       .       .       .
a       b       c       d
.       .       .       .
ä       ö       ü       ß
.       .       .       .
   äöü  .    öüä.       ä xx
EOF
env printf '\xEF\xBB\xBF' >> exp; cat <<\EOF >> exp || framework_failure_
1234567812345678123456781
.       .       .       .
a       b       c       d
.       .       .       .
ä       ö       ü       ß
.       .       .       .
   äöü  .    öüä.       ä xx
EOF

expand in1 in1 > out || fail=1
compare exp out > /dev/null 2>&1 || fail=1

Exit $fail
