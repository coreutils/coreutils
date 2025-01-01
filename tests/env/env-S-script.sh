#!/bin/sh
# Test env -S in a #! line of a script.

# Copyright (C) 2018-2025 Free Software Foundation, Inc.

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
print_ver_ env
print_ver_ printf

require_perl_

# a shortcut to avoid long lines
dir="$abs_top_builddir/src"

cat <<EOF > shebang || framework_failure_
#!$SHELL
EOF
cat <<\EOF >> shebang || framework_failure_
# Execute a script as per 3 argument shebang
# but without length limits (127 on Linux for example).
script="$1"; shift
shebang=$(sed -n 's/^#!//p;q' < "$script")
interp=$(printf '%s' "$shebang" | cut -d' ' -f1)
rest=$(printf '%s' "$shebang" | cut -s -d' ' -f2-)
test "$rest" && exec "$interp" "$rest" "$script" "$@"
exec "$interp" "$script" "$@"
EOF
chmod a+x shebang || framework_failure_

# A simple shebang program to call our new "env"
printf "#!$dir/env sh\necho hello\n" > env_test || framework_failure_
chmod a+x env_test || framework_failure_

# Verify we can run the shebang which is not the case if
# there are spaces in $abs_top_builddir.
./shebang ./env_test || skip_ "Error running env_test script"

# env should execute 'printf' with 7 parameters:
# 'x%sx\n', 'A', 'B' from the "-S" argument,
# the name of the executed script, and its 3 parameters (C,D,'E F').
# Ignoring the absolute paths, the script is:
#     #!env -S printf x%sx\n A B
printf "#!$dir/env -S $dir/printf "'x%%sx\\n A B\n' > env1 || framework_failure_
chmod a+x env1 || framework_failure_
cat<<\EOF>exp1 || framework_failure_
xAx
xBx
x./env1x
xCx
xDx
xE Fx
EOF
./shebang ./env1 C D "E F" > out1 || fail=1
compare exp1 out1 || fail=1


# similar to the above test but with quotes, the first parameter should be
# 'A B' and not two parameters 'A','B'.
# Ignoring the absolute paths, the script is:
#     #!env -S printf x%sx\n "A B"
printf "#!$dir/env -S $dir/printf "'x%%sx\\n "A B"\n' > env2 ||
  framework_failure_
chmod a+x env2 || framework_failure_
cat<<\EOF>exp2 || framework_failure_
xA Bx
x./env2x
EOF
./shebang ./env2 > out2 || fail=1
compare exp2 out2 || fail=1


# backslash-underscore instead of spaces.
# Ignoring the absolute paths, the script is:
#     #!env -Sprintf\_x%sx\n\_Y
printf "#!$dir/env -S$dir/printf"'\\_x%%sx\\n\\_Y\n' > env3 ||
  framework_failure_
chmod a+x env3 || framework_failure_
cat<<\EOF>exp3 || framework_failure_
xYx
x./env3x
xWx
EOF
./shebang ./env3 W > out3 || fail=1
compare exp3 out3 || fail=1



# Test comments - The "#C D" should be ignored.
# Ignoring the absolute paths, the script is:
#     #!env -Sprintf x%sx\n A#B #C D
printf "#!$dir/env -S$dir/printf"' x%%sx\\n A#B #C D\n' > env4 \
    || framework_failure_
chmod a+x env4 || framework_failure_
cat<<\EOF>exp4 || framework_failure_
xA#Bx
x./env4x
xZx
EOF
./shebang ./env4 Z > out4 || fail=1
compare exp4 out4 || fail=1


# Test with a simple Perl usage.
# (assume Perl is in $PATH, as it is required for the test suite).
# Ignoring the absolute paths, the script is:
#     #!env -S perl -w -T
#     print "hello\n";
{ printf "#!$dir/env -S $PERL -w -T\n" ;
  printf 'print "hello\\n";\n' ; } > env5 || framework_failure_
chmod a+x env5 || framework_failure_
cat<<\EOF>exp5 || framework_failure_
hello
EOF
./shebang ./env5 > out5 || fail=1
compare exp5 out5 || fail=1


# Test with a more complex Perl usage.
# Ignoring the absolute paths, the script is:
#     #!env -S perl -mFile::Basename=basename -e "print basename(\$ARGV[0]);"
# The backslash before the '$' is required to prevent env(1) from treating
# $ARGV as an (invalid syntax) envvar, and pass it as-is to Perl.
{ printf "#!$dir/env -S " ;
  printf "$PERL -mFile::Basename=basename -e " ;
  printf '"print basename(\\$ARGV[0]);"\n' ; } > env6 || framework_failure_
chmod a+x env6 || framework_failure_
# Note: the perl script does not output a newline.
printf "env6" > exp6 || framework_failure_
./shebang ./env6 > out6 || fail=1
compare exp6 out6 || fail=1


Exit $fail
