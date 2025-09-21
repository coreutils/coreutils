#!/bin/sh
# Exercise ls --block-size and related options.

# Copyright (C) 2011-2025 Free Software Foundation, Inc.

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

TZ=UTC0
export TZ

mkdir sub
cd sub

for size in 1024 4096 262144; do
  echo foo | dd conv=sync bs=$size >file$size || framework_failure_
done
touch -d '2001-01-01 00:00' file* || framework_failure_

size_etc='s/[^ ]* *[^ ]* *//'

ls -og * | sed "$size_etc" >../out || fail=1
POSIXLY_CORRECT=1 ls -og * | sed "$size_etc" >>../out || fail=1
POSIXLY_CORRECT=1 ls -k -og * | sed "$size_etc" >>../out || fail=1

for var in BLOCKSIZE BLOCK_SIZE LS_BLOCK_SIZE; do
  for blocksize in 1 512 1K 1KiB; do
    (eval $var=$blocksize && export $var &&
     echo "x x # $var=$blocksize" &&
     ls -og * &&
     ls -og -k * &&
     ls -og -k --block-size=$blocksize *
    ) | sed "$size_etc" >>../out || fail=1
  done
done

cd ..

cat >exp <<'EOF'
1024 Jan  1  2001 file1024
262144 Jan  1  2001 file262144
4096 Jan  1  2001 file4096
1024 Jan  1  2001 file1024
262144 Jan  1  2001 file262144
4096 Jan  1  2001 file4096
1024 Jan  1  2001 file1024
262144 Jan  1  2001 file262144
4096 Jan  1  2001 file4096
# BLOCKSIZE=1
1024 Jan  1  2001 file1024
262144 Jan  1  2001 file262144
4096 Jan  1  2001 file4096
1024 Jan  1  2001 file1024
262144 Jan  1  2001 file262144
4096 Jan  1  2001 file4096
1024 Jan  1  2001 file1024
262144 Jan  1  2001 file262144
4096 Jan  1  2001 file4096
# BLOCKSIZE=512
1024 Jan  1  2001 file1024
262144 Jan  1  2001 file262144
4096 Jan  1  2001 file4096
1024 Jan  1  2001 file1024
262144 Jan  1  2001 file262144
4096 Jan  1  2001 file4096
2 Jan  1  2001 file1024
512 Jan  1  2001 file262144
8 Jan  1  2001 file4096
# BLOCKSIZE=1K
1024 Jan  1  2001 file1024
262144 Jan  1  2001 file262144
4096 Jan  1  2001 file4096
1024 Jan  1  2001 file1024
262144 Jan  1  2001 file262144
4096 Jan  1  2001 file4096
1 Jan  1  2001 file1024
256 Jan  1  2001 file262144
4 Jan  1  2001 file4096
# BLOCKSIZE=1KiB
1024 Jan  1  2001 file1024
262144 Jan  1  2001 file262144
4096 Jan  1  2001 file4096
1024 Jan  1  2001 file1024
262144 Jan  1  2001 file262144
4096 Jan  1  2001 file4096
1 Jan  1  2001 file1024
256 Jan  1  2001 file262144
4 Jan  1  2001 file4096
# BLOCK_SIZE=1
1024 Jan  1  2001 file1024
262144 Jan  1  2001 file262144
4096 Jan  1  2001 file4096
1024 Jan  1  2001 file1024
262144 Jan  1  2001 file262144
4096 Jan  1  2001 file4096
1024 Jan  1  2001 file1024
262144 Jan  1  2001 file262144
4096 Jan  1  2001 file4096
# BLOCK_SIZE=512
2 Jan  1  2001 file1024
512 Jan  1  2001 file262144
8 Jan  1  2001 file4096
2 Jan  1  2001 file1024
512 Jan  1  2001 file262144
8 Jan  1  2001 file4096
2 Jan  1  2001 file1024
512 Jan  1  2001 file262144
8 Jan  1  2001 file4096
# BLOCK_SIZE=1K
1 Jan  1  2001 file1024
256 Jan  1  2001 file262144
4 Jan  1  2001 file4096
1 Jan  1  2001 file1024
256 Jan  1  2001 file262144
4 Jan  1  2001 file4096
1 Jan  1  2001 file1024
256 Jan  1  2001 file262144
4 Jan  1  2001 file4096
# BLOCK_SIZE=1KiB
1 Jan  1  2001 file1024
256 Jan  1  2001 file262144
4 Jan  1  2001 file4096
1 Jan  1  2001 file1024
256 Jan  1  2001 file262144
4 Jan  1  2001 file4096
1 Jan  1  2001 file1024
256 Jan  1  2001 file262144
4 Jan  1  2001 file4096
# LS_BLOCK_SIZE=1
1024 Jan  1  2001 file1024
262144 Jan  1  2001 file262144
4096 Jan  1  2001 file4096
1024 Jan  1  2001 file1024
262144 Jan  1  2001 file262144
4096 Jan  1  2001 file4096
1024 Jan  1  2001 file1024
262144 Jan  1  2001 file262144
4096 Jan  1  2001 file4096
# LS_BLOCK_SIZE=512
2 Jan  1  2001 file1024
512 Jan  1  2001 file262144
8 Jan  1  2001 file4096
2 Jan  1  2001 file1024
512 Jan  1  2001 file262144
8 Jan  1  2001 file4096
2 Jan  1  2001 file1024
512 Jan  1  2001 file262144
8 Jan  1  2001 file4096
# LS_BLOCK_SIZE=1K
1 Jan  1  2001 file1024
256 Jan  1  2001 file262144
4 Jan  1  2001 file4096
1 Jan  1  2001 file1024
256 Jan  1  2001 file262144
4 Jan  1  2001 file4096
1 Jan  1  2001 file1024
256 Jan  1  2001 file262144
4 Jan  1  2001 file4096
# LS_BLOCK_SIZE=1KiB
1 Jan  1  2001 file1024
256 Jan  1  2001 file262144
4 Jan  1  2001 file4096
1 Jan  1  2001 file1024
256 Jan  1  2001 file262144
4 Jan  1  2001 file4096
1 Jan  1  2001 file1024
256 Jan  1  2001 file262144
4 Jan  1  2001 file4096
EOF

compare exp out || fail=1

# Ensure --size alignment with multi-byte grouping chars
# which wasn't the case before coreutils v9.8
cd sub
for sizem in 1 10; do
  echo foo |
    dd conv=sync bs=$((sizem*1024*1024)) >file${sizem}M || framework_failure_
done

# If any of these unavailable, the C fallback should also be aligned
for loc in sv_SE.UTF-8 $LOCALE_FR_UTF8; do

  # Ensure we have a printable thousands separator
  # This is not the case on FreeBSD 11/12 at least with NBSP
  test $(LC_ALL=$loc ls -s1 --block-size=\'k file1M |
         cut -dK -f1 | LC_ALL=$loc wc -L) = 5 || continue

  test $(LC_ALL=$loc ls -s1 --block-size=\'k |
         tail -n+2 | cut -dK -f1 |
         while IFS= read size; do
           printf '%s\n' "$size" | LC_ALL=$loc wc -L
         done | uniq | wc -l) = 1 || fail=1
done

Exit $fail
