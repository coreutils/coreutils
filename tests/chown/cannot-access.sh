#!/bin/sh
# ensure chown and chgrp say "cannot access" for a missing operand,
# and "cannot dereference" only for a dangling symlink

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
print_ver_ chown chgrp

touch f &&
mkdir dir &&
ln -s no-such dangle &&
user=$(id -u) &&
group=$(id -g) || framework_failure_

for prog in chown chgrp; do
  case $prog in
    chown) owner=$user ;;
    chgrp) owner=$group ;;
  esac

  returns_ 1 $prog "+$owner" nosuch f/ dir/missing dangle 2> err || fail=1
  sed 's/: [^:]*$//' err > out || framework_failure_
  cat <<EOF > exp || framework_failure_
$prog: cannot access 'nosuch'
$prog: cannot access 'f/'
$prog: cannot access 'dir/missing'
$prog: cannot dereference 'dangle'
EOF
  compare exp out || fail=1

  # With -h, a missing operand is still reported as "cannot access".
  returns_ 1 $prog -h "+$owner" nosuch 2> err || fail=1
  sed 's/: [^:]*$//' err > out &&
  echo "$prog: cannot access 'nosuch'" > exp || framework_failure_
  compare exp out || fail=1
done

Exit $fail
