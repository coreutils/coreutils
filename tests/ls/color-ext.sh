#!/bin/sh
# Ensure "ls --color" properly colors file name extensions.

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
print_ver_ ls
working_umask_or_skip_

touch img1.jpg IMG2.JPG img3.JpG file1.z file2.Z || framework_failure_
code_jpg='01;35'
code_JPG='01;35;46'
code_z='01;31'
c0=$(printf '\033[0m')
c_jpg=$(printf '\033[%sm' $code_jpg)
c_JPG=$(printf '\033[%sm' $code_JPG)
c_z=$(printf '\033[%sm' $code_z)

# Case insensitive extensions
LS_COLORS="*.jpg=$code_jpg:*.Z=$code_z" ls -U1 --color=always \
  img1.jpg IMG2.JPG file1.z file2.Z > out || fail=1
printf "$c0\
${c_jpg}img1.jpg$c0
${c_jpg}IMG2.JPG$c0
${c_z}file1.z$c0
${c_z}file2.Z$c0
" > out_ok || framework_failure_
compare out out_ok || fail=1

# Case sensitive extensions
LS_COLORS="*.jpg=$code_jpg:*.JPG=$code_JPG" ls -U1 --color=always \
  img1.jpg IMG2.JPG img3.JpG > out || fail=1
printf "$c0\
${c_jpg}img1.jpg$c0
${c_JPG}IMG2.JPG$c0
img3.JpG
" > out_ok || framework_failure_
compare out out_ok || fail=1

# Case insensitive extensions (due to same sequences)
LS_COLORS="*.jpg=$code_jpg:*.JPG=$code_jpg" ls -U1 --color=always \
  img1.jpg IMG2.JPG img3.JpG > out || fail=1
printf "$c0\
${c_jpg}img1.jpg$c0
${c_jpg}IMG2.JPG$c0
${c_jpg}img3.JpG$c0
" > out_ok || framework_failure_
compare out out_ok || fail=1

# Case insensitive extensions (due to same sequences (after ignored sequences))
# Note later entries in LS_COLORS take precedence.
LS_COLORS="*.jpg=$code_jpg:*.jpg=$code_JPG:*.JPG=$code_JPG" \
  ls -U1 --color=always img1.jpg IMG2.JPG img3.JpG > out || fail=1
printf "$c0\
${c_JPG}img1.jpg$c0
${c_JPG}IMG2.JPG$c0
${c_JPG}img3.JpG$c0
" > out_ok || framework_failure_
compare out out_ok || fail=1

# Case sensitive extensions (due to diff sequences (after ignored sequences))
# Note later entries in LS_COLORS take precedence.
LS_COLORS="*.jpg=$code_JPG:*.jpg=$code_jpg:*.JPG=$code_JPG" \
  ls -U1 --color=always img1.jpg IMG2.JPG img3.JpG > out || fail=1
printf "$c0\
${c_jpg}img1.jpg$c0
${c_JPG}IMG2.JPG$c0
img3.JpG
" > out_ok || framework_failure_
compare out out_ok || fail=1

Exit $fail
