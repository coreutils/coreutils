#!/bin/sh
# ensure that an empty "ca=" attribute disables ls's capability-checking

# Copyright (C) 2008-2025 Free Software Foundation, Inc.

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
print_ver_ ls
require_strace_ capget
require_root_

touch file || framework_failure_

setcap 'cap_net_bind_service=ep' file ||
  skip_ "setcap doesn't work"

LS_COLORS=ca=1; export LS_COLORS
strace -e capget ls --color=always > /dev/null 2> out || fail=1
$EGREP 'capget\(' out || skip_ "your ls doesn't call capget"

LS_COLORS=ca=:; export LS_COLORS
strace -e capget ls --color=always > /dev/null 2> out || fail=1
$EGREP 'capget\(' out && fail=1

Exit $fail
