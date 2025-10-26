/* cksum -- calculate and print POSIX checksums and sizes of files
   Copyright (C) 2024-2025 Free Software Foundation, Inc.

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

#include <sys/types.h>
#include <x86intrin.h>
#include "system.h"

/* Number of bytes to read at once.  */
#define BUFLEN (1 << 16)

bool
cksum_avx512 (FILE *fp, uint_fast32_t *crc_out, uintmax_t *length_out)
{
  __m512i buf[BUFLEN / sizeof (__m512i)];
  uint_fast32_t crc = 0;
  uintmax_t length = 0;
  size_t bytes_read;
  __m512i single_mult_constant;
  __m512i four_mult_constant;
  __m512i shuffle_constant;

  if (!fp || !crc_out || !length_out)
    return false;

  /* These constants and general algorithms are taken from the Intel whitepaper
     "Fast CRC Computation for Generic Polynomials Using PCLMULQDQ Instruction"
  */
  single_mult_constant = _mm512_set_epi64 (0x8833794C, 0xE6228B11,
                                           0x8833794C, 0xE6228B11,
                                           0x8833794C, 0xE6228B11,
                                           0x8833794C, 0xE6228B11);
  four_mult_constant = _mm512_set_epi64 (0xCBCF3BCB, 0x88FE2237,
                                         0xCBCF3BCB, 0x88FE2237,
                                         0xCBCF3BCB, 0x88FE2237,
                                         0xCBCF3BCB, 0x88FE2237);

  /* Constant to byteswap a full AVX512 register */
  shuffle_constant = _mm512_set_epi8 (0, 1, 2, 3, 4, 5, 6, 7, 8,
                                      9, 10, 11, 12, 13, 14, 15,
                                      0, 1, 2, 3, 4, 5, 6, 7, 8,
                                      9, 10, 11, 12, 13, 14, 15,
                                      0, 1, 2, 3, 4, 5, 6, 7, 8,
                                      9, 10, 11, 12, 13, 14, 15,
                                      0, 1, 2, 3, 4, 5, 6, 7, 8,
                                      9, 10, 11, 12, 13, 14, 15);
  while ((bytes_read = fread (buf, 1, BUFLEN, fp)) > 0)
    {
      __m512i data;
      __m512i data2;
      __m512i data3;
      __m512i data4;
      __m512i data5;
      __m512i data6;
      __m512i data7;
      __m512i data8;
      __m512i fold_data;
      __m512i xor_crc;

      __m512i *datap;

      if (ckd_add (&length, length, bytes_read))
        {
          errno = EOVERFLOW;
          return false;
        }

      datap = (__m512i *)buf;

      /* Fold in parallel 32x 16-byte blocks into 16x 16-byte blocks */
      if (bytes_read >= 16 * 8 * 4)
        {
          data = _mm512_loadu_si512 (datap);
          data = _mm512_shuffle_epi8 (data, shuffle_constant);
          /* XOR in initial CRC value (for us 0 so no effect), or CRC value
             calculated for previous BUFLEN buffer from fread */
          xor_crc = _mm512_set_epi32 (0, 0, 0, 0, 0, 0, 0, 0,
                                      0, 0, 0, 0, crc, 0, 0, 0);
          crc = 0;
          data = _mm512_xor_si512 (data, xor_crc);
          data3 = _mm512_loadu_si512 (datap + 1);
          data3 = _mm512_shuffle_epi8 (data3, shuffle_constant);
          data5 = _mm512_loadu_si512 (datap + 2);
          data5 = _mm512_shuffle_epi8 (data5, shuffle_constant);
          data7 = _mm512_loadu_si512 (datap + 3);
          data7 = _mm512_shuffle_epi8 (data7, shuffle_constant);

          while (bytes_read >= 16 * 8 * 4)
            {
              datap += 4;

              /* Do multiplication here for 16x consecutive 16 byte blocks */
              data2 = _mm512_clmulepi64_epi128 (data, four_mult_constant,
                                                0x00);
              data = _mm512_clmulepi64_epi128 (data, four_mult_constant,
                                               0x11);
              data4 = _mm512_clmulepi64_epi128 (data3, four_mult_constant,
                                                0x00);
              data3 = _mm512_clmulepi64_epi128 (data3, four_mult_constant,
                                                0x11);
              data6 = _mm512_clmulepi64_epi128 (data5, four_mult_constant,
                                                0x00);
              data5 = _mm512_clmulepi64_epi128 (data5, four_mult_constant,
                                                0x11);
              data8 = _mm512_clmulepi64_epi128 (data7, four_mult_constant,
                                                0x00);
              data7 = _mm512_clmulepi64_epi128 (data7, four_mult_constant,
                                                0x11);

              /* Now multiplication results for the 16x blocks is xor:ed with
                 next 16x 16 byte blocks from the buffer. This effectively
                 "consumes" the first 16x blocks from the buffer.
                 Keep xor result in variables for multiplication in next
                 round of loop. */
              data = _mm512_xor_si512 (data, data2);
              data2 = _mm512_loadu_si512 (datap);
              data2 = _mm512_shuffle_epi8 (data2, shuffle_constant);
              data = _mm512_xor_si512 (data, data2);

              data3 = _mm512_xor_si512 (data3, data4);
              data4 = _mm512_loadu_si512 (datap + 1);
              data4 = _mm512_shuffle_epi8 (data4, shuffle_constant);
              data3 = _mm512_xor_si512 (data3, data4);

              data5 = _mm512_xor_si512 (data5, data6);
              data6 = _mm512_loadu_si512 (datap + 2);
              data6 = _mm512_shuffle_epi8 (data6, shuffle_constant);
              data5 = _mm512_xor_si512 (data5, data6);

              data7 = _mm512_xor_si512 (data7, data8);
              data8 = _mm512_loadu_si512 (datap + 3);
              data8 = _mm512_shuffle_epi8 (data8, shuffle_constant);
              data7 = _mm512_xor_si512 (data7, data8);

              bytes_read -= (16 * 4 * 4);
            }
          /* At end of loop we write out results from variables back into
             the buffer, for use in single fold loop */
          data = _mm512_shuffle_epi8 (data, shuffle_constant);
          _mm512_storeu_si512 (datap, data);
          data3 = _mm512_shuffle_epi8 (data3, shuffle_constant);
          _mm512_storeu_si512 (datap + 1, data3);
          data5 = _mm512_shuffle_epi8 (data5, shuffle_constant);
          _mm512_storeu_si512 (datap + 2, data5);
          data7 = _mm512_shuffle_epi8 (data7, shuffle_constant);
          _mm512_storeu_si512 (datap + 3, data7);
        }

      /* Fold two 64-byte blocks into one 64-byte block */
      if (bytes_read >= 128)
        {
          data = _mm512_loadu_si512 (datap);
          data = _mm512_shuffle_epi8 (data, shuffle_constant);
          xor_crc = _mm512_set_epi32 (0, 0, 0, 0, 0, 0, 0, 0,
                                      0, 0, 0, 0, crc, 0, 0, 0);
          crc = 0;
          data = _mm512_xor_si512 (data, xor_crc);
          while (bytes_read >= 128)
            {
              datap++;

              data2 = _mm512_clmulepi64_epi128 (data, single_mult_constant,
                                                0x00);
              data = _mm512_clmulepi64_epi128 (data, single_mult_constant,
                                               0x11);
              fold_data = _mm512_loadu_si512 (datap);
              fold_data = _mm512_shuffle_epi8 (fold_data, shuffle_constant);
              data = _mm512_xor_si512 (data, data2);
              data = _mm512_xor_si512 (data, fold_data);
              bytes_read -= 64;
            }
          data = _mm512_shuffle_epi8 (data, shuffle_constant);
          _mm512_storeu_si512 (datap, data);
        }

      /* And finish up last 0-127 bytes in a byte by byte fashion */
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
