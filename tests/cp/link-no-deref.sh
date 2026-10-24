#!/bin/sh
# Ensure that cp --link --no-dereference works properly

# Copyright (C) 2006-2026 Free Software Foundation, Inc.

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
print_ver_ cp printf
getlimits_
uses_strace_

ln -s no-such-file dangling-slink || framework_failure_


# Prior to coreutils-6.0, this would fail on non-Linux kernels,
# with link being applied to the dangling symlink.
cp --link --no-dereference dangling-slink d2 || fail=1

fail_chown() {
  chowns=chown,lchown,fchownat
  eval "file=\${$#}"
  strace --quiet=all -o fc.strace -P "$file" -e inject=$chowns:error=ENOSYS "$@"
}

NL='
'
if fail_chown true foo; then
  env printf 'cp: %s %q: %s\n' \
   'failed to preserve ownership for' "$NL" "$ENOSYS" >exp &&
  touch file && ln -s file slink || framework_failure_

  returns_ 1 fail_chown cp -P --preserve=ownership slink "$NL" \
    >out 2>err || fail=1

  grep 'chown.*INJECTED' fc.strace || { cat fc.strace; framework_failure_; }
  compare exp err || fail=1
  compare /dev/null out || fail=1
fi

Exit $fail
