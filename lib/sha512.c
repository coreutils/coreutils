/* sha512.c - Functions to compute SHA512 and SHA384 message digest of files or
   memory blocks according to the NIST specification FIPS-180-2.

   Copyright (C) 2005, 2006 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

/* Written by David Madore, considerably copypasting from
   Scott G. Miller's sha1.c
*/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "sha512.h"

#include <stddef.h>
#include <string.h>

#if USE_UNLOCKED_IO
# include "unlocked-io.h"
#endif

/*
  Not-swap is a macro that does an endian swap on architectures that are
  big-endian, as SHA512 needs some data in a little-endian format
*/

#ifdef WORDS_BIGENDIAN
# define NOTSWAP(n) (n)
#else
# define NOTSWAP(n)                                                         \
    (((n) << 56) | (((n) & 0xff00) << 40) | (((n) & 0xff0000UL) << 24) \
  | (((n) & 0xff000000UL) << 8) | (((n) >> 8) & 0xff000000UL) \
  | (((n) >> 24) & 0xff0000UL) | (((n) >> 40) & 0xff00UL) | ((n) >> 56))
#endif

#define BLOCKSIZE 4096
/* Ensure that BLOCKSIZE is a multiple of 128.  */
#if BLOCKSIZE % 128 != 0
# error "invalid BLOCKSIZE"
#endif

/* This array contains the bytes used to pad the buffer to the next
   64-byte boundary.  */
static const unsigned char fillbuf[128] = { 0x80, 0 /* , 0, 0, ...  */ };


/*
  Takes a pointer to a 512 bit block of data (eight 64 bit ints) and
  intializes it to the start constants of the SHA512 algorithm.  This
  must be called before using hash in the call to sha512_hash
*/
void
sha512_init_ctx (struct sha512_ctx *ctx)
{
  ctx->state[0] = 0x6a09e667f3bcc908ULL;
  ctx->state[1] = 0xbb67ae8584caa73bULL;
  ctx->state[2] = 0x3c6ef372fe94f82bULL;
  ctx->state[3] = 0xa54ff53a5f1d36f1ULL;
  ctx->state[4] = 0x510e527fade682d1ULL;
  ctx->state[5] = 0x9b05688c2b3e6c1fULL;
  ctx->state[6] = 0x1f83d9abfb41bd6bULL;
  ctx->state[7] = 0x5be0cd19137e2179ULL;

  ctx->total[0] = ctx->total[1] = 0;
  ctx->buflen = 0;
}

void
sha384_init_ctx (struct sha512_ctx *ctx)
{
  ctx->state[0] = 0xcbbb9d5dc1059ed8ULL;
  ctx->state[1] = 0x629a292a367cd507ULL;
  ctx->state[2] = 0x9159015a3070dd17ULL;
  ctx->state[3] = 0x152fecd8f70e5939ULL;
  ctx->state[4] = 0x67332667ffc00b31ULL;
  ctx->state[5] = 0x8eb44a8768581511ULL;
  ctx->state[6] = 0xdb0c2e0d64f98fa7ULL;
  ctx->state[7] = 0x47b5481dbefa4fa4ULL;

  ctx->total[0] = ctx->total[1] = 0;
  ctx->buflen = 0;
}

/* Put result from CTX in first 64 bytes following RESBUF.  The result
   must be in little endian byte order.

   IMPORTANT: On some systems it is required that RESBUF is correctly
   aligned for a 64-bit value.  */
void *
sha512_read_ctx (const struct sha512_ctx *ctx, void *resbuf)
{
  int i;

  for ( i=0 ; i<8 ; i++ )
    ((uint64_t *) resbuf)[i] = NOTSWAP (ctx->state[i]);

  return resbuf;
}

void *
sha384_read_ctx (const struct sha512_ctx *ctx, void *resbuf)
{
  int i;

  for ( i=0 ; i<6 ; i++ )
    ((uint64_t *) resbuf)[i] = NOTSWAP (ctx->state[i]);

  return resbuf;
}

/* Process the remaining bytes in the internal buffer and the usual
   prolog according to the standard and write the result to RESBUF.

   IMPORTANT: On some systems it is required that RESBUF is correctly
   aligned for a 64-bit value.  */
static void
sha512_conclude_ctx (struct sha512_ctx *ctx)
{
  /* Take yet unprocessed bytes into account.  */
  uint64_t bytes = ctx->buflen;
  size_t pad;

  /* Now count remaining bytes.  */
  ctx->total[0] += bytes;
  if (ctx->total[0] < bytes)
    ++ctx->total[1];

  pad = bytes >= 112 ? 128 + 112 - bytes : 112 - bytes;
  memcpy (&ctx->buffer[bytes], fillbuf, pad);

  /* Put the 64-bit file length in *bits* at the end of the buffer.  */
  *(uint64_t *) &ctx->buffer[bytes + pad + 8] = NOTSWAP (ctx->total[0] << 3);
  *(uint64_t *) &ctx->buffer[bytes + pad] = NOTSWAP ((ctx->total[1] << 3) |
						    (ctx->total[0] >> 61));

  /* Process last bytes.  */
  sha512_process_block (ctx->buffer, bytes + pad + 16, ctx);
}

void *
sha512_finish_ctx (struct sha512_ctx *ctx, void *resbuf)
{
  sha512_conclude_ctx (ctx);
  return sha512_read_ctx (ctx, resbuf);
}

void *
sha384_finish_ctx (struct sha512_ctx *ctx, void *resbuf)
{
  sha512_conclude_ctx (ctx);
  return sha384_read_ctx (ctx, resbuf);
}

/* Compute SHA512 message digest for bytes read from STREAM.  The
   resulting message digest number will be written into the 64 bytes
   beginning at RESBLOCK.  */
int
sha512_stream (FILE *stream, void *resblock)
{
  struct sha512_ctx ctx;
  char buffer[BLOCKSIZE + 72];
  size_t sum;

  /* Initialize the computation context.  */
  sha512_init_ctx (&ctx);

  /* Iterate over full file contents.  */
  while (1)
    {
      /* We read the file in blocks of BLOCKSIZE bytes.  One call of the
	 computation function processes the whole buffer so that with the
	 next round of the loop another block can be read.  */
      size_t n;
      sum = 0;

      /* Read block.  Take care for partial reads.  */
      while (1)
	{
	  n = fread (buffer + sum, 1, BLOCKSIZE - sum, stream);

	  sum += n;

	  if (sum == BLOCKSIZE)
	    break;

	  if (n == 0)
	    {
	      /* Check for the error flag IFF N == 0, so that we don't
		 exit the loop after a partial read due to e.g., EAGAIN
		 or EWOULDBLOCK.  */
	      if (ferror (stream))
		return 1;
	      goto process_partial_block;
	    }

	  /* We've read at least one byte, so ignore errors.  But always
	     check for EOF, since feof may be true even though N > 0.
	     Otherwise, we could end up calling fread after EOF.  */
	  if (feof (stream))
	    goto process_partial_block;
	}

      /* Process buffer with BLOCKSIZE bytes.  Note that
			BLOCKSIZE % 128 == 0
       */
      sha512_process_block (buffer, BLOCKSIZE, &ctx);
    }

 process_partial_block:;

  /* Process any remaining bytes.  */
  if (sum > 0)
    sha512_process_bytes (buffer, sum, &ctx);

  /* Construct result in desired memory.  */
  sha512_finish_ctx (&ctx, resblock);
  return 0;
}

/* FIXME: Avoid code duplication */
int
sha384_stream (FILE *stream, void *resblock)
{
  struct sha512_ctx ctx;
  char buffer[BLOCKSIZE + 72];
  size_t sum;

  /* Initialize the computation context.  */
  sha384_init_ctx (&ctx);

  /* Iterate over full file contents.  */
  while (1)
    {
      /* We read the file in blocks of BLOCKSIZE bytes.  One call of the
	 computation function processes the whole buffer so that with the
	 next round of the loop another block can be read.  */
      size_t n;
      sum = 0;

      /* Read block.  Take care for partial reads.  */
      while (1)
	{
	  n = fread (buffer + sum, 1, BLOCKSIZE - sum, stream);

	  sum += n;

	  if (sum == BLOCKSIZE)
	    break;

	  if (n == 0)
	    {
	      /* Check for the error flag IFF N == 0, so that we don't
		 exit the loop after a partial read due to e.g., EAGAIN
		 or EWOULDBLOCK.  */
	      if (ferror (stream))
		return 1;
	      goto process_partial_block;
	    }

	  /* We've read at least one byte, so ignore errors.  But always
	     check for EOF, since feof may be true even though N > 0.
	     Otherwise, we could end up calling fread after EOF.  */
	  if (feof (stream))
	    goto process_partial_block;
	}

      /* Process buffer with BLOCKSIZE bytes.  Note that
			BLOCKSIZE % 128 == 0
       */
      sha512_process_block (buffer, BLOCKSIZE, &ctx);
    }

 process_partial_block:;

  /* Process any remaining bytes.  */
  if (sum > 0)
    sha512_process_bytes (buffer, sum, &ctx);

  /* Construct result in desired memory.  */
  sha384_finish_ctx (&ctx, resblock);
  return 0;
}

/* Compute SHA512 message digest for LEN bytes beginning at BUFFER.  The
   result is always in little endian byte order, so that a byte-wise
   output yields to the wanted ASCII representation of the message
   digest.  */
void *
sha512_buffer (const char *buffer, size_t len, void *resblock)
{
  struct sha512_ctx ctx;

  /* Initialize the computation context.  */
  sha512_init_ctx (&ctx);

  /* Process whole buffer but last len % 128 bytes.  */
  sha512_process_bytes (buffer, len, &ctx);

  /* Put result in desired memory area.  */
  return sha512_finish_ctx (&ctx, resblock);
}

void *
sha384_buffer (const char *buffer, size_t len, void *resblock)
{
  struct sha512_ctx ctx;

  /* Initialize the computation context.  */
  sha384_init_ctx (&ctx);

  /* Process whole buffer but last len % 128 bytes.  */
  sha512_process_bytes (buffer, len, &ctx);

  /* Put result in desired memory area.  */
  return sha384_finish_ctx (&ctx, resblock);
}

void
sha512_process_bytes (const void *buffer, size_t len, struct sha512_ctx *ctx)
{
  /* When we already have some bits in our internal buffer concatenate
     both inputs first.  */
  if (ctx->buflen != 0)
    {
      size_t left_over = ctx->buflen;
      size_t add = 256 - left_over > len ? len : 256 - left_over;

      memcpy (&ctx->buffer[left_over], buffer, add);
      ctx->buflen += add;

      if (ctx->buflen > 128)
	{
	  sha512_process_block (ctx->buffer, ctx->buflen & ~63, ctx);

	  ctx->buflen &= 127;
	  /* The regions in the following copy operation cannot overlap.  */
	  memcpy (ctx->buffer, &ctx->buffer[(left_over + add) & ~127],
		  ctx->buflen);
	}

      buffer = (const char *) buffer + add;
      len -= add;
    }

  /* Process available complete blocks.  */
  if (len >= 128)
    {
#if !_STRING_ARCH_unaligned
# define alignof(type) offsetof (struct { char c; type x; }, x)
# define UNALIGNED_P(p) (((size_t) p) % alignof (uint64_t) != 0)
      if (UNALIGNED_P (buffer))
	while (len > 128)
	  {
	    sha512_process_block (memcpy (ctx->buffer, buffer, 128), 128, ctx);
	    buffer = (const char *) buffer + 128;
	    len -= 128;
	  }
      else
#endif
	{
	  sha512_process_block (buffer, len & ~127, ctx);
	  buffer = (const char *) buffer + (len & ~127);
	  len &= 127;
	}
    }

  /* Move remaining bytes in internal buffer.  */
  if (len > 0)
    {
      size_t left_over = ctx->buflen;

      memcpy (&ctx->buffer[left_over], buffer, len);
      left_over += len;
      if (left_over >= 128)
	{
	  sha512_process_block (ctx->buffer, 128, ctx);
	  left_over -= 128;
	  memcpy (ctx->buffer, &ctx->buffer[128], left_over);
	}
      ctx->buflen = left_over;
    }
}

/* --- Code below is the primary difference between sha1.c and sha512.c --- */

/* SHA512 round constants */
#define K(I) sha512_round_constants[I]
static const uint64_t sha512_round_constants[80] = {
 0x428a2f98d728ae22ULL, 0x7137449123ef65cdULL, 0xb5c0fbcfec4d3b2fULL, 0xe9b5dba58189dbbcULL, 0x3956c25bf348b538ULL, 0x59f111f1b605d019ULL, 0x923f82a4af194f9bULL, 0xab1c5ed5da6d8118ULL,
 0xd807aa98a3030242ULL, 0x12835b0145706fbeULL, 0x243185be4ee4b28cULL, 0x550c7dc3d5ffb4e2ULL, 0x72be5d74f27b896fULL, 0x80deb1fe3b1696b1ULL, 0x9bdc06a725c71235ULL, 0xc19bf174cf692694ULL,
 0xe49b69c19ef14ad2ULL, 0xefbe4786384f25e3ULL, 0x0fc19dc68b8cd5b5ULL, 0x240ca1cc77ac9c65ULL, 0x2de92c6f592b0275ULL, 0x4a7484aa6ea6e483ULL, 0x5cb0a9dcbd41fbd4ULL, 0x76f988da831153b5ULL,
 0x983e5152ee66dfabULL, 0xa831c66d2db43210ULL, 0xb00327c898fb213fULL, 0xbf597fc7beef0ee4ULL, 0xc6e00bf33da88fc2ULL, 0xd5a79147930aa725ULL, 0x06ca6351e003826fULL, 0x142929670a0e6e70ULL,
 0x27b70a8546d22ffcULL, 0x2e1b21385c26c926ULL, 0x4d2c6dfc5ac42aedULL, 0x53380d139d95b3dfULL, 0x650a73548baf63deULL, 0x766a0abb3c77b2a8ULL, 0x81c2c92e47edaee6ULL, 0x92722c851482353bULL,
 0xa2bfe8a14cf10364ULL, 0xa81a664bbc423001ULL, 0xc24b8b70d0f89791ULL, 0xc76c51a30654be30ULL, 0xd192e819d6ef5218ULL, 0xd69906245565a910ULL, 0xf40e35855771202aULL, 0x106aa07032bbd1b8ULL,
 0x19a4c116b8d2d0c8ULL, 0x1e376c085141ab53ULL, 0x2748774cdf8eeb99ULL, 0x34b0bcb5e19b48a8ULL, 0x391c0cb3c5c95a63ULL, 0x4ed8aa4ae3418acbULL, 0x5b9cca4f7763e373ULL, 0x682e6ff3d6b2b8a3ULL,
 0x748f82ee5defb2fcULL, 0x78a5636f43172f60ULL, 0x84c87814a1f0ab72ULL, 0x8cc702081a6439ecULL, 0x90befffa23631e28ULL, 0xa4506cebde82bde9ULL, 0xbef9a3f7b2c67915ULL, 0xc67178f2e372532bULL,
 0xca273eceea26619cULL, 0xd186b8c721c0c207ULL, 0xeada7dd6cde0eb1eULL, 0xf57d4f7fee6ed178ULL, 0x06f067aa72176fbaULL, 0x0a637dc5a2c898a6ULL, 0x113f9804bef90daeULL, 0x1b710b35131c471bULL,
 0x28db77f523047d84ULL, 0x32caab7b40c72493ULL, 0x3c9ebe0a15c9bebcULL, 0x431d67c49c100d4cULL, 0x4cc5d4becb3e42b6ULL, 0x597f299cfc657e2aULL, 0x5fcb6fab3ad6faecULL, 0x6c44198c4a475817ULL,
};

/* Round functions.  */
#define F2(A,B,C) ( ( A & B ) | ( C & ( A | B ) ) )
#define F1(E,F,G) ( G ^ ( E & ( F ^ G ) ) )

/* Process LEN bytes of BUFFER, accumulating context into CTX.
   It is assumed that LEN % 128 == 0.
   Most of this code comes from GnuPG's cipher/sha1.c.  */

void
sha512_process_block (const void *buffer, size_t len, struct sha512_ctx *ctx)
{
  const uint64_t *words = buffer;
  size_t nwords = len / sizeof (uint64_t);
  const uint64_t *endp = words + nwords;
  uint64_t x[16];
  uint64_t a = ctx->state[0];
  uint64_t b = ctx->state[1];
  uint64_t c = ctx->state[2];
  uint64_t d = ctx->state[3];
  uint64_t e = ctx->state[4];
  uint64_t f = ctx->state[5];
  uint64_t g = ctx->state[6];
  uint64_t h = ctx->state[7];

  /* First increment the byte count.  FIPS PUB 180-2 specifies the possible
     length of the file up to 2^128 bits.  Here we only compute the
     number of bytes.  Do a double word increment.  */
  ctx->total[0] += len;
  if (ctx->total[0] < len)
    ++ctx->total[1];

#define S0(x) (rol64(x,63)^rol64(x,56)^(x>>7))
#define S1(x) (rol64(x,45)^rol64(x,3)^(x>>6))
#define SS0(x) (rol64(x,36)^rol64(x,30)^rol64(x,25))
#define SS1(x) (rol64(x,50)^rol64(x,46)^rol64(x,23))

#define M(I) ( tm =   S1(x[(I-2)&0x0f]) + x[(I-7)&0x0f] \
		    + S0(x[(I-15)&0x0f]) + x[I&0x0f]    \
	       , x[I&0x0f] = tm )

#define R(A,B,C,D,E,F,G,H,K,M)  do { t0 = SS0(A) + F2(A,B,C); \
                                     t1 = H + SS1(E)  \
                                      + F1(E,F,G)     \
				      + K	      \
				      + M;	      \
				     D += t1;  H = t0 + t1; \
			       } while(0)

  while (words < endp)
    {
      uint64_t tm;
      uint64_t t0, t1;
      int t;
      /* FIXME: see sha1.c for a better implementation.  */
      for (t = 0; t < 16; t++)
	{
	  x[t] = NOTSWAP (*words);
	  words++;
	}

      R( a, b, c, d, e, f, g, h, K( 0), x[ 0] );
      R( h, a, b, c, d, e, f, g, K( 1), x[ 1] );
      R( g, h, a, b, c, d, e, f, K( 2), x[ 2] );
      R( f, g, h, a, b, c, d, e, K( 3), x[ 3] );
      R( e, f, g, h, a, b, c, d, K( 4), x[ 4] );
      R( d, e, f, g, h, a, b, c, K( 5), x[ 5] );
      R( c, d, e, f, g, h, a, b, K( 6), x[ 6] );
      R( b, c, d, e, f, g, h, a, K( 7), x[ 7] );
      R( a, b, c, d, e, f, g, h, K( 8), x[ 8] );
      R( h, a, b, c, d, e, f, g, K( 9), x[ 9] );
      R( g, h, a, b, c, d, e, f, K(10), x[10] );
      R( f, g, h, a, b, c, d, e, K(11), x[11] );
      R( e, f, g, h, a, b, c, d, K(12), x[12] );
      R( d, e, f, g, h, a, b, c, K(13), x[13] );
      R( c, d, e, f, g, h, a, b, K(14), x[14] );
      R( b, c, d, e, f, g, h, a, K(15), x[15] );
      R( a, b, c, d, e, f, g, h, K(16), M(16) );
      R( h, a, b, c, d, e, f, g, K(17), M(17) );
      R( g, h, a, b, c, d, e, f, K(18), M(18) );
      R( f, g, h, a, b, c, d, e, K(19), M(19) );
      R( e, f, g, h, a, b, c, d, K(20), M(20) );
      R( d, e, f, g, h, a, b, c, K(21), M(21) );
      R( c, d, e, f, g, h, a, b, K(22), M(22) );
      R( b, c, d, e, f, g, h, a, K(23), M(23) );
      R( a, b, c, d, e, f, g, h, K(24), M(24) );
      R( h, a, b, c, d, e, f, g, K(25), M(25) );
      R( g, h, a, b, c, d, e, f, K(26), M(26) );
      R( f, g, h, a, b, c, d, e, K(27), M(27) );
      R( e, f, g, h, a, b, c, d, K(28), M(28) );
      R( d, e, f, g, h, a, b, c, K(29), M(29) );
      R( c, d, e, f, g, h, a, b, K(30), M(30) );
      R( b, c, d, e, f, g, h, a, K(31), M(31) );
      R( a, b, c, d, e, f, g, h, K(32), M(32) );
      R( h, a, b, c, d, e, f, g, K(33), M(33) );
      R( g, h, a, b, c, d, e, f, K(34), M(34) );
      R( f, g, h, a, b, c, d, e, K(35), M(35) );
      R( e, f, g, h, a, b, c, d, K(36), M(36) );
      R( d, e, f, g, h, a, b, c, K(37), M(37) );
      R( c, d, e, f, g, h, a, b, K(38), M(38) );
      R( b, c, d, e, f, g, h, a, K(39), M(39) );
      R( a, b, c, d, e, f, g, h, K(40), M(40) );
      R( h, a, b, c, d, e, f, g, K(41), M(41) );
      R( g, h, a, b, c, d, e, f, K(42), M(42) );
      R( f, g, h, a, b, c, d, e, K(43), M(43) );
      R( e, f, g, h, a, b, c, d, K(44), M(44) );
      R( d, e, f, g, h, a, b, c, K(45), M(45) );
      R( c, d, e, f, g, h, a, b, K(46), M(46) );
      R( b, c, d, e, f, g, h, a, K(47), M(47) );
      R( a, b, c, d, e, f, g, h, K(48), M(48) );
      R( h, a, b, c, d, e, f, g, K(49), M(49) );
      R( g, h, a, b, c, d, e, f, K(50), M(50) );
      R( f, g, h, a, b, c, d, e, K(51), M(51) );
      R( e, f, g, h, a, b, c, d, K(52), M(52) );
      R( d, e, f, g, h, a, b, c, K(53), M(53) );
      R( c, d, e, f, g, h, a, b, K(54), M(54) );
      R( b, c, d, e, f, g, h, a, K(55), M(55) );
      R( a, b, c, d, e, f, g, h, K(56), M(56) );
      R( h, a, b, c, d, e, f, g, K(57), M(57) );
      R( g, h, a, b, c, d, e, f, K(58), M(58) );
      R( f, g, h, a, b, c, d, e, K(59), M(59) );
      R( e, f, g, h, a, b, c, d, K(60), M(60) );
      R( d, e, f, g, h, a, b, c, K(61), M(61) );
      R( c, d, e, f, g, h, a, b, K(62), M(62) );
      R( b, c, d, e, f, g, h, a, K(63), M(63) );
      R( a, b, c, d, e, f, g, h, K(64), M(64) );
      R( h, a, b, c, d, e, f, g, K(65), M(65) );
      R( g, h, a, b, c, d, e, f, K(66), M(66) );
      R( f, g, h, a, b, c, d, e, K(67), M(67) );
      R( e, f, g, h, a, b, c, d, K(68), M(68) );
      R( d, e, f, g, h, a, b, c, K(69), M(69) );
      R( c, d, e, f, g, h, a, b, K(70), M(70) );
      R( b, c, d, e, f, g, h, a, K(71), M(71) );
      R( a, b, c, d, e, f, g, h, K(72), M(72) );
      R( h, a, b, c, d, e, f, g, K(73), M(73) );
      R( g, h, a, b, c, d, e, f, K(74), M(74) );
      R( f, g, h, a, b, c, d, e, K(75), M(75) );
      R( e, f, g, h, a, b, c, d, K(76), M(76) );
      R( d, e, f, g, h, a, b, c, K(77), M(77) );
      R( c, d, e, f, g, h, a, b, K(78), M(78) );
      R( b, c, d, e, f, g, h, a, K(79), M(79) );

      a = ctx->state[0] += a;
      b = ctx->state[1] += b;
      c = ctx->state[2] += c;
      d = ctx->state[3] += d;
      e = ctx->state[4] += e;
      f = ctx->state[5] += f;
      g = ctx->state[6] += g;
      h = ctx->state[7] += h;
    }
}
