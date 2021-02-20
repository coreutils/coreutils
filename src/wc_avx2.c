/* wc_avx - Count the number of newlines with avx2 instructions.
   Copyright (C) 2021 Free Software Foundation, Inc.

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

#include "system.h"
#include "error.h"
#include "safe-read.h"

#include <x86intrin.h>

/* This must be below 16 KB (16384) or else the accumulators can
   theoretically overflow, producing wrong result. This is 2*32 bytes below,
   so there is no single bytes in the optimal case. */
#define BUFSIZE (16320)

extern bool
wc_lines_avx2 (char const *file, int fd, uintmax_t *lines_out,
               uintmax_t *bytes_out);

extern bool
wc_lines_avx2 (char const *file, int fd, uintmax_t *lines_out,
               uintmax_t *bytes_out)
{
  __m256i accumulator;
  __m256i accumulator2;
  __m256i zeroes;
  __m256i endlines;
  __m256i avx_buf[BUFSIZE / sizeof (__m256i)];
  __m256i *datap;
  uintmax_t lines = 0;
  uintmax_t bytes = 0;
  size_t bytes_read = 0;


  if (!lines_out || !bytes_out)
    return false;

  /* Using two parallel accumulators gave a good performance increase.
     Adding a third gave no additional benefit, at least on an
     Intel Xeon E3-1231v3.  Maybe on a newer CPU with additional vector
     execution engines it would be a win. */
  accumulator = _mm256_setzero_si256 ();
  accumulator2 = _mm256_setzero_si256 ();
  zeroes = _mm256_setzero_si256 ();
  endlines = _mm256_set1_epi8 ('\n');

  while ((bytes_read = safe_read (fd, avx_buf, sizeof (avx_buf))) > 0)
    {
      __m256i to_match;
      __m256i to_match2;
      __m256i matches;
      __m256i matches2;

      if (bytes_read == SAFE_READ_ERROR)
        {
          error (0, errno, "%s", quotef (file));
          return false;
        }

      bytes += bytes_read;

      datap = avx_buf;
      char *end = ((char *)avx_buf) + bytes_read;

      while (bytes_read >= 64)
        {
          to_match = _mm256_load_si256 (datap);
          to_match2 = _mm256_load_si256 (datap + 1);

          matches = _mm256_cmpeq_epi8 (to_match, endlines);
          matches2 = _mm256_cmpeq_epi8 (to_match2, endlines);
          /* Compare will set each 8 bit integer in the register to 0xFF
             on match.  When we subtract it the 8 bit accumulators
             will underflow, so this is equal to adding 1. */
          accumulator = _mm256_sub_epi8 (accumulator, matches);
          accumulator2 = _mm256_sub_epi8 (accumulator2, matches2);

          datap += 2;
          bytes_read -= 64;
        }

      /* Horizontally add all 8 bit integers in the register,
         and then reset it */
      accumulator = _mm256_sad_epu8 (accumulator, zeroes);
      lines +=   _mm256_extract_epi16 (accumulator, 0)
               + _mm256_extract_epi16 (accumulator, 4)
               + _mm256_extract_epi16 (accumulator, 8)
               + _mm256_extract_epi16 (accumulator, 12);
      accumulator = _mm256_setzero_si256 ();

      accumulator2 = _mm256_sad_epu8 (accumulator2, zeroes);
      lines +=   _mm256_extract_epi16 (accumulator2, 0)
               + _mm256_extract_epi16 (accumulator2, 4)
               + _mm256_extract_epi16 (accumulator2, 8)
               + _mm256_extract_epi16 (accumulator2, 12);
      accumulator2 = _mm256_setzero_si256 ();

      /* Finish up any left over bytes */
      char *p = (char *)datap;
      while (p != end)
        lines += *p++ == '\n';
    }

  *lines_out = lines;
  *bytes_out = bytes;

  return true;
}
