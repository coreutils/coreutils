#!/bin/sh
# Test "ln --relative".

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
print_ver_ ln

mkdir -p usr/bin || framework_failure_
mkdir -p usr/lib/foo || framework_failure_
touch usr/lib/foo/foo || framework_failure_

ln -sr usr/lib/foo/foo usr/bin/foo
test $(readlink usr/bin/foo) = '../lib/foo/foo' || fail=1

ln -sr usr/bin/foo usr/lib/foo/link-to-foo
test $(readlink usr/lib/foo/link-to-foo) = 'foo' || fail=1

# Correctly update an existing link, which was broken in <= 8.21
ln -s dir1/dir2/f existing_link
ln -srf here existing_link
test $(readlink existing_link) = 'here' || fail=1

# Demonstrate resolved symlinks used to generate relative links
# so here, 'web/latest' will not be linked to the intermediate 'latest' link.
# You'd probably want to use realpath(1) in conjunction
# with ln(1) without --relative to give greater control.
ln -s release1 alpha
ln -s release2 beta
ln -s beta latest
mkdir web
ln -sr latest web/latest
test $(readlink web/latest) = '../release2' || fail=1

# This would replace d/a with a symbolic link to itself with coreutils-8.31
# through coreutils-9.12.
echo a >exp-a &&
mkdir d &&
cp exp-a d/a &&
echo "ln: 'd/a' and 'd/a' are the same file" >exp-err || framework_failure_
returns_ 1 ln -srf d/a d/a >out 2>err || fail=1
compare /dev/null out || fail=1
compare exp-err err || fail=1
compare exp-a d/a || fail=1

# Likewise, using a symbolic link target directory.
ln -s d d-slink &&
echo "ln: 'd/a' and 'd-slink/a' are the same file" >exp-err \
  || framework_failure_
returns_ 1 ln -srf d/a d-slink >out 2>err || fail=1
compare /dev/null out || fail=1
compare exp-err err || fail=1
compare exp-a d/a || fail=1

# A relative operand that cannot be canonicalized must not be installed
# verbatim in another directory.  An absolute operand can be kept as given.
ln -s cycle/x cycle || framework_failure_
returns_ 1 ln -srf cycle d/a || fail=1
compare exp-a d/a || fail=1
abs_cycle=$(pwd)/./cycle
ln -sr "$abs_cycle" web/cycle || fail=1
test "$(readlink web/cycle)" = "$abs_cycle" || fail=1

# Preserve an empty target on systems that accept one (e.g., FreeBSD).
# Prior to coreutils-8.23, this would segfault.
if ln -s '' plain 2>/dev/null; then
  ln -sr '' F || fail=1
  test -L F || fail=1
else
  returns_ 1 ln -sr '' F || fail=1
fi

Exit $fail
