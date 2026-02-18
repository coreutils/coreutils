/* wc_neon - Count the number of newlines with neon instructions.
   Copyright (C) 2026 Free Software Foundation, Inc.

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

/* Written by Collin Funk <collin.funk1@gmail.com>, 2026.  */

#include <config.h>

#include "wc.h"
#include "system.h"
#include "ioblksize.h"

#include <arm_neon.h>

/* Read FD and return a summary.  */
extern struct wc_lines
wc_lines_neon (int fd)
{
  intmax_t lines = 0;
  intmax_t bytes = 0;

  uint8x16_t endlines = vdupq_n_u8 ('\n');
  uint8x16_t ones = vdupq_n_u8 (1);

  while (true)
    {
      unsigned char neon_buf[IO_BUFSIZE];
      ssize_t bytes_read = read (fd, neon_buf, sizeof neon_buf);
      if (bytes_read <= 0)
        return (struct wc_lines) { bytes_read == 0 ? 0 : errno, lines, bytes };

      bytes += bytes_read;
      unsigned char *datap = neon_buf;

      while (64 <= bytes_read)
        {
          /* Load 64 bytes from NEON_BUF.  */
          uint8x16_t v0 = vld1q_u8 (datap);
          uint8x16_t v1 = vld1q_u8 (datap + 16);
          uint8x16_t v2 = vld1q_u8 (datap + 32);
          uint8x16_t v3 = vld1q_u8 (datap + 48);

          /* Bitwise equal with ENDLINES.  */
          uint8x16_t m0 = vceqq_u8 (v0, endlines);
          uint8x16_t m1 = vceqq_u8 (v1, endlines);
          uint8x16_t m2 = vceqq_u8 (v2, endlines);
          uint8x16_t m3 = vceqq_u8 (v3, endlines);

          /* Bitwise and with ONES.  */
          uint8x16_t s0 = vandq_u8 (m0, ones);
          uint8x16_t s1 = vandq_u8 (m1, ones);
          uint8x16_t s2 = vandq_u8 (m2, ones);
          uint8x16_t s3 = vandq_u8 (m3, ones);

          /* Sum the vectors.  */
          uint16x8_t a0 = vpaddlq_u8 (s0);
          uint16x8_t a1 = vpaddlq_u8 (s1);
          uint16x8_t a2 = vpaddlq_u8 (s2);
          uint16x8_t a3 = vpaddlq_u8 (s3);
          uint32x4_t b0 = vpaddlq_u16 (a0);
          uint32x4_t b1 = vpaddlq_u16 (a1);
          uint32x4_t b2 = vpaddlq_u16 (a2);
          uint32x4_t b3 = vpaddlq_u16 (a3);
          uint64x2_t c0 = vpaddlq_u32 (b0);
          uint64x2_t c1 = vpaddlq_u32 (b1);
          uint64x2_t c2 = vpaddlq_u32 (b2);
          uint64x2_t c3 = vpaddlq_u32 (b3);

          /* Extract the vectors.  */
          lines += (vgetq_lane_u64 (c0, 0) + vgetq_lane_u64 (c0, 1)
                    + vgetq_lane_u64 (c1, 0) + vgetq_lane_u64 (c1, 1)
                    + vgetq_lane_u64 (c2, 0) + vgetq_lane_u64 (c2, 1)
                    + vgetq_lane_u64 (c3, 0) + vgetq_lane_u64 (c3, 1));

          datap += 64;
          bytes_read -= 64;
        }

      while (16 <= bytes_read)
        {
          uint8x16_t v = vld1q_u8 (datap);
          uint8x16_t m = vceqq_u8 (v, endlines);
          uint8x16_t s = vandq_u8 (m, ones);
          uint16x8_t a = vpaddlq_u8 (s);
          uint32x4_t b = vpaddlq_u16 (a);
          uint64x2_t c = vpaddlq_u32 (b);
          lines += vgetq_lane_u64 (c, 0) + vgetq_lane_u64 (c, 1);
          datap += 16;
          bytes_read -= 16;
        }

      /* Finish up any left over bytes.  */
      unsigned char *end = (unsigned char *) datap + bytes_read;
      for (unsigned char *p = (unsigned char *) datap; p < end; p++)
        lines += *p == '\n';
    }
}
