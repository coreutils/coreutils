#!/bin/sh
# Verify that all supported options have references in docs

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

# Currently only check short opts with corresponding long opt
shortopts() { getopts $1 | cut -s -d"'" -f2; }
longopts() { getopts $1 | cut -s -d'"' -f2; }

getopts() {
  skip='--help|--version'              # These refs treated specially

  env "$1" --help |
  grep -E '^(  -|      --)' |          # find options
  grep -Ev -- " - |-M.*from first" |   # exclude invalid matches
  sed -e 's/^ *//' -e's/  .*//' |      # strip leading space and descriptions
  tr ' ' '\n' |                        # split to one per line
  sed -n 's/^\(--\?[^,=[]*\).*/\1/p' | # Remove parameters
  grep -Ev -- "$skip"                  # These refs treated specially
}

for prg in $built_programs; do

  dprg=$prg
  test $prg = ginstall && dprg=install
  test $prg = '[' && dprg=test
  test $prg = 'dir' && dprg=ls
  test $prg = 'vdir' && dprg=ls
  test $prg = 'md5sum' && dprg=cksum
  test $prg = 'b2sum' && dprg=cksum
  test $prg = 'sha1sum' && dprg=cksum
  test $prg = 'sha224sum' && dprg=cksum
  test $prg = 'sha256sum' && dprg=cksum
  test $prg = 'sha384sum' && dprg=cksum
  test $prg = 'sha512sum' && dprg=cksum

  test "$DEBUG" && echo processing $sprg
  got_option=false
  for opt in $(getopts $prg); do
    got_option=true
    if ! grep -E "opt(Itemx?|Anchor)\\{$dprg,$opt[,}]" \
          "$abs_top_srcdir/doc/coreutils.texi" >/dev/null; then
      if ! grep "optItemx\\?{\\\\cmd\\\\,$opt," \
            "$abs_top_srcdir/doc/coreutils.texi" >/dev/null; then
        printf -- '%s %s reference missing in texi\n' $dprg $opt >&2
        fail=1
      elif test "$DEBUG"; then
        echo "only matched $dprg $opt in a general macro"
      fi
    fi
  done
  test "$DEBUG" && test $got_option = false && echo No options for $prg ?

done

Exit $fail
