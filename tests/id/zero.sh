#!/bin/sh
# Exercise "id --zero".

# Copyright (C) 2013-2025 Free Software Foundation, Inc.

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
print_ver_ id

u="$( id -nu )"
id || fail=1
id "$u" || fail=1

# id(1) should refuse --zero in default format.
echo 'id: option --zero not permitted in default format' > err-exp \
  || framework_failure_
id --zero > out 2>err && fail=1
compare /dev/null out || fail=1
compare err-exp   err || fail=1

# Create a nice list of users.
# Add $USER to ensure we have at least one explicit entry.
users="$u"
# Add a few typical users to test single group and multiple groups.
for u in root man postfix sshd nobody ; do
  id $u >/dev/null 2>&1 && users="$users $u"
done
# Add $users and '' (implicit $USER) to list to process.
printf '%s\n' $users '' >> users || framework_failure_

# Exercise "id -z" with various options.
printf '\n' > exp || framework_failure_
> out || framework_failure_

while read u ; do
  for o in g gr G Gr u ur ; do
    for n in '' n ; do
      printf   '%s: ' "id -${o}${n}[z] $u" >> exp || framework_failure_
      printf '\n%s: ' "id -${o}${n}[z] $u" >> out || framework_failure_
      # There may be no name corresponding to an id, so don't check
      # exit status when in name lookup mode
      id -${o}${n}  $u >> exp ||
        { test $? -ne 1 || test -z "$n" && fail=1; }
      id -${o}${n}z $u  > tmp ||
        { test $? -ne 1 || test -z "$n" && fail=1; }
      head -c-1 < tmp >> out || framework_failure_
    done
  done
done < users
printf '\n' >> out || framework_failure_
tr '\0' ' ' < out > out2 || framework_failure_
compare exp out2 || fail=1

# multiuser testing with -z
# test if the options work, these tests should pass if the above tests
# do.

for o in g gr u ur ; do
  for n in '' n ; do
    id -${o}${n}  $users >> tmp1 ||
      { test $? -ne 1 || test -z "$n" && fail=1; }
    id -${o}${n}z $users  > tmp2 ||
      { test $? -ne 1 || test -z "$n" && fail=1; }
    tr '\0' '\n' < tmp2 >> tmp3
  done
done
compare tmp1 tmp3 || fail=1

# Separate checks when we are testing for multiple users && -G.
# This is done because we terminate the records with two NULs
# instead of a regular single NUL.

NL='
'

for o in G Gr ; do
  for n in '' n ; do
    id -${o}${n}  $users >> gtmp1 ||
      { test $? -ne 1 || test -z "$n" && fail=1; }
    echo >> gtmp1 || framework_failure_

    id -${o}${n}z $users  > gtmp2 ||
      { test $? -ne 1 || test -z "$n" && fail=1; }
    # we replace all NULs with spaces, the result we get is there are two
    # consecutive spaces instead of two NUL's, we pass this to sed
    # to replace more than 1 space with a newline. This is ideally where a new
    # line should be. This should make the output similar to without -z.
    { tr '\0' ' ' < gtmp2; echo; } | sed "s/  /\\$NL/g" >> gtmp3
  done
done
compare gtmp1 gtmp3 || fail=1

Exit $fail
