#!/bin/sh
# Exercise shred --remove

# Copyright (C) 1999-2025 Free Software Foundation, Inc.

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
print_ver_ shred
skip_if_root_

# The length of the basename is what matters.
# In this case, shred-4.0l would try to rename the file 256^10 times
# before terminating.
file=0123456789
touch $file || framework_failure_
chmod u-w $file || framework_failure_

# This would take so long that it appears to infloop
# when using version from fileutils-4.0k.
# When the command completes, expect it to fail.
returns_ 1 shred -u $file > /dev/null 2>&1 || fail=1
rm -f $file || framework_failure_

# Ensure all --remove methods at least unlink the file
for mode in '' '=unlink' '=wipe' '=wipesync'; do
  touch $file || framework_failure_
  shred -n0 --remove"$mode" $file || fail=1
  test -e $file && fail=1
done

# Ensure incorrect params are diagnosed
touch $file || framework_failure_
returns_ 1 shred -n0 --remove=none $file 2>/dev/null || fail=1

# Ensure rename passes complete.
# coreutils-8.28 did not do the decreasing length rename
# which may have leaked the length of the removed file name
 # test level exhaustion
touch \
 0 1 2 3 4 5 6 7 8 9 \
 a b c d e f g h i j k l m n o p q r s t u v w x y z \
 A B C D E F G H I J K L M N O P Q R S T U V W X Y Z _ || framework_failure_
touch test 000 || framework_failure_  # test level increment
shred -vu test 2>out || fail=1
cat <<\EOF >exp || framework_failure_
shred: test: removing
shred: test: renamed to 0000
shred: 0000: renamed to 001
shred: 001: renamed to 00
shred: test: removed
EOF
compare exp out || fail=1

# Ensure renames are only retried for EEXIST
mkdir rodir && cd rodir && touch $file && chmod a-w . || framework_failure_
returns_ 1 timeout 10 shred -u $file || fail=1

Exit $fail
