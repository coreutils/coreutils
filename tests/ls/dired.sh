#!/bin/sh
# make sure --dired option works

# Copyright (C) 2001-2025 Free Software Foundation, Inc.

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
print_ver_ ls


# Check with constant positions
mkdir dir || framework_failure_

cat <<EOF > exp
  dir:
  total 0
//SUBDIRED// 2 5
//DIRED-OPTIONS// --quoting-style=literal
EOF
for opt in '-l' '' '--hyperlink' '-x'; do
  LC_MESSAGES=C ls $opt -R --dired dir > out || fail=1
  compare exp out || fail=1
done


# Check with varying positions (due to usernames etc.)
# Also use multibyte characters to show --dired counts bytes not characters
touch dir/1a dir/2æ || framework_failure_
mkdir -p dir/3dir || framework_failure_
touch dir/aaa || framework_failure_
ln -s target dir/0aaa_link || framework_failure_

ls -l --dired dir > out || fail=1

dired_values=$(grep "//DIRED//" out| cut -d' ' -f2-)
expected_files="0aaa_link 1a 2æ 3dir aaa"

dired_count=$(printf '%s\n' $dired_values | wc -l)
expected_count=$(printf '%s\n' $expected_files | wc -l)

if test "$expected_count" -ne $(($dired_count / 2)); then
  echo "Mismatch in number of files!" \
       "Expected: $expected_count, Found: $(($dired_count / 2))"
  fail=1
fi

# Split the values into pairs and extract the filenames
index=1
set -- $dired_values
while test "$#" -gt 0; do
  extracted_filename=$(head -c "$2" out | tail -c+"$(($1 + 1))")
  expected_file=$(echo $expected_files | cut -d' ' -f$index)

  # For symlinks, compare only the symlink name, ignoring the target part.
  if echo "$extracted_filename" | grep ' -> ' > /dev/null; then
    extracted_filename=$(echo "$extracted_filename" | cut -d' ' -f1)
  fi

  if test "$extracted_filename" != "$expected_file"; then
    echo "Mismatch! Expected: $expected_file, Found: $extracted_filename"
    fail=1
  fi
  shift; shift
  index=$(($index + 1))
done


Exit $fail
