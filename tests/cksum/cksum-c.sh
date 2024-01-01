#!/bin/sh
# Validate cksum --check dynamic operation

# Copyright (C) 2021-2024 Free Software Foundation, Inc.

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

for args in '-a sha384' '-a blake2b' '-a blake2b -l 384' '-a sm3'; do
  cksum $args 'input' >> CHECKSUMS || fail=1
done
cksum --strict --check CHECKSUMS || fail=1

# Ensure leading whitespace and \ ignored
sed 's/^/ \\/' CHECKSUMS | cksum --strict -c || fail=1

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

Exit $fail
