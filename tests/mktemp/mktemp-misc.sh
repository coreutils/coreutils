#!/bin/sh
# Test 'mktemp' under specialized situations.

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
print_ver_ mktemp
uses_strace_

# ensure randomization doesn't depend solely on ASLR
# Since this is a probabilistic test, we use a long template.
mktemp_rand() { setarch -R mktemp -u XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX; }
mktemp_rand >out1 && mktemp_rand >out2 && compare out1 out2 && fail=1

# ensure mktemp does not strongly depend on getrandom
if strace -o /dev/null -e inject=getrandom:error=ENOSYS true; then
  strace -o /dev/null -e inject=getrandom:error=ENOSYS mktemp -u || fail=1
fi

Exit $fail
