/* cksum -- calculate and print POSIX checksums and sizes of files
   Copyright (C) 2021-2025 Free Software Foundation, Inc.

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

#include <config.h>

#include "cksum.h"

#include <stdio.h>
#include <sys/types.h>
#include <x86intrin.h>
#include "system.h"

/* Number of bytes to read at once.  */
#define BUFLEN (1 << 16)

/* Calculate CRC32 using PCLMULQDQ CPU instruction found in x86/x64 CPUs */

bool
cksum_pclmul (FILE *fp, uint_fast32_t *crc_out, uintmax_t *length_out)
{
  __m128i buf[BUFLEN / sizeof (__m128i)];
  uint_fast32_t crc = 0;
  uintmax_t length = 0;
  size_t bytes_read;
  __m128i single_mult_constant;
  __m128i four_mult_constant;
  __m128i shuffle_constant;

  if (!fp || !crc_out || !length_out)
    return false;

  /* These constants and general algorithms are taken from the Intel whitepaper
     "Fast CRC Computation for Generic Polynomials Using PCLMULQDQ Instruction"
  */
  single_mult_constant = _mm_set_epi64x (0xC5B9CD4C, 0xE8A45605);
  four_mult_constant = _mm_set_epi64x (0x8833794C, 0xE6228B11);

  /* Constant to byteswap a full SSE register */
  shuffle_constant = _mm_set_epi8 (0, 1, 2, 3, 4, 5, 6, 7, 8,
                                   9, 10, 11, 12, 13, 14, 15);

  while ((bytes_read = fread (buf, 1, BUFLEN, fp)) > 0)
    {
      __m128i *datap;
      __m128i data;
      __m128i data2;
      __m128i data3;
      __m128i data4;
      __m128i data5;
      __m128i data6;
      __m128i data7;
      __m128i data8;
      __m128i fold_data;
      __m128i xor_crc;

      if (ckd_add (&length, length, bytes_read))
        {
          errno = EOVERFLOW;
          return false;
        }

      datap = (__m128i *)buf;

      /* Fold in parallel eight 16-byte blocks into four 16-byte blocks */
      if (bytes_read >= 16 * 8)
        {
          data = _mm_loadu_si128 (datap);
          data = _mm_shuffle_epi8 (data, shuffle_constant);
          /* XOR in initial CRC value (for us 0 so no effect), or CRC value
             calculated for previous BUFLEN buffer from fread */
          xor_crc = _mm_set_epi32 (crc, 0, 0, 0);
          crc = 0;
          data = _mm_xor_si128 (data, xor_crc);
          data3 = _mm_loadu_si128 (datap + 1);
          data3 = _mm_shuffle_epi8 (data3, shuffle_constant);
          data5 = _mm_loadu_si128 (datap + 2);
          data5 = _mm_shuffle_epi8 (data5, shuffle_constant);
          data7 = _mm_loadu_si128 (datap + 3);
          data7 = _mm_shuffle_epi8 (data7, shuffle_constant);


          while (bytes_read >= 16 * 8)
            {
              datap += 4;

              /* Do multiplication here for four consecutive 16 byte blocks */
              data2 = _mm_clmulepi64_si128 (data, four_mult_constant, 0x00);
              data = _mm_clmulepi64_si128 (data, four_mult_constant, 0x11);
              data4 = _mm_clmulepi64_si128 (data3, four_mult_constant, 0x00);
              data3 = _mm_clmulepi64_si128 (data3, four_mult_constant, 0x11);
              data6 = _mm_clmulepi64_si128 (data5, four_mult_constant, 0x00);
              data5 = _mm_clmulepi64_si128 (data5, four_mult_constant, 0x11);
              data8 = _mm_clmulepi64_si128 (data7, four_mult_constant, 0x00);
              data7 = _mm_clmulepi64_si128 (data7, four_mult_constant, 0x11);

              /* Now multiplication results for the four blocks is xor:ed with
                 next four 16 byte blocks from the buffer. This effectively
                 "consumes" the first four blocks from the buffer.
                 Keep xor result in variables for multiplication in next
                 round of loop. */
              data = _mm_xor_si128 (data, data2);
              data2 = _mm_loadu_si128 (datap);
              data2 = _mm_shuffle_epi8 (data2, shuffle_constant);
              data = _mm_xor_si128 (data, data2);

              data3 = _mm_xor_si128 (data3, data4);
              data4 = _mm_loadu_si128 (datap + 1);
              data4 = _mm_shuffle_epi8 (data4, shuffle_constant);
              data3 = _mm_xor_si128 (data3, data4);

              data5 = _mm_xor_si128 (data5, data6);
              data6 = _mm_loadu_si128 (datap + 2);
              data6 = _mm_shuffle_epi8 (data6, shuffle_constant);
              data5 = _mm_xor_si128 (data5, data6);

              data7 = _mm_xor_si128 (data7, data8);
              data8 = _mm_loadu_si128 (datap + 3);
              data8 = _mm_shuffle_epi8 (data8, shuffle_constant);
              data7 = _mm_xor_si128 (data7, data8);

              bytes_read -= (16 * 4);
            }
          /* At end of loop we write out results from variables back into
             the buffer, for use in single fold loop */
          data = _mm_shuffle_epi8 (data, shuffle_constant);
          _mm_storeu_si128 (datap, data);
          data3 = _mm_shuffle_epi8 (data3, shuffle_constant);
          _mm_storeu_si128 (datap + 1, data3);
          data5 = _mm_shuffle_epi8 (data5, shuffle_constant);
          _mm_storeu_si128 (datap + 2, data5);
          data7 = _mm_shuffle_epi8 (data7, shuffle_constant);
          _mm_storeu_si128 (datap + 3, data7);
        }

      /* Fold two 16-byte blocks into one 16-byte block */
      if (bytes_read >= 32)
        {
          data = _mm_loadu_si128 (datap);
          data = _mm_shuffle_epi8 (data, shuffle_constant);
          xor_crc = _mm_set_epi32 (crc, 0, 0, 0);
          crc = 0;
          data = _mm_xor_si128 (data, xor_crc);
          while (bytes_read >= 32)
            {
              datap++;

              data2 = _mm_clmulepi64_si128 (data, single_mult_constant, 0x00);
              data = _mm_clmulepi64_si128 (data, single_mult_constant, 0x11);
              fold_data = _mm_loadu_si128 (datap);
              fold_data = _mm_shuffle_epi8 (fold_data, shuffle_constant);
              data = _mm_xor_si128 (data, data2);
              data = _mm_xor_si128 (data, fold_data);
              bytes_read -= 16;
            }
          data = _mm_shuffle_epi8 (data, shuffle_constant);
          _mm_storeu_si128 (datap, data);
        }

      /* And finish up last 0-31 bytes in a byte by byte fashion */
      unsigned char *cp = (unsigned char *)datap;
      while (bytes_read--)
        crc = (crc << 8) ^ crctab[0][((crc >> 24) ^ *cp++) & 0xFF];
      if (feof (fp))
        break;
    }

  *crc_out = crc;
  *length_out = length;

  return !ferror (fp);
}
