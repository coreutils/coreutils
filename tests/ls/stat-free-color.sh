#!/bin/sh
# Show that --color need not use stat, as long as we have d_type support.

# Copyright (C) 2011-2013 Free Software Foundation, Inc.

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

. "${srcdir=.}/tests/init.sh"; path_prepend_ ./src
print_ver_ ls

# Note this list of _file name_ stat functions must be
# as cross platform as possible and so doesn't include
# fstatat64 as that's not available on aarch64 for example.
stats='stat,lstat,stat64,lstat64,newfstatat'

require_strace_ $stats
require_dirent_d_type_

ln -s nowhere dangle || framework_failure_

# Disable enough features via LS_COLORS so that ls --color
# can do its job without calling stat (other than the obligatory
# one-call-per-command-line argument).
cat <<EOF > color-without-stat || framework_failure_
RESET 0
DIR 01;34
LINK 01;36
FIFO 40;33
SOCK 01;35
DOOR 01;35
BLK 40;33;01
CHR 40;33;01
ORPHAN 00
SETUID 00
SETGID 00
CAPABILITY 00
STICKY_OTHER_WRITABLE 00
OTHER_WRITABLE 00
STICKY 00
EXEC 00
MULTIHARDLINK 00
EOF
eval $(dircolors -b color-without-stat)

# The system may perform additional stat-like calls before main.
# To avoid counting those, first get a baseline count by running
# ls with only the --help option.  Then, compare that with the
# invocation under test.
strace -o log-help -e $stats ls --help >/dev/null || fail=1
n_lines_help=$(wc -l < log-help)

strace -o log -e $stats ls --color=always . || fail=1
n_lines=$(wc -l < log)

n_stat=$(expr $n_lines - $n_lines_help)

# Expect one stat call.
case $n_stat in
  0) skip_ 'No stat calls recognized on this platform' ;;
  1) ;; # Corresponding to stat(".")
  *) fail=1; head -n30 log* ;;
esac

Exit $fail
