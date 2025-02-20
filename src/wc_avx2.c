/* wc_avx - Count the number of newlines with avx2 instructions.
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

#include "wc.h"
#include "system.h"
#include "ioblksize.h"

#include <x86intrin.h>

/* Read FD and return a summary.  */
extern struct wc_lines
wc_lines_avx2 (int fd)
{
  intmax_t lines = 0;
  intmax_t bytes = 0;

  __m256i endlines = _mm256_set1_epi8 ('\n');

  while (true)
    {
       __m256i avx_buf[IO_BUFSIZE / sizeof (__m256i)];
      ssize_t bytes_read = read (fd, avx_buf, sizeof avx_buf);
      if (bytes_read <= 0)
        return (struct wc_lines) { bytes_read == 0 ? 0 : errno, lines, bytes };

      bytes += bytes_read;
      __m256i *datap = avx_buf;

      while (bytes_read >= 32)
        {
           __m256i to_match = _mm256_load_si256 (datap);
           __m256i matches = _mm256_cmpeq_epi8 (to_match, endlines);
           int mask = _mm256_movemask_epi8 (matches);
           lines += __builtin_popcount (mask);
           datap += 1;
           bytes_read -= 32;
        }

      /* Finish up any left over bytes */
      char *end = (char *) datap + bytes_read;
      for (char *p = (char *) datap; p < end; p++)
        lines += *p == '\n';
    }
}
