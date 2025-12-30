#!/bin/sh
# Test df's behavior when the /proc cannot be read.
# This is an alternative for no-mtab-status.sh missing LD_PRELOAD supoort.
# This test is skipped if User namespace sandbox is disabled.

# Copyright (C) 2025 Free Software Foundation, Inc.

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

# Protect against inaccessible remote mounts etc.
timeout 10 df || skip_ "df fails"

unshare -rm true || skip_ 'User namespace sandbox is disabled'

df
# mask /proc
df() { unshare -rm bash -c "mount -t tmpfs tmpfs /proc && command df \"\$@\"" -- "$@"; }

df /proc || fail=1
df returns_ 1 /proc/self || fail=1

# Do same tests with no-mtab-status.sh
df '.' || fail=1
df -i '.' || fail=1
df -T '.' || fail=1
df -Ti '.' || fail=1
df --total '.' || fail=1

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
