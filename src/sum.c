/* sum -- checksum and count the blocks in a file
   Copyright (C) 1986-2025 Free Software Foundation, Inc.

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

/* Like BSD sum or SysV sum -r, except like SysV sum if -s option is given. */

/* Written by Kayvan Aghaiepour and David MacKenzie. */

#include <config.h>

#include <stdio.h>
#include <sys/types.h>
#include <endian.h>
#include "system.h"
#include "human.h"
#include "sum.h"

/* Calculate the checksum and the size in bytes of stream STREAM.
   Return -1 on error, 0 on success.  */

int
bsd_sum_stream (FILE *stream, void *resstream, uintmax_t *length)
{
  int ret = -1;
  size_t sum, n;
  int checksum = 0;	/* The checksum mod 2^16. */
  uintmax_t total_bytes = 0;	/* The number of bytes. */
  static const size_t buffer_length = 32768;
  uint8_t *buffer = malloc (buffer_length);

  if (! buffer)
    return -1;

  /* Process file */
  while (true)
  {
    sum = 0;

    /* Read block */
    while (true)
    {
      n = fread (buffer + sum, 1, buffer_length - sum, stream);
      sum += n;

      if (buffer_length == sum)
        break;

      if (n == 0)
        {
          if (ferror (stream))
            goto cleanup_buffer;
          goto final_process;
        }

      if (feof (stream))
        goto final_process;
    }

    for (size_t i = 0; i < sum; i++)
      {
        checksum = (checksum >> 1) + ((checksum & 1) << 15);
        checksum += buffer[i];
        checksum &= 0xffff;	/* Keep it within bounds. */
      }
    if (total_bytes + sum < total_bytes)
      {
        errno = EOVERFLOW;
        goto cleanup_buffer;
      }
    total_bytes += sum;
  }

final_process:;

  for (size_t i = 0; i < sum; i++)
    {
      checksum = (checksum >> 1) + ((checksum & 1) << 15);
      checksum += buffer[i];
      checksum &= 0xffff;	/* Keep it within bounds. */
    }
  if (total_bytes + sum < total_bytes)
    {
      errno = EOVERFLOW;
      goto cleanup_buffer;
    }
  total_bytes += sum;

  memcpy (resstream, &checksum, sizeof checksum);
  *length = total_bytes;
  ret = 0;
cleanup_buffer:
  free (buffer);
  return ret;
}

/* Calculate the checksum and the size in bytes of stream STREAM.
   Return -1 on error, 0 on success.  */

int
sysv_sum_stream (FILE *stream, void *resstream, uintmax_t *length)
{
  int ret = -1;
  size_t sum, n;
  uintmax_t total_bytes = 0;
  static const size_t buffer_length = 32768;
  uint8_t *buffer = malloc (buffer_length);

  if (! buffer)
    return -1;

  /* The sum of all the input bytes, modulo (UINT_MAX + 1).  */
  unsigned int s = 0;

  /* Process file */
  while (true)
  {
    sum = 0;

    /* Read block */
    while (true)
    {
      n = fread (buffer + sum, 1, buffer_length - sum, stream);
      sum += n;

      if (buffer_length == sum)
        break;

      if (n == 0)
        {
          if (ferror (stream))
            goto cleanup_buffer;
          goto final_process;
        }

      if (feof (stream))
        goto final_process;
    }

    for (size_t i = 0; i < sum; i++)
      s += buffer[i];
    if (total_bytes + sum < total_bytes)
      {
        errno = EOVERFLOW;
        goto cleanup_buffer;
      }
    total_bytes += sum;
  }

final_process:;

  for (size_t i = 0; i < sum; i++)
    s += buffer[i];
  if (total_bytes + sum < total_bytes)
    {
      errno = EOVERFLOW;
      goto cleanup_buffer;
    }
  total_bytes += sum;

  int r = (s & 0xffff) + ((s & 0xffffffff) >> 16);
  int checksum = (r & 0xffff) + (r >> 16);

  memcpy (resstream, &checksum, sizeof checksum);
  *length = total_bytes;
  ret = 0;
cleanup_buffer:
  free (buffer);
  return ret;
}

/* Print the checksum and size (in 1024 byte blocks) to stdout.
   If ARGS is true, also print the FILE name.  */

void
output_bsd (char const *file, MAYBE_UNUSED int binary_file, void const *digest,
            bool raw, MAYBE_UNUSED bool tagged, unsigned char delim, bool args,
            uintmax_t length)
{
  if (raw)
    {
      /* Output in network byte order (big endian).  */
      uint16_t out_int = *(int *)digest;
      out_int = htobe16 (out_int);
      fwrite (&out_int, 1, 16/8, stdout);
      return;
    }

  char hbuf[LONGEST_HUMAN_READABLE + 1];
  printf ("%05d %5s", *(int *)digest,
          human_readable (length, hbuf, human_ceiling, 1, 1024));
  if (args)
    printf (" %s", file);
  putchar (delim);
}

/* Print the checksum and size (in 512 byte blocks) to stdout.
   If ARGS is true, also print the FILE name.  */

void
output_sysv (char const *file, MAYBE_UNUSED int binary_file,
             void const *digest, bool raw, MAYBE_UNUSED bool tagged,
             unsigned char delim, bool args, uintmax_t length)
{
  if (raw)
    {
      /* Output in network byte order (big endian).  */
      uint16_t out_int = *(int *)digest;
      out_int = htobe16 (out_int);
      fwrite (&out_int, 1, 16/8, stdout);
      return;
    }

  char hbuf[LONGEST_HUMAN_READABLE + 1];
  printf ("%d %s", *(int *)digest,
          human_readable (length, hbuf, human_ceiling, 1, 512));
  if (args)
    printf (" %s", file);
  putchar (delim);
}
