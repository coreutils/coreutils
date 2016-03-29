#!/bin/sh
# Make sure chgrp gives the right diagnostic for a readable,
# but inaccessible directory.

# Copyright (C) 2003-2016 Free Software Foundation, Inc.

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
print_ver_ chgrp
require_membership_in_two_groups_
skip_if_root_

set _ $groups; shift
g1=$1
g2=$2

mkdir -p d/no-x/y || framework_failure_
chmod u=rw d/no-x || framework_failure_


# This must exit nonzero.
chgrp -R $g2 d >/dev/null 2>out && fail=1

prog=chgrp
# NOTE: this code is the same for all tests/*/no-x tests.
# Depending on whether fts is using native fdopendir, we see one
# of the following diagnostics (note also the /y suffix in one case):
#   prog: 'd/no-x': Permission denied
#   prog: cannot access 'd/no-x/y': Permission denied
#   prog: cannot read directory 'd/no-x': Permission denied
# Convert either of the latter two to the first one.
sed "s/^$prog: cannot access /$prog: /" out > t && mv t out
sed "s/^$prog: cannot read directory /$prog: /" out > t && mv t out
sed 's,d/no-x/y,d/no-x,' out > t && mv t out

cat <<EOF > exp
$prog: 'd/no-x': Permission denied
EOF

compare exp out || fail=1

Exit $fail
