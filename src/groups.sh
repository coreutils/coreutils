#!/bin/sh
# groups -- print the groups a user is in
# Copyright (C) 1991, 1997, 2000, 2002 Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation,
# Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

# Written by David MacKenzie <djm@gnu.ai.mit.edu>.

# Make sure we get GNU id, if possible; also allow
# it to be somewhere else in PATH if not installed yet.
PATH=@bindir@:$PATH

usage="Usage: $0 [OPTION]... [USERNAME]...

  --help      display this help and exit
  --version   output version information and exit

Same as id -Gn.  If no USERNAME, use current process.

Report bugs to <@PACKAGE_BUGREPORT@>."

fail=0
case $# in
  1 )
    case "z${1}" in
      z--help )
	 echo "$usage" || fail=1; exit $fail;;
      z--version )
	 echo "groups (@GNU_PACKAGE@) @VERSION@" || fail=1; exit $fail;;
      * ) ;;
    esac
    ;;
  * ) ;;
esac

if [ $# -eq 0 ]; then
  id -Gn
  fail=$?
else
  for name in "$@"; do
    groups=`id -Gn -- $name`
    status=$?
    if test $status = 0; then
      echo $name : $groups
    else
      fail=$status
    fi
  done
fi
exit $fail
