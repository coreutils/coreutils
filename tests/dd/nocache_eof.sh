#!/bin/sh
# Ensure dd invalidates to EOF when appropriate

# Copyright (C) 2017-2025 Free Software Foundation, Inc.

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
print_ver_ dd
require_strace_ fadvise64,fadvise64_64

head -c1234567 /dev/zero > in.f || framework_failure_

# Check basic operation or skip.
# We could check for 'Operation not supported' error here,
# but that was seen to be brittle. HPUX returns ENOTTY for example.
# So assume that if this basic operation fails, it's due to lack
# of support by the system.
dd if=in.f iflag=nocache count=0 ||
  skip_ 'this file system lacks support for posix_fadvise()'

strace_dd() {
  strace -o dd.strace -e fadvise64,fadvise64_64 dd status=none "$@" || fail=1
}

advised_to_eof() {
  grep -F ' 0, POSIX_FADV_DONTNEED' dd.strace >/dev/null
}

# The commented fadvise64 calls are what are expected with
# a 4KiB page size and 128KiB IO_BUFSIZE.

strace_dd if=in.f of=out.f bs=1M oflag=nocache,sync
#fadvise64(1, 0, 1048576, POSIX_FADV_DONTNEED) = 0
#fadvise64(1, 1048576, 131072, POSIX_FADV_DONTNEED) = 0
#fadvise64(1, 1179648, 0, POSIX_FADV_DONTNEED) = 0
advised_to_eof || fail=1

strace_dd if=in.f count=0 iflag=nocache
#fadvise64(0, 0, 0, POSIX_FADV_DONTNEED) = 0
advised_to_eof || fail=1

strace_dd if=in.f of=/dev/null iflag=nocache skip=10 count=300
#fadvise64(0, 5120, 131072, POSIX_FADV_DONTNEED) = 0
#fadvise64(0, 136192, 22528, POSIX_FADV_DONTNEED) = 0
returns_ 1 advised_to_eof || fail=1

strace_dd if=in.f of=/dev/null iflag=nocache bs=1M count=3000
#fadvise64(0, 0, 1048576, POSIX_FADV_DONTNEED) = 0
#fadvise64(0, 1048576, 131072, POSIX_FADV_DONTNEED) = 0
#fadvise64(0, 1179648, 0, POSIX_FADV_DONTNEED) = 0
advised_to_eof || fail=1

strace_dd if=in.f of=/dev/null bs=1M count=1 iflag=nocache
#fadvise64(0, 0, 1048576, POSIX_FADV_DONTNEED) = 0
returns_ 1 advised_to_eof || fail=1

strace_dd if=in.f of=out.f bs=1M iflag=nocache oflag=nocache,sync
#fadvise64(0, 0, 1048576, POSIX_FADV_DONTNEED) = 0
#fadvise64(1, 0, 1048576, POSIX_FADV_DONTNEED) = 0
#fadvise64(0, 1048576, 131072, POSIX_FADV_DONTNEED) = 0
#fadvise64(1, 1048576, 131072, POSIX_FADV_DONTNEED) = 0
#fadvise64(0, 1179648, 0, POSIX_FADV_DONTNEED) = 0
#fadvise64(1, 1179648, 0, POSIX_FADV_DONTNEED) = 0
advised_to_eof || fail=1

# Ensure sub page size offsets are handled.
# I.e., only page aligned offsets are sent to fadvise.
if ! strace -o dd.strace -e fadvise64,fadvise64_64 dd status=none \
 if=in.f of=out.f bs=1M oflag=direct oseek=512B; then
  warn_ '512 byte aligned O_DIRECT is not supported on this (file) system'
  # The current file system may not support O_DIRECT,
  # or older XFS had a page size alignment requirement
else
  #The first call is redundant but inconsequential
  #fadvise64(1, 1048576, 0, POSIX_FADV_DONTNEED) = 0
  #fadvise64(1, 1048576, 0, POSIX_FADV_DONTNEED) = 0
  advised_to_eof || fail=1

  strace_dd if=in.f of=out.f bs=1M oflag=direct
  #fadvise64(1, 1048576, 0, POSIX_FADV_DONTNEED) = 0
  #fadvise64(1, 1048576, 0, POSIX_FADV_DONTNEED) = 0
  advised_to_eof || fail=1
fi

Exit $fail
