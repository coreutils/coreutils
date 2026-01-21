#!/bin/sh
# Test df's behavior when /proc cannot be read.
# This is an alternative for no-mtab-status.sh for static binaries.
# This test is skipped if User namespace sandbox is unavailable.

# Copyright (C) 2025-2026 Free Software Foundation, Inc.

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
print_ver_ df
skip_if_root_

# Sanitizers need to read from /proc
sanitizer_build_ df && skip_ 'Sanitizer not supported'

# Protect against inaccessible remote mounts etc.
timeout 10 df || skip_ "df fails"

unshare -rm true || skip_ 'User namespace sandbox is disabled'

# mask /proc
df() {
  unshare -rm $SHELL -c \
    "mount -t tmpfs tmpfs /proc && env df \"\$@\"" -- "$@";
}

df /proc || fail=1
returns_ 1 df /proc/self || framework_failure_

# Keep the following in sync with no-mtab-status.sh

# These tests are supposed to succeed:
df '.' || fail=1
df -i '.' || fail=1
df -T '.' || fail=1
df -Ti '.' || fail=1
df --total '.' || fail=1

# These tests are supposed to fail:
returns_ 1 df || fail=1
returns_ 1 df -i || fail=1
returns_ 1 df -T || fail=1
returns_ 1 df -Ti || fail=1
returns_ 1 df --total || fail=1

returns_ 1 df -a || fail=1
returns_ 1 df -a '.' || fail=1

returns_ 1 df -l || fail=1
returns_ 1 df -l '.' || fail=1

returns_ 1 df -t hello || fail=1
returns_ 1 df -t hello '.' || fail=1

returns_ 1 df -x hello || fail=1
returns_ 1 df -x hello '.' || fail=1

Exit $fail
