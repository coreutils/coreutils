/* Base64, base32, and similar encoding/decoding strings or files.
   Copyright (C) 2004-2025 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>. */

/* Written by Simon Josefsson <simon@josefsson.org>.  */

#include <config.h>

#include <stdio.h>
#include <getopt.h>
#include <sys/types.h>

#include "system.h"
#include "assure.h"
#include "c-ctype.h"
#include "fadvise.h"
#include "quote.h"
#include "xstrtol.h"
#include "xdectoint.h"
#include "xbinary-io.h"

#if BASE_TYPE == 42
# define AUTHORS \
  proper_name ("Simon Josefsson"), \
  proper_name ("Assaf Gordon")
#else
# define AUTHORS proper_name ("Simon Josefsson")
#endif

#if BASE_TYPE == 32
# include "base32.h"
# define PROGRAM_NAME "base32"
#elif BASE_TYPE == 64
# include "base64.h"
# define PROGRAM_NAME "base64"
#elif BASE_TYPE == 42
# include "base32.h"
# include "base64.h"
# include "assure.h"
# define PROGRAM_NAME "basenc"
#else
# error missing/invalid BASE_TYPE definition
#endif



#if BASE_TYPE == 42
enum
{
  BASE64_OPTION = CHAR_MAX + 1,
  BASE64URL_OPTION,
  BASE32_OPTION,
  BASE32HEX_OPTION,
  BASE16_OPTION,
  BASE2MSBF_OPTION,
  BASE2LSBF_OPTION,
  Z85_OPTION
};
#endif

static struct option const long_options[] =
{
  {"decode", no_argument, 0, 'd'},
  {"wrap", required_argument, 0, 'w'},
  {"ignore-garbage", no_argument, 0, 'i'},
#if BASE_TYPE == 42
  {"base64",    no_argument, 0, BASE64_OPTION},
  {"base64url", no_argument, 0, BASE64URL_OPTION},
  {"base32",    no_argument, 0, BASE32_OPTION},
  {"base32hex", no_argument, 0, BASE32HEX_OPTION},
  {"base16",    no_argument, 0, BASE16_OPTION},
  {"base2msbf", no_argument, 0, BASE2MSBF_OPTION},
  {"base2lsbf", no_argument, 0, BASE2LSBF_OPTION},
  {"z85",       no_argument, 0, Z85_OPTION},
#endif
  {GETOPT_HELP_OPTION_DECL},
  {GETOPT_VERSION_OPTION_DECL},
  {nullptr, 0, nullptr, 0}
};

void
usage (int status)
{
  if (status != EXIT_SUCCESS)
    emit_try_help ();
  else
    {
      printf (_("\
Usage: %s [OPTION]... [FILE]\n\
"), program_name);

#if BASE_TYPE == 42
      fputs (_("\
basenc encode or decode FILE, or standard input, to standard output.\n\
"), stdout);
#else
      printf (_("\
Base%d encode or decode FILE, or standard input, to standard output.\n\
"), BASE_TYPE);
#endif

      emit_stdin_note ();
      emit_mandatory_arg_note ();
#if BASE_TYPE == 42
      fputs (_("\
      --base64          same as 'base64' program (RFC4648 section 4)\n\
"), stdout);
      fputs (_("\
      --base64url       file- and url-safe base64 (RFC4648 section 5)\n\
"), stdout);
      fputs (_("\
      --base32          same as 'base32' program (RFC4648 section 6)\n\
"), stdout);
      fputs (_("\
      --base32hex       extended hex alphabet base32 (RFC4648 section 7)\n\
"), stdout);
      fputs (_("\
      --base16          hex encoding (RFC4648 section 8)\n\
"), stdout);
      fputs (_("\
      --base2msbf       bit string with most significant bit (msb) first\n\
"), stdout);
      fputs (_("\
      --base2lsbf       bit string with least significant bit (lsb) first\n\
"), stdout);
#endif
      fputs (_("\
  -d, --decode          decode data\n\
  -i, --ignore-garbage  when decoding, ignore non-alphabet characters\n\
  -w, --wrap=COLS       wrap encoded lines after COLS character (default 76).\n\
                          Use 0 to disable line wrapping\n\
"), stdout);
#if BASE_TYPE == 42
      fputs (_("\
      --z85             ascii85-like encoding (ZeroMQ spec:32/Z85);\n\
                        when encoding, input length must be a multiple of 4;\n\
                        when decoding, input length must be a multiple of 5\n\
"), stdout);
#endif
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
#if BASE_TYPE == 42
      fputs (_("\
\n\
When decoding, the input may contain newlines in addition to the bytes of\n\
the formal alphabet.  Use --ignore-garbage to attempt to recover\n\
from any other non-alphabet bytes in the encoded stream.\n\
"), stdout);
#else
      printf (_("\
\n\
The data are encoded as described for the %s alphabet in RFC 4648.\n\
When decoding, the input may contain newlines in addition to the bytes of\n\
the formal %s alphabet.  Use --ignore-garbage to attempt to recover\n\
from any other non-alphabet bytes in the encoded stream.\n"),
              PROGRAM_NAME, PROGRAM_NAME);
#endif
      emit_ancillary_info (PROGRAM_NAME);
    }

  exit (status);
}

#if BASE_TYPE != 64
static int
base32_required_padding (int len)
{
  int partial = len % 8;
  return partial ? 8 - partial : 0;
}
#endif

#if BASE_TYPE != 32
static int
base64_required_padding (int len)
{
  int partial = len % 4;
  return partial ? 4 - partial : 0;
}
#endif

#if BASE_TYPE == 42
static int
no_required_padding (int len)
{
  return 0;
}
#endif

#define ENC_BLOCKSIZE (1024 * 3 * 10)

#if BASE_TYPE == 32
# define BASE_LENGTH BASE32_LENGTH
# define REQUIRED_PADDING base32_required_padding
/* Note that increasing this may decrease performance if --ignore-garbage
   is used, because of the memmove operation below.  */
# define DEC_BLOCKSIZE (1024 * 5)

/* Ensure that BLOCKSIZE is a multiple of 5 and 8.  */
static_assert (ENC_BLOCKSIZE % 40 == 0); /* Padding chars only on last block. */
static_assert (DEC_BLOCKSIZE % 40 == 0); /* Complete encoded blocks are used. */

# define base_encode base32_encode
# define base_decode_context base32_decode_context
# define base_decode_ctx_init base32_decode_ctx_init
# define base_decode_ctx base32_decode_ctx
# define isubase isubase32
#elif BASE_TYPE == 64
# define BASE_LENGTH BASE64_LENGTH
# define REQUIRED_PADDING base64_required_padding
/* Note that increasing this may decrease performance if --ignore-garbage
   is used, because of the memmove operation below.  */
# define DEC_BLOCKSIZE (1024 * 3)

/* Ensure that BLOCKSIZE is a multiple of 3 and 4.  */
static_assert (ENC_BLOCKSIZE % 12 == 0); /* Padding chars only on last block. */
static_assert (DEC_BLOCKSIZE % 12 == 0); /* Complete encoded blocks are used. */

# define base_encode base64_encode
# define base_decode_context base64_decode_context
# define base_decode_ctx_init base64_decode_ctx_init
# define base_decode_ctx base64_decode_ctx
# define isubase isubase64
#elif BASE_TYPE == 42


# define BASE_LENGTH base_length
# define REQUIRED_PADDING required_padding

/* Note that increasing this may decrease performance if --ignore-garbage
   is used, because of the memmove operation below.  */
# define DEC_BLOCKSIZE (4200)
static_assert (DEC_BLOCKSIZE % 40 == 0); /* complete encoded blocks for base32*/
static_assert (DEC_BLOCKSIZE % 12 == 0); /* complete encoded blocks for base64*/

static int (*base_length) (int i);
static int (*required_padding) (int i);
static bool (*isubase) (unsigned char ch);
static void (*base_encode) (char const *restrict in, idx_t inlen,
                            char *restrict out, idx_t outlen);

struct base16_decode_context
{
  /* Either a 4-bit nibble, or negative if we have no nibble.  */
  signed char nibble;
};

struct z85_decode_context
{
  int i;
  unsigned char octets[5];
};

struct base2_decode_context
{
  unsigned char octet;
};

struct base_decode_context
{
  int i; /* will be updated manually */
  union {
    struct base64_decode_context base64;
    struct base32_decode_context base32;
    struct base16_decode_context base16;
    struct base2_decode_context base2;
    struct z85_decode_context z85;
  } ctx;
  char *inbuf;
  idx_t bufsize;
};
static void (*base_decode_ctx_init) (struct base_decode_context *ctx);
static bool (*base_decode_ctx) (struct base_decode_context *ctx,
                                char const *restrict in, idx_t inlen,
                                char *restrict out, idx_t *outlen);
#endif




#if BASE_TYPE == 42

static int
base64_length_wrapper (int len)
{
  return BASE64_LENGTH (len);
}

static void
base64_decode_ctx_init_wrapper (struct base_decode_context *ctx)
{
  base64_decode_ctx_init (&ctx->ctx.base64);
}

static bool
base64_decode_ctx_wrapper (struct base_decode_context *ctx,
                           char const *restrict in, idx_t inlen,
                           char *restrict out, idx_t *outlen)
{
  bool b = base64_decode_ctx (&ctx->ctx.base64, in, inlen, out, outlen);
  ctx->i = ctx->ctx.base64.i;
  return b;
}

static void
init_inbuf (struct base_decode_context *ctx)
{
  ctx->bufsize = DEC_BLOCKSIZE;
  ctx->inbuf = xcharalloc (ctx->bufsize);
}

static void
prepare_inbuf (struct base_decode_context *ctx, idx_t inlen)
{
  if (ctx->bufsize < inlen)
    ctx->inbuf = xpalloc (ctx->inbuf, &ctx->bufsize,
                          inlen - ctx->bufsize, -1, sizeof *ctx->inbuf);
}


static void
base64url_encode (char const *restrict in, idx_t inlen,
                  char *restrict out, idx_t outlen)
{
  base64_encode (in, inlen, out, outlen);
  /* translate 62nd and 63rd characters */
  char *p = out;
  while (outlen--)
    {
      if (*p == '+')
        *p = '-';
      else if (*p == '/')
        *p = '_';
      ++p;
    }
}

static bool
isubase64url (unsigned char ch)
{
  return (ch == '-' || ch == '_'
          || (ch != '+' && ch != '/' && isubase64 (ch)));
}

static void
base64url_decode_ctx_init_wrapper (struct base_decode_context *ctx)
{
  base64_decode_ctx_init (&ctx->ctx.base64);
  init_inbuf (ctx);
}


static bool
base64url_decode_ctx_wrapper (struct base_decode_context *ctx,
                              char const *restrict in, idx_t inlen,
                              char *restrict out, idx_t *outlen)
{
  prepare_inbuf (ctx, inlen);
  memcpy (ctx->inbuf, in, inlen);

  /* translate 62nd and 63rd characters */
  idx_t i = inlen;
  char *p = ctx->inbuf;
  while (i--)
    {
      if (*p == '+' || *p == '/')
        {
          *outlen = 0;
          return false; /* reject base64 input */
        }
      else if (*p == '-')
        *p = '+';
      else if (*p == '_')
        *p = '/';
      ++p;
    }

  bool b = base64_decode_ctx (&ctx->ctx.base64, ctx->inbuf, inlen,
                              out, outlen);
  ctx->i = ctx->ctx.base64.i;

  return b;
}



static int
base32_length_wrapper (int len)
{
  return BASE32_LENGTH (len);
}

static void
base32_decode_ctx_init_wrapper (struct base_decode_context *ctx)
{
  base32_decode_ctx_init (&ctx->ctx.base32);
}

static bool
base32_decode_ctx_wrapper (struct base_decode_context *ctx,
                           char const *restrict in, idx_t inlen,
                           char *restrict out, idx_t *outlen)
{
  bool b = base32_decode_ctx (&ctx->ctx.base32, in, inlen, out, outlen);
  ctx->i = ctx->ctx.base32.i;
  return b;
}

/* ABCDEFGHIJKLMNOPQRSTUVWXYZ234567
     to
   0123456789ABCDEFGHIJKLMNOPQRSTUV */
static const char base32_norm_to_hex[32 + 9] = {
/*0x32, 0x33, 0x34, 0x35, 0x36, 0x37, */
  'Q',  'R',  'S',  'T',  'U',  'V',

  0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40,

/*0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, */
  '0',  '1',  '2',  '3',  '4',  '5',  '6',  '7',

/*0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0x50, */
  '8',  '9',  'A',  'B',  'C',  'D',  'E',  'F',

/*0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, */
  'G',  'H',  'I',  'J',  'K',  'L',  'M',  'N',

/*0x59, 0x5a, */
  'O',  'P',
};

/* 0123456789ABCDEFGHIJKLMNOPQRSTUV
     to
   ABCDEFGHIJKLMNOPQRSTUVWXYZ234567 */
static const char base32_hex_to_norm[32 + 9] = {
  /* from: 0x30 .. 0x39 ('0' to '9') */
  /* to:*/ 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',

  0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40,

  /* from: 0x41 .. 0x4A ('A' to 'J') */
  /* to:*/ 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',

  /* from: 0x4B .. 0x54 ('K' to 'T') */
  /* to:*/ 'U', 'V', 'W', 'X', 'Y', 'Z', '2', '3', '4', '5',

  /* from: 0x55 .. 0x56 ('U' to 'V') */
  /* to:*/ '6', '7'
};


inline static bool
isubase32hex (unsigned char ch)
{
  return ('0' <= ch && ch <= '9') || ('A' <= ch && ch <= 'V');
}


static void
base32hex_encode (char const *restrict in, idx_t inlen,
                  char *restrict out, idx_t outlen)
{
  base32_encode (in, inlen, out, outlen);

  for (char *p = out; outlen--; p++)
    {
      affirm (0x32 <= *p && *p <= 0x5a);          /* LCOV_EXCL_LINE */
      *p = base32_norm_to_hex[*p - 0x32];
    }
}


static void
base32hex_decode_ctx_init_wrapper (struct base_decode_context *ctx)
{
  base32_decode_ctx_init (&ctx->ctx.base32);
  init_inbuf (ctx);
}


static bool
base32hex_decode_ctx_wrapper (struct base_decode_context *ctx,
                              char const *restrict in, idx_t inlen,
                              char *restrict out, idx_t *outlen)
{
  prepare_inbuf (ctx, inlen);

  idx_t i = inlen;
  char *p = ctx->inbuf;
  while (i--)
    {
      if (isubase32hex (*in))
        *p = base32_hex_to_norm[*in - 0x30];
      else
        *p = *in;
      ++p;
      ++in;
    }

  bool b = base32_decode_ctx (&ctx->ctx.base32, ctx->inbuf, inlen,
                              out, outlen);
  ctx->i = ctx->ctx.base32.i;

  return b;
}
/* With this approach this file works independent of the charset used
   (think EBCDIC).  However, it does assume that the characters in the
   Base32 alphabet (A-Z2-7) are encoded in 0..255.  POSIX
   1003.1-2001 require that char and unsigned char are 8-bit
   quantities, though, taking care of that problem.  But this may be a
   potential problem on non-POSIX C99 platforms.

   IBM C V6 for AIX mishandles "#define B32(x) ...'x'...", so use "_"
   as the formal parameter rather than "x".  */
# define B16(_)                                 \
  ((_) == '0' ? 0                               \
   : (_) == '1' ? 1                             \
   : (_) == '2' ? 2                             \
   : (_) == '3' ? 3                             \
   : (_) == '4' ? 4                             \
   : (_) == '5' ? 5                             \
   : (_) == '6' ? 6                             \
   : (_) == '7' ? 7                             \
   : (_) == '8' ? 8                             \
   : (_) == '9' ? 9                             \
   : (_) == 'A' || (_) == 'a' ? 10              \
   : (_) == 'B' || (_) == 'b' ? 11              \
   : (_) == 'C' || (_) == 'c' ? 12              \
   : (_) == 'D' || (_) == 'd' ? 13              \
   : (_) == 'E' || (_) == 'e' ? 14              \
   : (_) == 'F' || (_) == 'f' ? 15              \
   : -1)

static signed char const base16_to_int[256] = {
  B16 (0), B16 (1), B16 (2), B16 (3),
  B16 (4), B16 (5), B16 (6), B16 (7),
  B16 (8), B16 (9), B16 (10), B16 (11),
  B16 (12), B16 (13), B16 (14), B16 (15),
  B16 (16), B16 (17), B16 (18), B16 (19),
  B16 (20), B16 (21), B16 (22), B16 (23),
  B16 (24), B16 (25), B16 (26), B16 (27),
  B16 (28), B16 (29), B16 (30), B16 (31),
  B16 (32), B16 (33), B16 (34), B16 (35),
  B16 (36), B16 (37), B16 (38), B16 (39),
  B16 (40), B16 (41), B16 (42), B16 (43),
  B16 (44), B16 (45), B16 (46), B16 (47),
  B16 (48), B16 (49), B16 (50), B16 (51),
  B16 (52), B16 (53), B16 (54), B16 (55),
  B16 (56), B16 (57), B16 (58), B16 (59),
  B16 (60), B16 (61), B16 (62), B16 (63),
  B16 (32), B16 (65), B16 (66), B16 (67),
  B16 (68), B16 (69), B16 (70), B16 (71),
  B16 (72), B16 (73), B16 (74), B16 (75),
  B16 (76), B16 (77), B16 (78), B16 (79),
  B16 (80), B16 (81), B16 (82), B16 (83),
  B16 (84), B16 (85), B16 (86), B16 (87),
  B16 (88), B16 (89), B16 (90), B16 (91),
  B16 (92), B16 (93), B16 (94), B16 (95),
  B16 (96), B16 (97), B16 (98), B16 (99),
  B16 (100), B16 (101), B16 (102), B16 (103),
  B16 (104), B16 (105), B16 (106), B16 (107),
  B16 (108), B16 (109), B16 (110), B16 (111),
  B16 (112), B16 (113), B16 (114), B16 (115),
  B16 (116), B16 (117), B16 (118), B16 (119),
  B16 (120), B16 (121), B16 (122), B16 (123),
  B16 (124), B16 (125), B16 (126), B16 (127),
  B16 (128), B16 (129), B16 (130), B16 (131),
  B16 (132), B16 (133), B16 (134), B16 (135),
  B16 (136), B16 (137), B16 (138), B16 (139),
  B16 (140), B16 (141), B16 (142), B16 (143),
  B16 (144), B16 (145), B16 (146), B16 (147),
  B16 (148), B16 (149), B16 (150), B16 (151),
  B16 (152), B16 (153), B16 (154), B16 (155),
  B16 (156), B16 (157), B16 (158), B16 (159),
  B16 (160), B16 (161), B16 (162), B16 (163),
  B16 (132), B16 (165), B16 (166), B16 (167),
  B16 (168), B16 (169), B16 (170), B16 (171),
  B16 (172), B16 (173), B16 (174), B16 (175),
  B16 (176), B16 (177), B16 (178), B16 (179),
  B16 (180), B16 (181), B16 (182), B16 (183),
  B16 (184), B16 (185), B16 (186), B16 (187),
  B16 (188), B16 (189), B16 (190), B16 (191),
  B16 (192), B16 (193), B16 (194), B16 (195),
  B16 (196), B16 (197), B16 (198), B16 (199),
  B16 (200), B16 (201), B16 (202), B16 (203),
  B16 (204), B16 (205), B16 (206), B16 (207),
  B16 (208), B16 (209), B16 (210), B16 (211),
  B16 (212), B16 (213), B16 (214), B16 (215),
  B16 (216), B16 (217), B16 (218), B16 (219),
  B16 (220), B16 (221), B16 (222), B16 (223),
  B16 (224), B16 (225), B16 (226), B16 (227),
  B16 (228), B16 (229), B16 (230), B16 (231),
  B16 (232), B16 (233), B16 (234), B16 (235),
  B16 (236), B16 (237), B16 (238), B16 (239),
  B16 (240), B16 (241), B16 (242), B16 (243),
  B16 (244), B16 (245), B16 (246), B16 (247),
  B16 (248), B16 (249), B16 (250), B16 (251),
  B16 (252), B16 (253), B16 (254), B16 (255)
};

static bool
isubase16 (unsigned char ch)
{
  return ch < sizeof base16_to_int && 0 <= base16_to_int[ch];
}

static int
base16_length (int len)
{
  return len * 2;
}


static void
base16_encode (char const *restrict in, idx_t inlen,
               char *restrict out, idx_t outlen)
{
  static const char base16[16] = "0123456789ABCDEF";

  while (inlen && outlen)
    {
      unsigned char c = *in;
      *out++ = base16[c >> 4];
      *out++ = base16[c & 0x0F];
      ++in;
      inlen--;
      outlen -= 2;
    }
}


static void
base16_decode_ctx_init (struct base_decode_context *ctx)
{
  init_inbuf (ctx);
  ctx->ctx.base16.nibble = -1;
  ctx->i = 1;
}


static bool
base16_decode_ctx (struct base_decode_context *ctx,
                   char const *restrict in, idx_t inlen,
                   char *restrict out, idx_t *outlen)
{
  bool ignore_lines = true;  /* for now, always ignore them */
  char *out0 = out;
  signed char nibble = ctx->ctx.base16.nibble;

  /* inlen==0 is request to flush output.
     if there is a dangling high nibble - we are missing the low nibble,
     so return false - indicating an invalid input.  */
  if (inlen == 0)
    {
      *outlen = 0;
      return nibble < 0;
    }

  while (inlen--)
    {
      unsigned char c = *in++;
      if (ignore_lines && c == '\n')
        continue;

      if (sizeof base16_to_int <= c || base16_to_int[c] < 0)
        {
          *outlen = out - out0;
          return false; /* garbage - return false */
        }

      if (nibble < 0)
        nibble = base16_to_int[c];
      else
        {
          /* have both nibbles, write octet */
          *out++ = (nibble << 4) + base16_to_int[c];
          nibble = -1;
        }
    }

  ctx->ctx.base16.nibble = nibble;
  *outlen = out - out0;
  return true;
}




static int
z85_length (int len)
{
  /* Z85 does not allow padding, so no need to round to highest integer.  */
  int outlen = (len * 5) / 4;
  return outlen;
}

static bool
isuz85 (unsigned char ch)
{
  return c_isalnum (ch) || strchr (".-:+=^!/*?&<>()[]{}@%$#", ch) != nullptr;
}

static char const z85_encoding[85] =
  "0123456789"
  "abcdefghijklmnopqrstuvwxyz"
  "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
  ".-:+=^!/*?&<>()[]{}@%$#";

static void
z85_encode (char const *restrict in, idx_t inlen,
            char *restrict out, idx_t outlen)
{
  int i = 0;
  unsigned char quad[4];
  idx_t outidx = 0;

  while (true)
    {
      if (inlen == 0)
        {
          /* no more input, exactly on 4 octet boundary. */
          if (i == 0)
            return;

          /* currently, there's no way to return an error in encoding.  */
          error (EXIT_FAILURE, 0,
                 _("invalid input (length must be multiple of 4 characters)"));
        }
      else
        {
          quad[i++] = *in++;
          --inlen;
        }

      /* Got a quad, encode it */
      if (i == 4)
        {
          int_fast64_t val = quad[0];
          val = (val << 24) + (quad[1] << 16) + (quad[2] << 8) + quad[3];

          for (int j = 4; j >= 0; --j)
            {
              int c = val % 85;
              val /= 85;

              /* NOTE: if there is padding (which is trimmed by z85
                 before outputting the result), the output buffer 'out'
                 might not include enough allocated bytes for the padding,
                 so don't store them. */
              if (outidx + j < outlen)
                out[j] = z85_encoding[c];
            }
          out += 5;
          outidx += 5;
          i = 0;
        }
    }
}

static void
z85_decode_ctx_init (struct base_decode_context *ctx)
{
  init_inbuf (ctx);
  ctx->ctx.z85.i = 0;
  ctx->i = 1;
}


# define Z85_LO_CTX_TO_32BIT_VAL(ctx) \
  (((ctx)->ctx.z85.octets[1] * 85 * 85 * 85) +      \
   ((ctx)->ctx.z85.octets[2] * 85 * 85) +	    \
   ((ctx)->ctx.z85.octets[3] * 85) +		    \
   ((ctx)->ctx.z85.octets[4]))


# define Z85_HI_CTX_TO_32BIT_VAL(ctx) \
  ((int_fast64_t) (ctx)->ctx.z85.octets[0] * 85 * 85 * 85 * 85 )

/*
 0 -  9:  0 1 2 3 4 5 6 7 8 9
 10 - 19:  a b c d e f g h i j
 20 - 29:  k l m n o p q r s t
 30 - 39:  u v w x y z A B C D
 40 - 49:  E F G H I J K L M N
 50 - 59:  O P Q R S T U V W X
 60 - 69:  Y Z . - : + = ^ ! /   #dummy comment to workaround syntax-check
 70 - 79:  * ? & < > ( ) [ ] {
 80 - 84:  } @ % $ #
*/
static signed char const z85_decoding[93] = {
  68, -1,  84,  83, 82,  72, -1,               /* ! " # $ % & ' */
  75, 76,  70,  65, -1,  63, 62, 69,           /* ( ) * + , - . / */
  0,  1,   2,   3,  4,   5,  6,   7,  8,  9,   /* '0' to '9' */
  64, -1,  73,  66, 74,  71, 81,               /* : ; < =  > ? @ */
  36, 37,  38,  39, 40,  41, 42,  43, 44, 45,  /* 'A' to 'J' */
  46, 47,  48,  49, 50,  51, 52,  53, 54, 55,  /* 'K' to 'T' */
  56, 57,  58,  59, 60,  61,                   /* 'U' to 'Z' */
  77,  -1, 78,  67,  -1,  -1,                  /* [ \ ] ^ _ ` */
  10, 11,  12,  13, 14,  15, 16,  17, 18, 19,  /* 'a' to 'j' */
  20, 21,  22,  23, 24,  25, 26,  27, 28, 29,  /* 'k' to 't' */
  30, 31,  32,  33, 34,  35,                   /* 'u' to 'z' */
  79, -1,  80                                  /* { | } */
};

static bool
z85_decode_ctx (struct base_decode_context *ctx,
                char const *restrict in, idx_t inlen,
                char *restrict out, idx_t *outlen)
{
  bool ignore_lines = true;  /* for now, always ignore them */

  *outlen = 0;

  /* inlen==0 is request to flush output.
     if there are dangling values - we are missing entries,
     so return false - indicating an invalid input.  */
  if (inlen == 0)
    {
      if (ctx->ctx.z85.i > 0)
        {
          /* Z85 variant does not allow padding - input must
             be a multiple of 5 - so return error.  */
          return false;
        }
      return true;
    }

  while (inlen--)
    {
      if (ignore_lines && *in == '\n')
        {
          ++in;
          continue;
        }

      /* z85 decoding */
      unsigned char c = *in;

      if (c >= 33 && c <= 125)
        {
          signed char ch = z85_decoding[c - 33];
          if (ch < 0)
            return false; /* garbage - return false */
          c = ch;
        }
      else
        return false; /* garbage - return false */

      ++in;

      ctx->ctx.z85.octets[ctx->ctx.z85.i++] = c;
      if (ctx->ctx.z85.i == 5)
        {
          /* decode the lowest 4 octets, then check for overflows.  */
          int_fast64_t val = Z85_LO_CTX_TO_32BIT_VAL (ctx);

          /* The Z85 spec and the reference implementation say nothing
             about overflows. To be on the safe side, reject them.  */

          val += Z85_HI_CTX_TO_32BIT_VAL (ctx);
          if ((val >> 24) & ~0xFF)
            return false;

          *out++ = val >> 24;
          *out++ = (val >> 16) & 0xFF;
          *out++ = (val >> 8) & 0xFF;
          *out++ = val & 0xFF;

          *outlen += 4;

          ctx->ctx.z85.i = 0;
        }
    }
  ctx->i = ctx->ctx.z85.i;
  return true;
}


inline static bool
isubase2 (unsigned char ch)
{
  return ch == '0' || ch == '1';
}

static int
base2_length (int len)
{
  return len * 8;
}


inline static void
base2msbf_encode (char const *restrict in, idx_t inlen,
                  char *restrict out, idx_t outlen)
{
  while (inlen && outlen)
    {
      unsigned char c = *in;
      for (int i = 0; i < 8; i++)
        {
          *out++ = c & 0x80 ? '1' : '0';
          c <<= 1;
        }
      inlen--;
      outlen -= 8;
      ++in;
    }
}

inline static void
base2lsbf_encode (char const *restrict in, idx_t inlen,
                  char *restrict out, idx_t outlen)
{
  while (inlen && outlen)
    {
      unsigned char c = *in;
      for (int i = 0; i < 8; i++)
        {
          *out++ = c & 0x01 ? '1' : '0';
          c >>= 1;
        }
      inlen--;
      outlen -= 8;
      ++in;
    }
}


static void
base2_decode_ctx_init (struct base_decode_context *ctx)
{
  init_inbuf (ctx);
  ctx->ctx.base2.octet = 0;
  ctx->i = 0;
}


static bool
base2lsbf_decode_ctx (struct base_decode_context *ctx,
                      char const *restrict in, idx_t inlen,
                      char *restrict out, idx_t *outlen)
{
  bool ignore_lines = true;  /* for now, always ignore them */

  *outlen = 0;

  /* inlen==0 is request to flush output.
     if there is a dangling bit - we are missing some bits,
     so return false - indicating an invalid input.  */
  if (inlen == 0)
    return ctx->i == 0;

  while (inlen--)
    {
      if (ignore_lines && *in == '\n')
        {
          ++in;
          continue;
        }

      if (!isubase2 (*in))
        return false;

      bool bit = (*in == '1');
      ctx->ctx.base2.octet |= bit << ctx->i;
      ++ctx->i;

      if (ctx->i == 8)
        {
          *out++ = ctx->ctx.base2.octet;
          ctx->ctx.base2.octet = 0;
          ++*outlen;
          ctx->i = 0;
        }

      ++in;
    }

  return true;
}

static bool
base2msbf_decode_ctx (struct base_decode_context *ctx,
                      char const *restrict in, idx_t inlen,
                      char *restrict out, idx_t *outlen)
{
  bool ignore_lines = true;  /* for now, always ignore them */

  *outlen = 0;

  /* inlen==0 is request to flush output.
     if there is a dangling bit - we are missing some bits,
     so return false - indicating an invalid input.  */
  if (inlen == 0)
    return ctx->i == 0;

  while (inlen--)
    {
      if (ignore_lines && *in == '\n')
        {
          ++in;
          continue;
        }

      if (!isubase2 (*in))
        return false;

      bool bit = (*in == '1');
      if (ctx->i == 0)
        ctx->i = 8;
      --ctx->i;
      ctx->ctx.base2.octet |= bit << ctx->i;

      if (ctx->i == 0)
        {
          *out++ = ctx->ctx.base2.octet;
          ctx->ctx.base2.octet = 0;
          ++*outlen;
          ctx->i = 0;
        }

      ++in;
    }

  return true;
}

#endif /* BASE_TYPE == 42, i.e., "basenc"*/



static void
wrap_write (char const *buffer, idx_t len,
            idx_t wrap_column, idx_t *current_column, FILE *out)
{
  if (wrap_column == 0)
    {
      /* Simple write. */
      if (fwrite (buffer, 1, len, stdout) < len)
        write_error ();
    }
  else
    for (idx_t written = 0; written < len; )
      {
        idx_t to_write = MIN (wrap_column - *current_column, len - written);

        if (to_write == 0)
          {
            if (fputc ('\n', out) == EOF)
              write_error ();
            *current_column = 0;
          }
        else
          {
            if (fwrite (buffer + written, 1, to_write, stdout) < to_write)
              write_error ();
            *current_column += to_write;
            written += to_write;
          }
      }
}

static _Noreturn void
finish_and_exit (FILE *in, char const *infile)
{
  if (fclose (in) != 0)
    {
      if (STREQ (infile, "-"))
        error (EXIT_FAILURE, errno, _("closing standard input"));
      else
        error (EXIT_FAILURE, errno, "%s", quotef (infile));
    }

  exit (EXIT_SUCCESS);
}

static _Noreturn void
do_encode (FILE *in, char const *infile, FILE *out, idx_t wrap_column)
{
  idx_t current_column = 0;
  char *inbuf, *outbuf;
  idx_t sum;

  inbuf = xmalloc (ENC_BLOCKSIZE);
  outbuf = xmalloc (BASE_LENGTH (ENC_BLOCKSIZE));

  do
    {
      idx_t n;

      sum = 0;
      do
        {
          n = fread (inbuf + sum, 1, ENC_BLOCKSIZE - sum, in);
          sum += n;
        }
      while (!feof (in) && !ferror (in) && sum < ENC_BLOCKSIZE);

      if (sum > 0)
        {
          /* Process input one block at a time.  Note that ENC_BLOCKSIZE
             is sized so that no pad chars will appear in output. */
          base_encode (inbuf, sum, outbuf, BASE_LENGTH (sum));

          wrap_write (outbuf, BASE_LENGTH (sum), wrap_column,
                      &current_column, out);
        }
    }
  while (!feof (in) && !ferror (in) && sum == ENC_BLOCKSIZE);

  /* When wrapping, terminate last line. */
  if (wrap_column && current_column > 0 && fputc ('\n', out) == EOF)
    write_error ();

  if (ferror (in))
    error (EXIT_FAILURE, errno, _("read error"));

  finish_and_exit (in, infile);
}

static _Noreturn void
do_decode (FILE *in, char const *infile, FILE *out, bool ignore_garbage)
{
  char *inbuf, *outbuf;
  idx_t sum;
  struct base_decode_context ctx;

  char padbuf[8] = "========";
  inbuf = xmalloc (BASE_LENGTH (DEC_BLOCKSIZE));
  outbuf = xmalloc (DEC_BLOCKSIZE);

#if BASE_TYPE == 42
  ctx.inbuf = nullptr;
#endif
  base_decode_ctx_init (&ctx);

  do
    {
      bool ok;

      sum = 0;
      do
        {
          idx_t n = fread (inbuf + sum,
                           1, BASE_LENGTH (DEC_BLOCKSIZE) - sum, in);

          if (ignore_garbage)
            {
              for (idx_t i = 0; n > 0 && i < n;)
                {
                  if (isubase (inbuf[sum + i]) || inbuf[sum + i] == '=')
                    i++;
                  else
                    memmove (inbuf + sum + i, inbuf + sum + i + 1, --n - i);
                }
            }

          sum += n;

          if (ferror (in))
            error (EXIT_FAILURE, errno, _("read error"));
        }
      while (sum < BASE_LENGTH (DEC_BLOCKSIZE) && !feof (in));

      /* The following "loop" is usually iterated just once.
         However, when it processes the final input buffer, we want
         to iterate it one additional time, but with an indicator
         telling it to flush what is in CTX.  */
      for (int k = 0; k < 1 + !!feof (in); k++)
        {
          if (k == 1)
            {
              if (ctx.i == 0)
                break;

              /* auto pad input (at eof).  */
              idx_t auto_padding = REQUIRED_PADDING (ctx.i);
              if (auto_padding && (sum == 0 || inbuf[sum - 1] != '='))
                {
                  affirm (auto_padding <= sizeof (padbuf));
                  IF_LINT (free (inbuf));
                  sum = auto_padding;
                  inbuf = padbuf;
                }
              else
                sum = 0;  /* process ctx buffer only */
            }
          idx_t n = DEC_BLOCKSIZE;
          ok = base_decode_ctx (&ctx, inbuf, sum, outbuf, &n);

          if (fwrite (outbuf, 1, n, out) < n)
            write_error ();

          if (!ok)
            error (EXIT_FAILURE, 0, _("invalid input"));
        }
    }
  while (!feof (in));

  finish_and_exit (in, infile);
}

int
main (int argc, char **argv)
{
  int opt;
  FILE *input_fh;
  char const *infile;

  /* True if --decode has been given and we should decode data. */
  bool decode = false;
  /* True if we should ignore non-base-alphabetic characters. */
  bool ignore_garbage = false;
  /* Wrap encoded data around the 76th column, by default. */
  idx_t wrap_column = 76;

#if BASE_TYPE == 42
  int base_type = 0;
#endif

  initialize_main (&argc, &argv);
  set_program_name (argv[0]);
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  while ((opt = getopt_long (argc, argv, "diw:", long_options, nullptr)) != -1)
    switch (opt)
      {
      case 'd':
        decode = true;
        break;

      case 'w':
        {
          intmax_t w;
          strtol_error s_err = xstrtoimax (optarg, nullptr, 10, &w, "");
          if (LONGINT_OVERFLOW < s_err || w < 0)
            error (EXIT_FAILURE, 0, "%s: %s",
                   _("invalid wrap size"), quote (optarg));
          wrap_column = s_err == LONGINT_OVERFLOW || IDX_MAX < w ? 0 : w;
        }
        break;

      case 'i':
        ignore_garbage = true;
        break;

#if BASE_TYPE == 42
      case BASE64_OPTION:
      case BASE64URL_OPTION:
      case BASE32_OPTION:
      case BASE32HEX_OPTION:
      case BASE16_OPTION:
      case BASE2MSBF_OPTION:
      case BASE2LSBF_OPTION:
      case Z85_OPTION:
        base_type = opt;
        break;
#endif

      case_GETOPT_HELP_CHAR;

      case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);

      default:
        usage (EXIT_FAILURE);
        break;
      }

#if BASE_TYPE == 42
  switch (base_type)
    {
    case BASE64_OPTION:
      base_length = base64_length_wrapper;
      required_padding = base64_required_padding;
      isubase = isubase64;
      base_encode = base64_encode;
      base_decode_ctx_init = base64_decode_ctx_init_wrapper;
      base_decode_ctx = base64_decode_ctx_wrapper;
      break;

    case BASE64URL_OPTION:
      base_length = base64_length_wrapper;
      required_padding = base64_required_padding;
      isubase = isubase64url;
      base_encode = base64url_encode;
      base_decode_ctx_init = base64url_decode_ctx_init_wrapper;
      base_decode_ctx = base64url_decode_ctx_wrapper;
      break;

    case BASE32_OPTION:
      base_length = base32_length_wrapper;
      required_padding = base32_required_padding;
      isubase = isubase32;
      base_encode = base32_encode;
      base_decode_ctx_init = base32_decode_ctx_init_wrapper;
      base_decode_ctx = base32_decode_ctx_wrapper;
      break;

    case BASE32HEX_OPTION:
      base_length = base32_length_wrapper;
      required_padding = base32_required_padding;
      isubase = isubase32hex;
      base_encode = base32hex_encode;
      base_decode_ctx_init = base32hex_decode_ctx_init_wrapper;
      base_decode_ctx = base32hex_decode_ctx_wrapper;
      break;

    case BASE16_OPTION:
      base_length = base16_length;
      required_padding = no_required_padding;
      isubase = isubase16;
      base_encode = base16_encode;
      base_decode_ctx_init = base16_decode_ctx_init;
      base_decode_ctx = base16_decode_ctx;
      break;

    case BASE2MSBF_OPTION:
      base_length = base2_length;
      required_padding = no_required_padding;
      isubase = isubase2;
      base_encode = base2msbf_encode;
      base_decode_ctx_init = base2_decode_ctx_init;
      base_decode_ctx = base2msbf_decode_ctx;
      break;

    case BASE2LSBF_OPTION:
      base_length = base2_length;
      required_padding = no_required_padding;
      isubase = isubase2;
      base_encode = base2lsbf_encode;
      base_decode_ctx_init = base2_decode_ctx_init;
      base_decode_ctx = base2lsbf_decode_ctx;
      break;

    case Z85_OPTION:
      base_length = z85_length;
      required_padding = no_required_padding;
      isubase = isuz85;
      base_encode = z85_encode;
      base_decode_ctx_init = z85_decode_ctx_init;
      base_decode_ctx = z85_decode_ctx;
      break;

    default:
      error (0, 0, _("missing encoding type"));
      usage (EXIT_FAILURE);
    }
#endif

  if (argc - optind > 1)
    {
      error (0, 0, _("extra operand %s"), quote (argv[optind + 1]));
      usage (EXIT_FAILURE);
    }

  if (optind < argc)
    infile = argv[optind];
  else
    infile = "-";

  if (STREQ (infile, "-"))
    {
      xset_binary_mode (STDIN_FILENO, O_BINARY);
      input_fh = stdin;
    }
  else
    {
      input_fh = fopen (infile, "rb");
      if (input_fh == nullptr)
        error (EXIT_FAILURE, errno, "%s", quotef (infile));
    }

  fadvise (input_fh, FADVISE_SEQUENTIAL);

  if (decode)
    do_decode (input_fh, infile, stdout, ignore_garbage);
  else
    do_encode (input_fh, infile, stdout, wrap_column);
}
