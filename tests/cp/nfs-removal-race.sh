#!/bin/sh
# Running cp S D on an NFS client while another client has just removed D
# would lead (w/coreutils-8.16 and earlier) to cp's initial stat call
# seeing (via stale NFS cache) that D exists, so that cp would then call
# open without the O_CREAT flag.  Yet, the open must actually consult
# the server, which confesses that D has been deleted, thus causing the
# open call to fail with ENOENT.
#
# This test simulates that situation by intercepting stat for a nonexistent
# destination, D, and making the stat fill in the result struct for another
# file and return 0.

# Copyright (C) 2012-2025 Free Software Foundation, Inc.

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

touch S || framework_failure_

strace -o strace.log -P D -e inject=%stat:retval=0 -e inject=openat:error=ENOENT:when=1 cp S D || fail=1
grep INJECTED strace.log || skip_ 'syscall openat is not injected?'

compare S D || fail=1
Exit $fail
