/* sha.c - Functions to compute the SHA1 hash (message-digest) of files
   or blocks of memory.  Complies to the NIST specification FIPS-180-1.

   Copyright (C) 2000 Scott G. Miller

   Credits:
      Robert Klep <robert@ilse.nl>  -- Expansion function fix
*/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <sys/types.h>

#if STDC_HEADERS || defined _LIBC
# include <stdlib.h>
# include <string.h>
#else
# ifndef HAVE_MEMCPY
#  define memcpy(d, s, n) bcopy ((s), (d), (n))
# endif
#endif

#include "md5.h"
#include "sha.h"

/*
  Not-swap is a macro that does an endian swap on architectures that are
  big-endian, as SHA needs some data in a little-endian format
*/

#ifdef WORDS_BIGENDIAN
# define NOTSWAP(n) (n)
# define SWAP(n)							\
    (((n) << 24) | (((n) & 0xff00) << 8) | (((n) >> 8) & 0xff00) | ((n) >> 24))
#else
# define NOTSWAP(n)                                                         \
    (((n) << 24) | (((n) & 0xff00) << 8) | (((n) >> 8) & 0xff00) | ((n) >> 24))
# define SWAP(n) (n)
#endif

/* This array contains the bytes used to pad the buffer to the next
   64-byte boundary.  (RFC 1321, 3.1: Step 1)  */
static const unsigned char fillbuf[64] = { 0x80, 0 /* , 0, 0, ...  */ };


/*
  Takes a pointer to a 160 bit block of data (five 32 bit ints) and
  intializes it to the start constants of the SHA1 algorithm.  This
  must be called before using hash in the call to sha_hash
*/
void
sha_init_ctx (struct sha_ctx *ctx)
{
  ctx->A = 0x67452301;
  ctx->B = 0xefcdab89;
  ctx->C = 0x98badcfe;
  ctx->D = 0x10325476;
  ctx->E = 0xc3d2e1f0;

  ctx->total[0] = ctx->total[1] = 0;
  ctx->buflen = 0;
}

/* Put result from CTX in first 20 bytes following RESBUF.  The result
   must be in little endian byte order.

   IMPORTANT: On some systems it is required that RESBUF is correctly
   aligned for a 32 bits value.  */
void *
sha_read_ctx (const struct sha_ctx *ctx, void *resbuf)
{
  ((md5_uint32 *) resbuf)[0] = NOTSWAP (ctx->A);
  ((md5_uint32 *) resbuf)[1] = NOTSWAP (ctx->B);
  ((md5_uint32 *) resbuf)[2] = NOTSWAP (ctx->C);
  ((md5_uint32 *) resbuf)[3] = NOTSWAP (ctx->D);
  ((md5_uint32 *) resbuf)[4] = NOTSWAP (ctx->E);

  return resbuf;
}

/* Process the remaining bytes in the internal buffer and the usual
   prolog according to the standard and write the result to RESBUF.

   IMPORTANT: On some systems it is required that RESBUF is correctly
   aligned for a 32 bits value.  */
void *
sha_finish_ctx (struct sha_ctx *ctx, void *resbuf)
{
  /* Take yet unprocessed bytes into account.  */
  md5_uint32 bytes = ctx->buflen;
  size_t pad;

  /* Now count remaining bytes.  */
  ctx->total[0] += bytes;
  if (ctx->total[0] < bytes)
    ++ctx->total[1];

  pad = bytes >= 56 ? 64 + 56 - bytes : 56 - bytes;
  memcpy (&ctx->buffer[bytes], fillbuf, pad);

  /* Put the 64-bit file length in *bits* at the end of the buffer.  */
  *(md5_uint32 *) &ctx->buffer[bytes + pad + 4] = NOTSWAP (ctx->total[0] << 3);
  *(md5_uint32 *) &ctx->buffer[bytes + pad] = NOTSWAP ((ctx->total[1] << 3) |
						    (ctx->total[0] >> 29));

  /* Process last bytes.  */
  sha_process_block (ctx->buffer, bytes + pad + 8, ctx);

  return sha_read_ctx (ctx, resbuf);
}

/* Compute SHA1 message digest for bytes read from STREAM.  The
   resulting message digest number will be written into the 16 bytes
   beginning at RESBLOCK.  */
int
sha_stream (FILE *stream, void *resblock)
{
  /* Important: BLOCKSIZE must be a multiple of 64.  */
#define BLOCKSIZE 4096
  struct sha_ctx ctx;
  char buffer[BLOCKSIZE + 72];
  size_t sum;

  /* Initialize the computation context.  */
  sha_init_ctx (&ctx);

  /* Iterate over full file contents.  */
  while (1)
    {
      /* We read the file in blocks of BLOCKSIZE bytes.  One call of the
	 computation function processes the whole buffer so that with the
	 next round of the loop another block can be read.  */
      size_t n;
      sum = 0;

      /* Read block.  Take care for partial reads.  */
      do
	{
	  n = fread (buffer + sum, 1, BLOCKSIZE - sum, stream);

	  sum += n;
	}
      while (sum < BLOCKSIZE && n != 0);
      if (n == 0 && ferror (stream))
        return 1;

      /* If end of file is reached, end the loop.  */
      if (n == 0)
	break;

      /* Process buffer with BLOCKSIZE bytes.  Note that
			BLOCKSIZE % 64 == 0
       */
      sha_process_block (buffer, BLOCKSIZE, &ctx);
    }

  /* Add the last bytes if necessary.  */
  if (sum > 0)
    sha_process_bytes (buffer, sum, &ctx);

  /* Construct result in desired memory.  */
  sha_finish_ctx (&ctx, resblock);
  return 0;
}

/* Compute MD5 message digest for LEN bytes beginning at BUFFER.  The
   result is always in little endian byte order, so that a byte-wise
   output yields to the wanted ASCII representation of the message
   digest.  */
void *
sha_buffer (const char *buffer, size_t len, void *resblock)
{
  struct sha_ctx ctx;

  /* Initialize the computation context.  */
  sha_init_ctx (&ctx);

  /* Process whole buffer but last len % 64 bytes.  */
  sha_process_bytes (buffer, len, &ctx);

  /* Put result in desired memory area.  */
  return sha_finish_ctx (&ctx, resblock);
}

void
sha_process_bytes (const void *buffer, size_t len, struct sha_ctx *ctx)
{
  /* When we already have some bits in our internal buffer concatenate
     both inputs first.  */
  if (ctx->buflen != 0)
    {
      size_t left_over = ctx->buflen;
      size_t add = 128 - left_over > len ? len : 128 - left_over;

      memcpy (&ctx->buffer[left_over], buffer, add);
      ctx->buflen += add;

      if (left_over + add > 64)
	{
	  sha_process_block (ctx->buffer, (left_over + add) & ~63, ctx);
	  /* The regions in the following copy operation cannot overlap.  */
	  memcpy (ctx->buffer, &ctx->buffer[(left_over + add) & ~63],
		  (left_over + add) & 63);
	  ctx->buflen = (left_over + add) & 63;
	}

      buffer = (const char *) buffer + add;
      len -= add;
    }

  /* Process available complete blocks.  */
  if (len > 64)
    {
      sha_process_block (buffer, len & ~63, ctx);
      buffer = (const char *) buffer + (len & ~63);
      len &= 63;
    }

  /* Move remaining bytes in internal buffer.  */
  if (len > 0)
    {
      memcpy (ctx->buffer, buffer, len);
      ctx->buflen = len;
    }
}

/* --- Code below is the primary difference between md5.c and sha.c --- */

/* SHA1 round constants */
#define K1 0x5a827999L
#define K2 0x6ed9eba1L
#define K3 0x8f1bbcdcL
#define K4 0xca62c1d6L

/* Round functions.  Note that F2() is used in both rounds 2 and 4 */
#define F1(B,C,D) ( D ^ ( B & ( C ^ D ) ) )
#define F2(B,C,D) (B ^ C ^ D)
#define F3(B,C,D) ( ( B & C ) | ( D & ( B | C ) ) )

#if SHA_DEBUG
char bin2hex[16]={'0','1','2','3','4','5','6','7',
		  '8','9','a','b','c','d','e','f'};
# define BH(x) bin2hex[x]
# define PH(x) \
      printf("%c%c%c%c%c%c%c%c\t", BH((x>>28)&0xf), \
	     BH((x>>24)&0xf), BH((x>>20)&0xf), BH((x>>16)&0xf),\
	     BH((x>>12)&0xf), BH((x>>8)&0xf), BH((x>>4)&0xf),\
	     BH(x&0xf));
#endif

/* Process LEN bytes of BUFFER, accumulating context into CTX.
   It is assumed that LEN % 64 == 0.  */

void
sha_process_block (const void *buffer, size_t len, struct sha_ctx *ctx)
{
  const md5_uint32 *words = buffer;
  size_t nwords = len / sizeof (md5_uint32);
  const md5_uint32 *endp = words + nwords;
  md5_uint32 W[80];
  md5_uint32 A = ctx->A;
  md5_uint32 B = ctx->B;
  md5_uint32 C = ctx->C;
  md5_uint32 D = ctx->D;
  md5_uint32 E = ctx->E;

  /* First increment the byte count.  RFC 1321 specifies the possible
     length of the file up to 2^64 bits.  Here we only compute the
     number of bytes.  Do a double word increment.  */
  ctx->total[0] += len;
  if (ctx->total[0] < len)
    ++ctx->total[1];

  while (words < endp)
    {
      int t;
      for (t = 0; t < 16; t++)
	{
	  W[t] = NOTSWAP (*words);
	  words++;
	}

      /* SHA1 Data expansion */
      for (t = 16; t < 80; t++)
	{
	  md5_uint32 x = W[t - 3] ^ W[t - 8] ^ W[t - 14] ^ W[t - 16];
	  W[t] = rol (x, 1);
	}

      /* SHA1 main loop (t=0 to 79)
         This is broken down into four subloops in order to use
         the correct round function and constant */
      for (t = 0; t < 20; t++)
	{
	  md5_uint32 tmp = rol (A, 5) + F1 (B, C, D) + E + W[t] + K1;
	  E = D;
	  D = C;
	  C = rol (B, 30);
	  B = A;
	  A = tmp;
	}
      for (; t < 40; t++)
	{
	  md5_uint32 tmp = rol (A, 5) + F2 (B, C, D) + E + W[t] + K2;
	  E = D;
	  D = C;
	  C = rol (B, 30);
	  B = A;
	  A = tmp;
	}
      for (; t < 60; t++)
	{
	  md5_uint32 tmp = rol (A, 5) + F3 (B, C, D) + E + W[t] + K3;
	  E = D;
	  D = C;
	  C = rol (B, 30);
	  B = A;
	  A = tmp;
	}
      for (; t < 80; t++)
	{
	  md5_uint32 tmp = rol (A, 5) + F2 (B, C, D) + E + W[t] + K4;
	  E = D;
	  D = C;
	  C = rol (B, 30);
	  B = A;
	  A = tmp;
	}
      A = ctx->A += A;
      B = ctx->B += B;
      C = ctx->C += C;
      D = ctx->D += D;
      E = ctx->E += E;
    }
}
