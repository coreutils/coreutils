/* cksum -- calculate and print POSIX checksums and sizes of files
   Copyright (C) 1992-2023 Free Software Foundation, Inc.

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

/* Written by Q. Frank Xia, qx@math.columbia.edu.
   Cosmetic changes and reorganization by David MacKenzie, djm@gnu.ai.mit.edu.

  Usage: cksum [file...]

  The code segment between "#ifdef CRCTAB" and "#else" is the code
  which calculates the "crctab". It is included for those who want
  verify the correctness of the "crctab". To recreate the "crctab",
  do something like the following:

      cc -I../lib -DCRCTAB -o crctab cksum.c
      crctab > crctab.c

  This software is compatible with neither the System V nor the BSD
  'sum' program.  It is supposed to conform to POSIX, except perhaps
  for foreign language support.  Any inconsistency with the standard
  (other than foreign language support) is a bug.  */

#include <config.h>

#include <stdio.h>
#include <sys/types.h>
#include <stdint.h>
#include "system.h"

#include <byteswap.h>
#ifdef WORDS_BIGENDIAN
# define SWAP(n) (n)
#else
# define SWAP(n) bswap_32 (n)
#endif

#ifdef CRCTAB

# define BIT(x)	((uint_fast32_t) 1 << (x))
# define SBIT	BIT (31)

/* The generating polynomial is

          32   26   23   22   16   12   11   10   8   7   5   4   2   1
    G(X)=X  + X  + X  + X  + X  + X  + X  + X  + X + X + X + X + X + X + 1

  The i bit in GEN is set if X^i is a summand of G(X) except X^32.  */

# define GEN	(BIT (26) | BIT (23) | BIT (22) | BIT (16) | BIT (12) \
                 | BIT (11) | BIT (10) | BIT (8) | BIT (7) | BIT (5) \
                 | BIT (4) | BIT (2) | BIT (1) | BIT (0))

static uint_fast32_t r[8];

static void
fill_r (void)
{
  r[0] = GEN;
  for (int i = 1; i < 8; i++)
    r[i] = (r[i - 1] << 1) ^ ((r[i - 1] & SBIT) ? GEN : 0);
}

static uint_fast32_t
crc_remainder (int m)
{
  uint_fast32_t rem = 0;

  for (int i = 0; i < 8; i++)
    if (BIT (i) & m)
      rem ^= r[i];

  return rem & 0xFFFFFFFF;	/* Make it run on 64-bit machine.  */
}

int
main (void)
{
  int i;
  static uint_fast32_t crctab[8][256];

  fill_r ();

  for (i = 0; i < 256; i++)
    {
      crctab[0][i] = crc_remainder (i);
    }

  /* CRC(0x11 0x22 0x33 0x44)
     is equal to
     CRC(0x11 0x00 0x00 0x00) XOR CRC(0x22 0x00 0x00) XOR
     CRC(0x33 0x00) XOR CRC(0x44)
     We precompute the CRC values for the offset values into
     separate CRC tables. We can then use them to speed up
     CRC calculation by processing multiple bytes at the time. */
  for (i = 0; i < 256; i++)
    {
      uint32_t crc = 0;

      crc = (crc << 8) ^ crctab[0][((crc >> 24) ^ (i & 0xFF)) & 0xFF];
      for (idx_t offset = 1; offset < 8; offset++)
        {
          crc = (crc << 8) ^ crctab[0][((crc >> 24) ^ 0x00) & 0xFF];
          crctab[offset][i] = crc;
        }
    }

  printf ("#include <config.h>\n");
  printf ("#include <stdint.h>\n");
  printf ("\nuint_fast32_t const crctab[8][256] = {\n");
  for (int y = 0; y < 8; y++)
    {
      printf ("{\n  0x%08x", crctab[y][0]);
      for (i = 0; i < 51; i++)
        {
          printf (",\n  0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x",
                  crctab[y][i * 5 + 1], crctab[y][i * 5 + 2],
                  crctab[y][i * 5 + 3], crctab[y][i * 5 + 4],
                  crctab[y][i * 5 + 5]);
        }
        printf ("\n},\n");
    }
  printf ("};\n");
  return EXIT_SUCCESS;
}

#else /* !CRCTAB */

# include "cksum.h"

/* Number of bytes to read at once.  */
# define BUFLEN (1 << 16)

# if USE_PCLMUL_CRC32
static bool
pclmul_supported (void)
{
  bool pclmul_enabled = (0 < __builtin_cpu_supports ("pclmul")
                         && 0 < __builtin_cpu_supports ("avx"));

  if (cksum_debug)
    error (0, 0, "%s",
           (pclmul_enabled
            ? _("using pclmul hardware support")
            : _("pclmul support not detected")));

  return pclmul_enabled;
}
# endif /* USE_PCLMUL_CRC32 */

static bool
cksum_slice8 (FILE *fp, uint_fast32_t *crc_out, uintmax_t *length_out)
{
  uint32_t buf[BUFLEN / sizeof (uint32_t)];
  uint_fast32_t crc = 0;
  uintmax_t length = 0;
  size_t bytes_read;

  if (!fp || !crc_out || !length_out)
    return false;

  while ((bytes_read = fread (buf, 1, BUFLEN, fp)) > 0)
    {
      uint32_t *datap;

      if (length + bytes_read < length)
        {
          errno = EOVERFLOW;
          return false;
        }
      length += bytes_read;

      /* Process multiples of 8 bytes */
      datap = (uint32_t *)buf;
      while (bytes_read >= 8)
        {
          uint32_t first = *datap++, second = *datap++;
          crc ^= SWAP (first);
          second = SWAP (second);
          crc = (crctab[7][(crc >> 24) & 0xFF]
                 ^ crctab[6][(crc >> 16) & 0xFF]
                 ^ crctab[5][(crc >> 8) & 0xFF]
                 ^ crctab[4][(crc) & 0xFF]
                 ^ crctab[3][(second >> 24) & 0xFF]
                 ^ crctab[2][(second >> 16) & 0xFF]
                 ^ crctab[1][(second >> 8) & 0xFF]
                 ^ crctab[0][(second) & 0xFF]);
          bytes_read -= 8;
        }

      /* And finish up last 0-7 bytes in a byte by byte fashion */
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

/* Calculate the checksum and length in bytes of stream STREAM.
   Return -1 on error, 0 on success.  */

int
crc_sum_stream (FILE *stream, void *resstream, uintmax_t *length)
{
  uintmax_t total_bytes = 0;
  uint_fast32_t crc = 0;

# if USE_PCLMUL_CRC32
  static bool (*cksum_fp) (FILE *, uint_fast32_t *, uintmax_t *);
  if (! cksum_fp)
    cksum_fp = pclmul_supported () ? cksum_pclmul : cksum_slice8;
# else
  bool (*cksum_fp) (FILE *, uint_fast32_t *, uintmax_t *) = cksum_slice8;
# endif

  if (! cksum_fp (stream, &crc, &total_bytes))
    return -1;

  *length = total_bytes;

  for (; total_bytes; total_bytes >>= 8)
    crc = (crc << 8) ^ crctab[0][((crc >> 24) ^ total_bytes) & 0xFF];
  crc = ~crc & 0xFFFFFFFF;

  unsigned int crc_out = crc;
  memcpy (resstream, &crc_out, sizeof crc_out);

  return 0;
}

/* Print the checksum and size to stdout.
   If ARGS is true, also print the FILE name.  */

void
output_crc (char const *file, int binary_file, void const *digest, bool raw,
            bool tagged, unsigned char delim, bool args, uintmax_t length)
{
  if (raw)
    {
      /* Output in network byte order (big endian).  */
      uint32_t out_int = SWAP (*(uint32_t *)digest);
      fwrite (&out_int, 1, 32/8, stdout);
      return;
    }

  char length_buf[INT_BUFSIZE_BOUND (uintmax_t)];
  printf ("%u %s", *(unsigned int *)digest, umaxtostr (length, length_buf));
  if (args)
    printf (" %s", file);
  putchar (delim);
}

#endif /* !CRCTAB */
