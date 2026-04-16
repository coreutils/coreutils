#!/bin/sh
# Test stat interactions with mounts

# Copyright (C) 2010-2026 Free Software Foundation, Inc.

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
print_ver_ stat

stat_mnt=$(stat -c%m .) || fail=1
case "$stat_mnt" in
  /*) ;;
  *) fail=1;;
esac

# Ensure stat works without mounting /proc
hide_proc() {
  unshare -rm $SHELL -c 'mount -t tmpfs tmpfs /proc && "$@"' -- "$@"
}
if hide_proc true; then
  hide_proc mount; ret=$?
  if test "$ret" = 2; then  # Avoid segfaults under unshare on Guix
    hide_proc stat -c '0%#a' / || fail=1
  fi
fi

Exit $fail
