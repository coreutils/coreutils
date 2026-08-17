#!/bin/sh
# Ensure cp --preserves copies capabilities

# Copyright (C) 2010-2026 Free Software Foundation, Inc.

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
print_ver_ cp
require_root_
working_umask_or_skip_


grep '^#define HAVE_CAP 1' $CONFIG_HEADER > /dev/null \
  || skip_ "configured without libcap support"

(setcap --help) 2>&1 |grep 'usage: setcap' > /dev/null \
  || skip_ "setcap utility not found"
(getcap --help) 2>&1 |grep 'usage: getcap' > /dev/null \
  || skip_ "getcap utility not found"


touch file || framework_failure_
chown $NON_ROOT_USERNAME file || framework_failure_

setcap 'cap_net_bind_service=ep' file ||
  skip_ "setcap doesn't work"
getcap file | grep cap_net_bind_service >/dev/null ||
  skip_ "getcap doesn't work"

# When configured with --disable-xattr, 'cp --preserve=xattr' will fail
# before the copy is created because it cannot preserve extended attributes.
cp --preserve=xattr file copy1 >out 2>err
ret=$?
compare /dev/null out || fail=1
if ! grep '^#define USE_XATTR 1' $CONFIG_HEADER > /dev/null; then
  cat <<\EOF >exp || framework_failure_
cp: cannot preserve extended attributes, cp is built without xattr support
EOF
  compare exp err || fail=1
  test "$ret" = 1 || fail=1
  returns_ 1 test -f copy1 || fail=1
else
  compare /dev/null err || fail=1
  test "$ret" = 0 || fail=1
fi

# Before coreutils 8.5 the capabilities would not be preserved,
# as the owner was set _after_ copying xattrs, thus clearing any capabilities.
cp --preserve=all file copy2 >out >err || fail=1
compare /dev/null out || fail=1
compare /dev/null err || fail=1

# When configured with --disable-xattr, 'cp --preserve=all' will exit
# normally without extended attributes copied.
if ! grep '^#define USE_XATTR 1' $CONFIG_HEADER > /dev/null; then
  copies=copy2
  check_capabilities ()
  {
    getcap $1 | returns_ 1 grep cap_net_bind_service >/dev/null || fail=1
  }
else
  copies='copy1 copy2'
  check_capabilities ()
  {
    getcap $1 | grep cap_net_bind_service >/dev/null || fail=1
  }
fi

for file in $copies; do
  check_capabilities $file
done

Exit $fail
