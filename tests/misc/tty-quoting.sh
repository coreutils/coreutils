#!/bin/sh
# Test quoting output when standard output is a terminal.

# Copyright (C) 2026 Free Software Foundation, Inc.

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
print_ver_ basename dirname du ls readlink realpath printf test
require_strace_ ioctl

touch 'b ar' || framework_failure_
ln -s 'b ar' 'f oo' || framework_failure_

# Induce terminal probes to return true.
# Also ensure a single ioctl(TCGETS) is intercepted.
run_tty_ ()
{
  strace --quiet=all -o run_tty.strace -e trace=ioctl \
    -e inject=ioctl:retval=0 "$@" &&

  # Return status of these also so commands' tty detection
  # is correlated with that of the initial 'test -t 1' probe
  test "$(wc -l < run_tty.strace)" = 1 &&
  grep 'ioctl.*TCGETS' run_tty.strace >/dev/null
}

run_tty_ env test -t 1 ||
 skip_ 'strace cannot make standard output appear to be a terminal'

# We're redirecting to file so this doesn't happen on glibc
# but be defensive on systems that might probe in this case.
run_tty_ env printf foo >printf.t &&
 skip_ 'libc buffering induced a tty probe'

for cmd in basename du dirname 'ls -w0' readlink 'realpath --relative-to=.'; do
  test "$cmd" = 'du' && field=2 || field=1
  test "$cmd" = 'dirname' && file='f oo/.' || file='f oo'

  run_tty_ $cmd "$file" >quoted.t || fail=1
  cut -f$field- quoted.t >quoted || framework_failure_

  # Note ls theoretically doesn't need isatty() for a specified QUOTING_STYLE
  # but it does need it to determine appropriate output format.
  QUOTING_STYLE=literal run_tty_ $cmd "$file" >unquoted.t || fail=1
  cut -f$field- unquoted.t >unquoted || framework_failure_

  env printf '%q\n' "$(cat unquoted)" >printf_quoted || framework_failure_

  compare quoted printf_quoted || fail=1
done

Exit $fail
