#!/bin/sh
# make sure that dd doesn't allocate memory unnecessarily

# Copyright (C) 2013 Free Software Foundation, Inc.

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
print_ver_ dd
require_ulimit_v_

# count and skip is zero, we don't need to allocate memory
(ulimit -v 20000; dd  bs=30M count=0) || fail=1
(ulimit -v 20000; dd ibs=30M count=0) || fail=1
(ulimit -v 20000; dd obs=30M count=0) || fail=1


# Use a fifo for which seek fails, but read does not
if mkfifo tape; then
  # for non seekable output we need to allocate buffer when needed
  echo 1 > tape&
  (ulimit -v 20000; dd  bs=30M skip=1 count=0 if=tape) && fail=1

  echo 1 > tape&
  (ulimit -v 20000; dd ibs=30M skip=1 count=0 if=tape) && fail=1

  echo 1 > tape&
  (ulimit -v 20000; dd obs=30M skip=1 count=0 if=tape) || fail=1


  # for non seekable output we need to allocate buffer when needed
  echo 1 > tape&
  (ulimit -v 20000; dd  bs=30M seek=1 count=0 of=tape) && fail=1

  echo 1 > tape&
  (ulimit -v 20000; dd obs=30M seek=1 count=0 of=tape) && fail=1

  echo 1 > tape&
  (ulimit -v 20000; dd ibs=30M seek=1 count=0 of=tape) || fail=1
fi

Exit $fail
