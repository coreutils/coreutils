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
  exit 1
fi

fail=0
case $# in
  1 )
    case "z${1}" in
      z--help )
	 echo "$usage" || fail=1; echo "$usage_help" || fail=1; exit $fail;;
      z--version )
	 echo "nohup (@GNU_PACKAGE@) @VERSION@" || fail=1; exit $fail;;
      * ) ;;
    esac
    ;;
  * ) ;;
esac

# Make sure we get GNU nice, if possible; also allow
# it to be somewhere else in PATH if not installed yet.
# But do not modify PATH itself.
IFS="${IFS=	 }"; save_ifs="$IFS"; IFS=":"
nicepath="@bindir@:$PATH"
niceprog="nice"
for nicedir in $nicepath; do
  test -z "$nicedir" && nicedir="."
  if test -x "$nicedir/nice"; then
    niceprog="$nicedir/nice"
    break
  fi
done
IFS="$save_ifs"

trap "" 1
oldmask=`umask`; umask 077
# Only redirect the output if the user didn't already do it.
if [ -t 1 ]; then
  # If we cannot write to the current directory, use the home directory.
  if cat /dev/null >> nohup.out; then
    echo "nohup: appending output to \`nohup.out'" 2>&1
    umask $oldmask
    exec "$niceprog" -n 5 -- "$@" >> nohup.out 2>&1
  else
    cat /dev/null >> $HOME/nohup.out
    echo "nohup: appending output to \`$HOME/nohup.out'" 2>&1
    umask $oldmask
    exec "$niceprog" -n 5 -- "$@" >> $HOME/nohup.out 2>&1
  fi
else
  umask $oldmask
  exec "$niceprog" -n 5 -- "$@"
fi
