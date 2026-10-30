#!/bin/sh
# Test df's behavior when the mount list cannot be read.

# Copyright (C) 2012-2026 Free Software Foundation, Inc.

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
uses_strace_

# Protect against inaccessible remote mounts etc.
timeout 10 df || skip_ "df fails"

no-mtab() {
  strace -o /dev/null \
    -e quiet=path-resolution \
    -P /proc/self/mountinfo \
    -P /proc/mounts \
    -P /etc/mtab \
    -P /proc/self/mounts \
    -e fault=openat \
    "$@"
}

no-mtab true || skip_ 'strace injection for openat failed'

df() {
  no-mtab df "$@"
}

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
