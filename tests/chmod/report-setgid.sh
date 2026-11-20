#!/bin/sh
# Verify chmod's output when the kernel silently clears the set-group-ID bit.

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
print_ver_ chmod
require_root_

# Let the owner change the directory mode without belonging to its group.
chmod go+x . && mkdir d && chown "$NON_ROOT_USERNAME:+0" d &&
  chmod =755 d || framework_failure_
as_owner_()
{
  chroot --skip-chdir --userspec="$NON_ROOT_USERNAME:+1" --groups='' / "$@"
}

as_owner_ chmod g+s d && test ! -g d ||
  skip_ 'chmod g+s must was not ignored'

as_owner_ chmod -v g+s d >out || fail=1
echo "mode of 'd' retained as 0755 (rwxr-xr-x)" >exp
compare exp out || fail=1

as_owner_ chmod -c g+ws d >out || fail=1
echo "mode of 'd' changed from 0755 (rwxr-xr-x) to 0775 (rwxrwxr-x)" >exp
compare exp out || fail=1

Exit $fail
