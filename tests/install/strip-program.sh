#!/bin/sh
# Ensure "install -s --strip-program=PROGRAM" uses the program to strip

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
print_ver_ ginstall

working_umask_or_skip_

cat <<EOF > b || framework_failure_
#!$SHELL
sed s/b/B/ \$1 > \$1.t && mv \$1.t \$1
EOF
chmod a+x b || framework_failure_

echo abc > src || framework_failure_
echo aBc > exp || framework_failure_
ginstall src dest -s --strip-program=./b || fail=1
compare exp dest || fail=1

# Check that install cleans up properly if strip fails.
returns_ 1 ginstall src dest2 -s --strip-program=./FOO || fail=1
test -e dest2 && fail=1

# Ensure naked hyphens not passed
cat <<EOF > no-hyphen || framework_failure_
#!$SHELL
printf -- '%s\\n' "\$1" | grep '^[^-]'
EOF
chmod a+x no-hyphen || framework_failure_

ginstall -s --strip-program=./no-hyphen -- src -dest || fail=1

Exit $fail
