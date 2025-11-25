#!/bin/sh
# Verify that all supported options are mentioned in usage

# Copyright (C) 2025-2026 Free Software Foundation, Inc.

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

getopts() {
  sed -n '/long_*opt.*\[/,/^}/{/^ *{/p}' "$abs_top_srcdir/src/$1.c" |
  grep -Ev "(\"-|Deprecated|Obsolescent|Not in $1)"
}
# Currently only check short opts with corresponding long opt
shortopts() { getopts $1 | cut -s -d"'" -f2; }
longopts() { getopts $1 | cut -s -d'"' -f2; }

for prg in $built_programs; do

  sprg=$prg
  test $prg = ginstall && sprg=install

  # Only check cases where the command matches the source name.
  # One could lookup the actual source file for main with nm -l
  # but then we'd have to deal with cases like one source file
  # only enabling some options for certain commands.
  test -f "$abs_top_srcdir/src/$sprg.c" || {
    test "$DEBUG" && echo skipping $sprg
    continue
  }

  test "$DEBUG" && echo processing $sprg
  got_option=false
  for opt in $(shortopts $sprg); do
    got_option=true
    env HELP_NO_MARKUP=1 $prg --help | grep -F -- " -$opt" >/dev/null ||
      { printf -- '%s -%s missing from --help\n' $sprg $opt >&2; fail=1; }
  done
  for opt in $(longopts $sprg); do
    got_option=true
    env HELP_NO_MARKUP=1 $prg --help | grep -F -- "--$opt" >/dev/null ||
      { printf -- '%s --%s missing from --help\n' $sprg $opt >&2; fail=1; }
  done
  test "$DEBUG" && test $got_option = false && echo No options for $prg ?

done

Exit $fail
