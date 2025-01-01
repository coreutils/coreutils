#!/bin/sh
# make sure chgrp --from=... works

# Copyright (C) 2023-2025 Free Software Foundation, Inc.

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
print_ver_ chgrp
require_root_

touch f || framework_failure_

chgrp -R --preserve-root 1 f

# Make sure the owner and group are 0 and 1 respectively.
set _ $(ls -n f); shift; test ":$4" = ':1' || fail=1

# Make sure the correct diagnostic is output
# Note we output a name even though an id was specified.
chgrp -v --from=42 43 f > out || fail=1
printf "group of 'f' retained as $(id -nu 1)\n" > exp
compare exp out || fail=1

chgrp --from=:1 010 f || fail=1

# And now they should be 10
set _ $(ls -n f); shift; test ":$4" = ':10' || fail=1

ln -s f slink
# Applying chgrp to a symlink with --no-dereference
# should change only the link.
chgrp --no-dereference 1 slink || fail=1
# owner/group on the symlink should be set
set _ $(ls -n slink); shift; test ":$4" = ':1' || fail=1
# owner/group on the referent should remain unchanged
set _ $(ls -n f);     shift; test ":$4" = ':10' || fail=1

chgrp --no-dereference --from=:1 010 slink || fail=1
# owner/group on the symlink should be changed
set _ $(ls -n slink); shift; test ":$4" = ':10' || fail=1

Exit $fail
