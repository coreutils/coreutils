/* Calculate checksums and sizes of files
   Copyright 2025 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

#ifndef __CKSUM_H__
# define __CKSUM_H__

# include <stdint.h>
# include <stdio.h>

extern bool cksum_debug;

extern int
crc_sum_stream (FILE *stream, void *resstream, uintmax_t *length);

extern int
crc32b_sum_stream (FILE *stream, void *resstream, uintmax_t *length);

extern void
output_crc (char const *file, int binary_file, void const *digest, bool raw,
            bool tagged, unsigned char delim, bool args, uintmax_t length)
  _GL_ATTRIBUTE_NONNULL ((3));

extern bool
cksum_vmull (FILE *fp, uint_fast32_t *crc_out, uintmax_t *length_out);

extern bool
cksum_pclmul (FILE *fp, uint_fast32_t *crc_out, uintmax_t *length_out);

extern bool
cksum_avx2 (FILE *fp, uint_fast32_t *crc_out, uintmax_t *length_out);

extern bool
cksum_avx512 (FILE *fp, uint_fast32_t *crc_out, uintmax_t *length_out);

extern uint_fast32_t const crctab[8][256];

#endif
