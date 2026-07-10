#!/bin/sh
# exercise the -m option

# Copyright (C) 2003-2026 Free Software Foundation, Inc.

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
print_ver_ ls

seq 2000 > b || framework_failure_
touch a || framework_failure_


# Before coreutils-5.1.1, the following would output a space after the comma.
ls -w2 -m a b > out || fail=1

# Before coreutils-5.1.1, the following would produce leading white space.
# All of the sed business is because the sizes are not portable.
ls -sm a b | sed 's/^[0-9]/0/;s/, [0-9][0-9]* b/, 12 b/' >> out || fail=1
cat <<\EOF > exp || framework_failure_
a,
b
0 a, 12 b
EOF

compare exp out || fail=1

# Ensure exact-fit comma output accounts for the trailing separator.
touch bb c || framework_failure_
cat <<\EOF > exp || framework_failure_
a,
bb, c
EOF
ls -w5 -m a bb c > out || fail=1
compare exp out || fail=1

printf '%s\n' 'a, bb' > exp || framework_failure_
ls -w5 -m a bb > out || fail=1
compare exp out || fail=1

# Ensure commas in names are quoted appropriately for all quoting styles.
# Without this quoting, interactive output could become confusing,
# especially in the presence of NBSP etc.
touch 'com,ma' || framework_failure_
cat <<\EOF > exp || framework_failure_
literal: com,ma
shell: 'com,ma'
shell-always: 'com,ma'
shell-escape: 'com,ma'
shell-escape-always: 'com,ma'
c: "com,ma"
c-maybe: "com,ma"
escape: com\,ma
locale: 'com,ma'
clocale: "com,ma"
EOF
for qs in $(cut -d: -f1 exp); do
  printf '%s: ' "$qs"
  ls -m --quoting-style="$qs" 'com,ma' || fail=1
done > out
compare exp out || fail=1

# Newlines are preserved with unlimited width, where -m does not wrap,
newline='n
l'
touch "$newline" || framework_failure_
printf '%s\n' "$newline" > exp || framework_failure_
ls -m -w0 "$newline" > out || fail=1
compare exp out || fail=1

# Otherwise protect newlines that could be confused with separators.
printf '%s\n' 'n?l' > exp || framework_failure_
ls -m "$newline" > out || fail=1
compare exp out || fail=1

Exit $fail
