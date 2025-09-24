/* wc_avx512 - Count the number of newlines with avx512 instructions.
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
wc_lines_avx512 (int fd)
{
  intmax_t lines = 0;
  intmax_t bytes = 0;

  __m512i endlines = _mm512_set1_epi8 ('\n');

  while (true)
    {
       __m512i avx_buf[IO_BUFSIZE / sizeof (__m512i)];
      ssize_t bytes_read = read (fd, avx_buf, sizeof avx_buf);
      if (bytes_read <= 0)
        return (struct wc_lines) { bytes_read == 0 ? 0 : errno, lines, bytes };

      bytes += bytes_read;
      __m512i *datap = avx_buf;

      while (bytes_read >= 64)
        {
           __m512i to_match = _mm512_load_si512 (datap);
           long long matches = _mm512_cmpeq_epi8_mask (to_match, endlines);
           lines += __builtin_popcountll (matches);
           datap += 1;
           bytes_read -= 64;
        }

      /* Finish up any left over bytes */
      char *end = (char *) datap + bytes_read;
      for (char *p = (char *) datap; p < end; p++)
        lines += *p == '\n';
    }
}
