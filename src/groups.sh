#!/bin/sh
# groups -- print the groups a user is in
# Copyright (C) 1991, 1997, 2000, 2002, 2004-2007 Free Software Foundation, Inc.

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
# Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

# Written by David MacKenzie <djm@gnu.ai.mit.edu>.

# Make sure we get GNU id, if possible; also allow
# it to be somewhere else in PATH if not installed yet.
PATH=@bindir@:$PATH

usage="Usage: $0 [OPTION]... [USERNAME]...

  --help      display this help and exit
  --version   output version information and exit

Same as id -Gn.  If no USERNAME, use current process.

Report bugs to <@PACKAGE_BUGREPORT@>."

version='groups (@GNU_PACKAGE@) @VERSION@
Copyright (C) @RELEASE_YEAR@ Free Software Foundation, Inc.
This is free software.  You may redistribute copies of it under the terms of
the GNU General Public License <http://www.gnu.org/licenses/gpl.html>.
There is NO WARRANTY, to the extent permitted by law.

Written by David MacKenzie.'


for arg
do
  case $arg in
    --help | --hel | --he | --h)
      exec echo "$usage" ;;
    --version | --versio | --versi | --vers | --ver | --ve | --v)
      exec echo "$version" ;;
    --)
      shift
      break ;;
    -*)
      echo "$0: invalid option: $arg" >&2
      exit 1 ;;
  esac
done

# With fewer than two arguments, simply exec "id".
case $# in
  0|1) exec id -Gn -- "$@" ;;
esac

# With more, we need a loop, and be sure to exit nonzero upon failure.
status=0
write_error=0

for name
do
  if groups=`id -Gn -- "$name"`; then
    echo "$name : $groups" || {
      status=$?
      if test $write_error = 0; then
	echo "$0: write error" >&2
	write_error=1
      fi
    }
  else
    status=$?
  fi
done

exit $status
