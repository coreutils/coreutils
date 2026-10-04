#!/bin/sh
# Test that various programs can execute a program name with invalid
# Unicode characters.

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

script="$(bad_unicode)"

# Create a program with invalid Unicode in its name.
cat <<EOF >"$script"
#!$SHELL
echo "\$@"
EOF
test $? = 0 || skip_ 'bad unicode not supported in shell or file system'
chmod u+x "$script" || framework_failure_

# Check that it can be executed.
./"$script" "$script" >exp \
  || skip_ 'cannot execute a file name with invalid unicode'

# List of programs that execute another command.
printf '%s' '\
chroot --skip-chdir /
env
nice
nohup
runcon '"$(id -Z)"'
stdbuf -oL
timeout 10
' |
sort -k 1b,1 > all_executors || framework_failure_

printf '%s\n' $built_programs |
sort -k 1b,1 > built_programs || framework_failure_

join all_executors built_programs > built_executors || framework_failure_

for loc in C "$LOCALE_FR" "$LOCALE_FR_UTF8"; do
  { test -z "$loc" || test "$loc" = none; } && continue
  while read executor; do
    LC_ALL="$loc" $executor ./"$script" "$script" >out 2>err
    if test $? = 0; then
      compare exp out || fail=1
      compare /dev/null err || fail=1
    else
      case "$executor" in
        runcon*)
          grep 'runcon: runcon may be used only on a SELinux kernel' err
          ;;
        chroot*)
          ! uid_is_privileged_
          ;;
        *)
          false
          ;;
      esac || { cat err; fail=1; }
    fi
  done < built_executors
done

# Test 'sort --compress-program'.
cat <<EOF >"$script" || framework_failure_
#!$SHELL
tr 41 14
touch ok
EOF
seq -w 2000 >exp || framework_failure_
tac exp >in || framework_failure_
for loc in C "$LOCALE_FR" "$LOCALE_FR_UTF8"; do
  { test -z "$loc" || test "$loc" = none; } && continue
  LC_ALL="$loc" sort -S 1k --compress-program=./"$script" in >out 2>err \
    || fail=1
  compare exp out || fail=1
  compare /dev/null err || fail=1
  test -f ok || fail=1
done

# Test 'install --strip --strip-program'.
cat <<EOF >"$script" || framework_failure_
#!$SHELL
sed s/b/B/ \$1 > \$1.t && mv \$1.t \$1
EOF
for loc in C "$LOCALE_FR" "$LOCALE_FR_UTF8"; do
  { test -z "$loc" || test "$loc" = none; } && continue
  echo abc > src || framework_failure_
  echo aBc > exp || framework_failure_
  LC_ALL="$loc" ginstall src dest -s --strip-program=./"$script" >out 2>err \
    || fail=1
  compare /dev/null out || fail=1
  compare /dev/null err || fail=1
  compare exp dest || fail=1
  rm -f dest || framework_failure_
done

# Test 'split --filter'.
cat <<\EOF >in || framework_failure_
A
B
C
EOF
for f in a b c; do
  echo $f > exp-$f || framework_failure_
done
cat <<EOF >"$script" || framework_failure_
#!$SHELL
tr '[A-Z]' '[a-z]'
EOF
for loc in C "$LOCALE_FR" "$LOCALE_FR_UTF8"; do
  { test -z "$loc" || test "$loc" = none; } && continue
  LC_ALL="$loc" split -l1 --filter=./"'$script'"' > $FILE.filtered' in out- \
    >out 2>err || fail=1
  compare /dev/null out || fail=1
  compare /dev/null err || fail=1
  for f in a b c; do
    compare exp-$f out-a$f.filtered || fail=1
    rm -f out-a$f.filtered || framework_failure_
  done
done

Exit $fail
