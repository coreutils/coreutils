#!/bin/sh
# nohup -- run a command immume to hangups, with output to a non-tty
# Copyright (C) 1991, 1997, 1999, 2000, 2002 Free Software Foundation, Inc.

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

usage="Usage: $0 COMMAND [ARG]...
  or:  $0 OPTION"

usage_try="
Try \`$0 --help' for more information."

usage_help="Run COMMAND, ignoring hangup signals.

  --help      display this help and exit
  --version   output version information and exit

Report bugs to <@PACKAGE_BUGREPORT@>."

if [ $# -eq 0 ]; then
  echo >&2 "$usage"
  echo >&2 "$usage_try"
  exit 127
fi

fail=0
case $# in
  1 )
    case "z${1}" in
      z--help )
	 echo "$usage" || fail=127; echo "$usage_help" || fail=127
	 exit $fail;;
      z--version )
	 echo "nohup (@GNU_PACKAGE@) @VERSION@" || fail=127
	 exit $fail;;
      * ) ;;
    esac
    ;;
  * ) ;;
esac

trap "" 1

# Redirect stdout if the user didn't already do it.
if [ -t 1 ]; then
  oldmask=`umask`; umask 077
  # If we cannot write to the current directory, use the home directory.
  if exec >> nohup.out; then
    echo "nohup: appending output to \`nohup.out'" >&2
  elif exec >> "$HOME/nohup.out"; then
    echo "nohup: appending output to \`$HOME/nohup.out'" >&2
  else
    echo "nohup: cannot append to either \`nohup.out' or \`$HOME/nohup.out'" \
      >&2
    exit 127
  fi
  umask $oldmask
fi

# Redirect stderr if the user didn't already do it.
if [ -t 2 ]; then
  exec 2>&1
fi

exec "$@"
