#!/bin/sh
# groups -- print the groups a user is in
# Copyright (C) 1991 Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

# Written by David MacKenzie <djm@gnu.ai.mit.edu>.

# Make sure we get GNU id, if possible; also allow
# it to be somewhere else in PATH if not installed yet.
PATH=@bindir@:$PATH

if [ $# -eq 0 ]; then
  id -Gn
else
  for name in "$@"; do
    echo $name : `id -Gn $name`
  done
fi
