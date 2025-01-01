#!/bin/sh
# Test that cp --keep-directory-symlink follows symlinks.

# Copyright (C) 2024-2025 Free Software Foundation, Inc.

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

mkdir -p a/b b/d/e || framework_failure_
ln -s b a/d || framework_failure_

cp -RT --copy-contents b a || fail=1
cp -RT --copy-contents --keep-directory-symlink b a || fail=1
ls a/b/e || fail=1
