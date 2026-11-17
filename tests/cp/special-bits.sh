#!/bin/sh
# make sure 'cp -p' preserves special bits
# This works only when run as root.

# Copyright (C) 2000-2026 Free Software Foundation, Inc.

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

# This test would fail due to a bug introduced in 4.0y.
# The bug was fixed in 4.0z.

. "${srcdir=.}/tests/init.sh"; path_prepend_ ./src
print_ver_ cp
require_root_

touch a b c || framework_failure_
chmod u+sx,go= a || framework_failure_
chmod u=rwx,g=sx,o= b || framework_failure_
chmod a=r,ug+sx c || framework_failure_
chown $NON_ROOT_USERNAME . || framework_failure_
chmod u=rwx,g=rx,o=rx . || framework_failure_


cp -p a a2 || fail=1
set _ $(ls -l a); shift; p1=$1
set _ $(ls -l a2); shift; p2=$1
test $p1 = $p2 || fail=1

cp -p b b2 || fail=1
set _ $(ls -l b); shift; p1=$1
set _ $(ls -l b2); shift; p2=$1
test $p1 = $p2 || fail=1

chroot --skip-chdir --user=$NON_ROOT_USERNAME / env PATH="$PATH" cp -p c c2 \
  || fail=1
set _ $(ls -l c); shift; p1=$1
set _ $(ls -l c2); shift; p2=$1
test $p1 = $p2 && fail=1

# Likewise for parent directories created by --recursive and --parents.
mkdir -p d/e &&
touch d/e/f &&
chmod a=rx,ug+s,+t d d/e || framework_failure_
for opt in --recursive --parents; do
  rm -rf dst dst2 &&
  mkdir -p dst dst2 &&
  chown $NON_ROOT_USERNAME dst2 || framework_failure_

  src=d
  test $opt = --parents && src=d/e/f

  cp -p $opt $src dst || fail=1
  set _ $(ls -ld d); shift; p1=$1
  set _ $(ls -ld dst/d); shift; p2=$1
  set _ $(ls -ld dst/d/e); shift; p3=$1
  test $p1 = $p2 || fail=1
  test $p1 = $p3 || fail=1

  # coreutils-9.12 and earlier would not clear special mode bits
  # on parent directories created by --parents.
  chroot --skip-chdir --user=$NON_ROOT_USERNAME / env PATH="$PATH" \
    cp -p $opt $src dst2 || fail=1
  set _ $(ls -ld dst2/d); shift; p2=$1
  set _ $(ls -ld dst2/d/e); shift; p3=$1
  case $p2 in dr-xr-xr-x*) ;; *) fail=1 ;; esac
  case $p3 in dr-xr-xr-x*) ;; *) fail=1 ;; esac
done

# Test the behavior when --preserve=ownership is used.
for opt in --recursive --parents; do
  rm -rf dst dst2 &&
  mkdir -p dst dst2 &&
  chown $NON_ROOT_USERNAME dst2 || framework_failure_

  src=d
  test $opt = --parents && src=d/e/f

  cp --preserve=ownership $opt $src dst || fail=1
  set _ $(ls -ld dst/d); shift; p2=$1
  set _ $(ls -ld dst/d/e); shift; p3=$1
  case $p2 in dr-xr-xr-t*) ;; *) fail=1 ;; esac
  case $p3 in dr-xr-xr-t*) ;; *) fail=1 ;; esac

  chroot --skip-chdir --user=$NON_ROOT_USERNAME / env PATH="$PATH" \
    cp --preserve=ownership $opt $src dst2 || fail=1
  set _ $(ls -ld dst2/d); shift; p2=$1
  set _ $(ls -ld dst2/d/e); shift; p3=$1
  case $p2 in dr-xr-xr-t*) ;; *) fail=1 ;; esac
  case $p3 in dr-xr-xr-t*) ;; *) fail=1 ;; esac
done

Exit $fail
