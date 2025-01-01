#!/bin/sh
# ensure that tail -f doesn't hang in various cases

# Copyright (C) 2009-2025 Free Software Foundation, Inc.

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
print_ver_ tail test head
trap_sigpipe_or_skip_

# Speedup the non inotify case
fastpoll='-s.1 --max-unchanged-stats=1'

for mode in '' '---disable-inotify'; do
# Ensure :|tail -f doesn't hang, per POSIX
echo oo > exp || framework_failure_
echo foo | timeout 10 tail -f $mode $fastpoll -c3 > out || fail=1
compare exp out || fail=1
cat <<\EOF > exp || framework_failure_
==> standard input <==
ar
EOF
echo bar | returns_ 1 \
 timeout 10 tail -f $mode $fastpoll -c3 - missing > out || fail=1
compare exp out || fail=1

# This would wait indefinitely before v8.28 due to no EPIPE being
# generated due to no data written after the first small amount.
# Also check tail exits if SIGPIPE is being ignored.
# Note 'trap - SIGPIPE' is ineffective if the initiating shell
# has ignored SIGPIPE, but that's not the normal case.
case $host_triplet in
  *aix*) echo  'avoiding due to no way to detect closed outputs on AIX' ;;
  *)
for disposition in '' '-'; do
  (trap "$disposition" PIPE;
   returns_ 124 timeout 10 \
    tail -n2 -f $mode $fastpoll out && touch timed_out) |
  head -n2 > out2
  test -e timed_out && fail=1
  compare exp out2 || fail=1
  rm -f timed_out
done ;;
esac

# This would wait indefinitely before v8.28 (until first write)
# test -w /dev/stdout is used to check that >&- is effective
# which was seen not to be the case on NetBSD 7.1 / x86_64:
if env test -w /dev/stdout >/dev/null &&
   env test ! -w /dev/stdout >&-; then
  (returns_ 1 timeout 10 tail -f $mode $fastpoll /dev/null >&-) || fail=1
fi
done

Exit $fail
