#! /bin/sh
# Make sure stty can parse most of its options.

# Copyright (C) 1998-2015 Free Software Foundation, Inc.

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

# Make sure there's a tty on stdin.
. "${srcdir=.}/tests/init.sh"; path_prepend_ ./src
print_ver_ stty

require_controlling_input_terminal_
trap '' TTOU # Ignore SIGTTOU

# Get the reversible settings from stty.c.
stty_reversible_init_

saved_state=.saved-state
stty --save > $saved_state || fail=1
stty $(cat $saved_state) || fail=1

# This would segfault prior to sh-utils-2.0j.
stty erase - || fail=1

# These would improperly ignore invalid options through coreutils 5.2.1.
returns_ 1 stty -F 2>/dev/null || fail=1
returns_ 1 stty -raw -F no/such/file 2>/dev/null || fail=1
returns_ 1 stty -raw -a 2>/dev/null || fail=1

# Build a list of all boolean options stty accepts on this system.
# Don't depend on terminal width.  Put each option on its own line,
# remove all non-boolean ones, then remove any leading hyphens.
sed_del='/^speed/d;/^rows/d;/^columns/d;/ = /d'
options=$(stty -a | tr -s ';' '\n' | sed "s/^ //;$sed_del;s/-//g")

# Take them one at a time, with and without the leading '-'.
for opt in $options; do
  # 'stty parenb' and 'stty -parenb' fail with this message
  # stty: standard input: unable to perform all requested operations
  # on Linux 2.2.0-pre4 kernels.  Also since around Linux 2.6.30
  # other serial control settings give the same error. So skip them.
  # Also on ppc*|sparc* glibc platforms 'icanon' gives the same error.
  # See: http://debbugs.gnu.org/7228#14
  case $opt in
    parenb|parodd|cmspar) continue;;
    cstopb|crtscts|cdtrdsr|icanon) continue;;
  esac

  # This is listed as supported on FreeBSD
  # but the ioctl returns ENOTTY.
  test $opt = extproc && continue

  stty $opt || fail=1

  # Likewise, 'stty -cread' would fail, so skip that, too.
  test $opt = cread && continue
  if stty_reversible_query_ "$opt" ; then
    stty -$opt || { fail=1; echo -$opt; }
  fi
done

stty $(cat $saved_state)

Exit $fail
