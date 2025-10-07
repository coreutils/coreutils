#!/bin/sh
# Validate cksum --check dynamic operation

# Copyright (C) 2021-2025 Free Software Foundation, Inc.

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
print_ver_ cksum shuf

shuf -i 1-10 > input || framework_failure_

for args in '-a sha2 -l 384' '-a blake2b' '-a blake2b -l 384' '-a sm3'; do
  cksum $args 'input' >> CHECKSUMS || fail=1
done
cksum --strict --check CHECKSUMS || fail=1

# We don't output but do support SHA2-### tagged format.
# Also ensure we check both formats with and without -a specified.
cksum -a sha2 -l 384 input > sha384-tag.sum || framework_failure_
sed 's/^SHA/SHA2-/' sha384-tag.sum > sha2-tag.sum || framework_failure_
for file in sha384-tag.sum sha2-tag.sum; do
  for spec in '' '-a sha2'; do
    cksum --check $spec $file || fail=1
  done
done

# Ensure invalid length is handled appropriately
# coreutils-9.8 had undefined behavior with the following:
printf '%s\n' 'SHA2-128 (/dev/null) = 38b060a751ac96384cd9327eb1b1e36a' \
  > sha2-bad-length.sum || framework_failure_
returns_ 1 cksum --check sha2-bad-length.sum 2>err || fail=1
echo 'cksum: sha2-bad-length.sum: no properly formatted checksum lines found' \
  > experr || framework_failure_
compare experr err || fail=1

# Ensure base64 in untagged format that matches tags is supported
# From coreutils 9.2 - 9.8 inclusive this was not supported
echo 'SHA1+++++++++++++++++++++++=  /dev/null' > tag-prefix.sum \
  || framework_failure_
returns_ 1 cksum --check -a sha1 tag-prefix.sum 2>err || fail=1
echo 'cksum: WARNING: 1 computed checksum did NOT match' \
  > experr || framework_failure_
compare experr err || fail=1

# Ensure leading whitespace and \ ignored
sed 's/^/ \\/' CHECKSUMS | cksum --strict -c || fail=1

# Ensure file names with " (=" supported.
awkward_file='abc (f) = abc'
touch "$awkward_file" || framework_failure_
cksum -a sha1 "$awkward_file" > tag-awkward.sum || framework_failure_
cksum --check tag-awkward.sum || fail=1

# Check common signed checksums format works in non strict mode
cat >> signed_CHECKSUMS <<\EOF
-----BEGIN PGP SIGNED MESSAGE-----
Hash: SHA384

# ignored comment
EOF
cat CHECKSUMS >> signed_CHECKSUMS
cat >> signed_CHECKSUMS <<\EOF
-----BEGIN PGP SIGNATURE-----

# Note base64 doesn't have ambiguous delimiters in its charset
SHA384+BCAAdFiEEjFummQvbJuGfKhqAEWGuaUVxmjkFAmCDId0ACgkQEWGuaUVx
BLAKE2b/00001EuON62pTEnqrJ5lav61QxRByiuDp/6VODrRL2JWM6Stxu1Myws=
=AWU7
-----END PGP SIGNATURE-----
EOF
cksum --check signed_CHECKSUMS || fail=1

# Can check individual digests in a mixed file
cksum --check -a sm3 CHECKSUMS || fail=1

# Checks against older (non hex) checksum formats not supported
returns_ 1 cksum -a crc -c CHECKSUMS || fail=1
cksum -a crc 'input' > CHECKSUMS.crc || fail=1
returns_ 1 cksum -c CHECKSUMS.crc || fail=1

# Test --status
cksum --status --check CHECKSUMS >out 2>&1 || fail=1
# Should be empty
compare /dev/null out || fail=1

# Add a comment. No errors
echo '# Very important comment' >> CHECKSUMS
cksum --status --check CHECKSUMS >out 2>&1 || fail=1

# Check for the error mgmt
echo 'invalid line' >> CHECKSUMS
# Exit code is 0 in this case
cksum --check CHECKSUMS >out 2>&1 || fail=1
grep '1 line is improperly formatted' out || fail=1
# But not with --strict
cksum --strict --check CHECKSUMS >out 2>&1 && fail=1
grep '1 line is improperly formatted' out || fail=1
echo "invalid line" >> CHECKSUMS
# plurality checks
cksum --strict --check CHECKSUMS >out 2>&1 && fail=1
grep '2 lines are improperly formatted' out || fail=1

# Inject an incorrect checksum
invalid_sum='aaaaaaaaaaaaaaaaaaaaaaaaaaaaafdb57c725157cb40b5aee8d937b8351477e'
echo "SM3 (input) = $invalid_sum" >> CHECKSUMS
cksum --check CHECKSUMS >out 2>&1 && fail=1
# with --strict (won't change the result)
grep '1 computed checksum did NOT match' out || fail=1
grep 'input: FAILED' out || fail=1
cksum --check --strict CHECKSUMS >out 2>&1 && fail=1

# With a non existing file
echo "SM3 (input2) = $invalid_sum" >> CHECKSUMS2
cksum --check CHECKSUMS2 >out 2>&1 && fail=1
grep 'input2: FAILED open or read' out || fail=1
grep '1 listed file could not be read' out || fail=1
# with --strict (won't change the result)
cksum --check --strict CHECKSUMS2 >out 2>&1 && fail=1

# With errors
cksum --status --check CHECKSUMS >out 2>&1 && fail=1
compare /dev/null out || fail=1

# Test --warn
echo "BLAKE2b (missing-file) = $invalid_sum" >> CHECKSUMS
cksum --warn --check CHECKSUMS > out 2>&1
# check that the incorrect lines are correctly reported with --warn
grep 'CHECKSUMS: 6: improperly formatted SM3 checksum line' out || fail=1
grep 'CHECKSUMS: 9: improperly formatted BLAKE2b checksum line' out || fail=1

# Test --ignore-missing

echo "SM3 (nonexistent) = $invalid_sum" >> CHECKSUMS-missing
# we have output on both stdout and stderr
cksum --check CHECKSUMS-missing > stdout 2> stderr && fail=1
grep 'nonexistent: FAILED open or read' stdout || fail=1
grep 'nonexistent: No such file or directory' stderr || fail=1
grep '1 listed file could not be read' stderr || fail=1

cksum --ignore-missing --check CHECKSUMS-missing  > stdout 2> stderr && fail=1
# We should not get these errors
grep 'nonexistent: No such file or directory' stdout && fail=1
grep 'nonexistent: FAILED open or read' stdout && fail=1
grep 'CHECKSUMS-missing: no file was verified' stderr || fail=1

# Combination of --status and --warn
cksum --status --warn --check CHECKSUMS > out 2>&1 && fail=1

grep 'CHECKSUMS: 9: improperly formatted BLAKE2b checksum line' out || fail=1
grep 'WARNING: 3 lines are improperly formatted' out || fail=1
grep 'WARNING: 1 computed checksum did NOT match' out || fail=1

# The order matters. --status will hide the results
cksum --warn --status --check CHECKSUMS > out 2>&1 && fail=1
grep 'CHECKSUMS: 8: improperly formatted BLAKE2b checksum line' out && fail=1
grep 'WARNING: 3 lines are improperly formatted' out && fail=1
grep 'WARNING: 1 computed checksum did NOT match' out && fail=1

# Combination of --status and --ignore-missing
cksum --status --ignore-missing --check CHECKSUMS > out 2>&1 && fail=1
# should be empty
compare /dev/null out || fail=1

# Combination of all three
cksum --status --warn --ignore-missing --check \
      CHECKSUMS-missing > out 2>&1 && fail=1
# Not empty
test -s out || fail=1
grep 'CHECKSUMS-missing: no file was verified' out || fail=1
grep 'nonexistent: No such file or directory' stdout && fail=1

# Check with several files
# When the files don't exist
cksum --check non-existing-1 non-existing-2 > out 2>&1 && fail=1
grep 'non-existing-1: No such file or directory' out || fail=1
grep 'non-existing-2: No such file or directory' out || fail=1

# When the files are empty
touch empty-1 empty-2 || framework_failure_
cksum --check empty-1 empty-2 > out 2>&1 && fail=1
grep 'empty-1: no properly formatted checksum lines found' out || fail=1
grep 'empty-2: no properly formatted checksum lines found' out || fail=1
Exit $fail
