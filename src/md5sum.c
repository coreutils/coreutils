/* Compute MD5 checksum of files or strings according to the definition
   of MD5 in RFC 1321 from April 1992.
   Copyright (C) 1995 Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* If you want to use this code in your own program as a library just
   define the preprocessor macro `USE_AS_LIBRARY'.

   cc -DUSE_AS_LIBRARY -c md5sum.c
 */

/* Written by Ulrich Drepper <drepper@gnu.ai.mit.edu>.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <getopt.h>
#include <stdio.h>
#include <sys/types.h>

#if HAVE_LIMITS_H || _LIBC
# include <limits.h>
#endif

#include "system.h"
#include "error.h"
#include "version.h"

#ifdef WORDS_BIGENDIAN
# define SWAP(n)							\
    (((n) << 24) | (((n) & 0xff00) << 8) | (((n) >> 8) & 0xff00) | ((n) >> 24))
#else
# define SWAP(n) (n)
#endif

/* For performance reasons we use low-level I/O whenever possible.  */
#if UNIX || __UNIX__ || unix || __unix__ || _POSIX_VERSION
# define FILETYPE int
# define STDINFILE STDIN_FILENO
# define OPEN open
# define OPENOPTS O_RDONLY
# define ILLFILEVAL -1
# define READ(f, b, n) read ((f), (b), (n))
# define CLOSE(f) close (f)
#else
# ifdef MSDOS
#  define TEXT1TO1 "rb"
#  define TEXTCNVT "r"
# else
#  if defined VMS
#   define TEXT1TO1 "rb", "ctx=stm"
#   define TEXTCNVT "r", "ctx=stm"
#  endif
# endif
# define FILETYPE FILE *
# define STDINFILE stdin
# define OPEN fopen
# define OPENOPTS (binary != 0 ? TEXT1TO1 : TEXTCNVT)
# define ILLFILEVAL NULL
# define READ(f, b, n) fread ((b), 1, (n), (f))
# define CLOSE(f) fclose (f)
#endif

#undef __P
#if defined __STDC__ && __STDC__
# define __P(args) args
#else
# define __P(args) ()
#endif

#ifdef __GNUC__
# define INLINE __inline
#else
# define INLINE /* empty */
#endif

/* The following contortions are an attempt to use the C preprocessor
   to determine an unsigned integral type that is 32 bits wide.  An
   alternative approach is to use autoconf's AC_CHECK_SIZEOF macro, but
   doing that would require that the configure script compile and *run*
   the resulting executable.  Locally running cross-compiled executables
   is usually not possible.  */

#if defined __STDC__ && __STDC__
# define UINT_MAX_32_BITS 4294967295U
#else
# define UINT_MAX_32_BITS 0xFFFFFFFF
#endif

/* If UINT_MAX isn't defined, assume it's a 32-bit type.
   This should be valid for all systems GNU cares about because
   that doesn't include 16-bit systems, and only modern systems
   (that certainly have <limits.h>) have 64+-bit integral types.  */

#ifndef UINT_MAX
# define UINT_MAX UINT_MAX_32_BITS
#endif

#if UINT_MAX == UINT_MAX_32_BITS
typedef unsigned int uint32;
#else
# if USHRT_MAX == UINT_MAX_32_BITS
typedef unsigned short uint32;
# else
  /* The following line is intended to throw an error.  Using #error is
     not portable enough.  */
  "Cannot determine unsigned 32-bit data type."
# endif
#endif

#define TOLOWER(c) (ISUPPER (c) ? tolower (c) : (c))

/* Hook for i18n.  */
#define _(str) str

/* Structure to save state of computation between the single steps.  */
struct md5_ctx
{
  uint32 A;
  uint32 B;
  uint32 C;
  uint32 D;
};

/* The name this program was run with.  */
char *program_name;

/* This array contains the bytes used to pad the buffer to the next
   64-byte boundary.  (RFC 1321, 3.1: Step 1)  */
static const unsigned char fillbuf[64] = { 0x80, 0 /* , 0, 0, ...  */ };

static const struct option long_options[] =
{
  { "binary", no_argument, 0, 'b' },
  { "check", required_argument, 0, 'c' },
  { "help", no_argument, 0, 'h' },
  { "string", required_argument, 0, 's' },
  { "text", no_argument, 0, 't' },
  { "verbose", no_argument, 0, 'v' },
  { "version", no_argument, 0, 'V' },
  { NULL, 0, NULL, 0 }
};

char *xmalloc ();

/* Prototypes for local functions.  */
static void usage __P ((int status));
static INLINE void init __P ((struct md5_ctx *ctx));
static INLINE void *result __P ((const struct md5_ctx *ctx, void *resbuf));
void *md5_file __P ((const char *filename, void *resblock, int binary));
void *md5_buffer __P ((const char *buffer, size_t len, void *resblock));
static void process_buffer __P ((const void *buffer, size_t len,
				 struct md5_ctx *ctx));

#ifndef USE_AS_LIBRARY

/* FIXME: but this won't work with filenames containing blanks.  */
/* FIXME: This is provisory.  Use strtok.  */

static int
split_3 (s, u, v, w)
     char *s, **u, **v, **w;
{
  size_t i;
  char *p[3];

#define ISWHITE(c) ((c) == ' ' || (c) == '\t')

  i = 0;
  while (s[i] && ISWHITE (s[i]))
    ++i;
  if (s[i])
    {
      p[0] = &s[i];
      while (s[i] && !ISWHITE (s[i]))
	++i;
      if (s[i])
	s[i++] = '\0';
      while (s[i] && ISWHITE (s[i]))
	++i;
      if (s[i])
	{
	  p[1] = &s[i];
	  while (s[i] && !ISWHITE (s[i]))
	    ++i;
	  if (s[i])
	    s[i++] = '\0';
	  while (s[i] && ISWHITE (s[i]))
	    ++i;
	  if (s[i])
	    {
	      p[2] = &s[i];
	      /* Skip past the third token.  */
	      while (s[i] && !ISWHITE (s[i]))
		++i;
	      if (s[i])
		s[i++] = '\0';
	      /* Allow trailing white space.  */
	      while (s[i] && ISWHITE (s[i]))
		++i;
	      if (!s[i])
	        {
		  *u = p[0];
		  *v = p[1];
		  *w = p[2];
		  return 0;
		}
	    }
	}
    }
  return 1;
}

/* FIXME: use strcspn.  */

static int
hex_digits (s)
     const char *s;
{
  while (*s)
    {
      if (!ISXDIGIT (*s))
        return 0;
      ++s;
    }
  return 1;
}

int
main (argc, argv)
     int argc;
     char *argv[];
{
  unsigned char md5buffer[16];
  const char *check_file = NULL;
  int binary = 1;
  int do_help = 0;
  int do_version = 0;
  int verbose = 0;
  int opt;
  char **string = NULL;
  char n_strings = 0;
  size_t i;

  /* Setting values of global variables.  */
  program_name = argv[0];

  while ((opt = getopt_long (argc, argv, "bc:hs::tvV", long_options, NULL))
	 != EOF)
    switch (opt)
      {
      case 0:			/* long option */
	break;
      case 'b':
	binary = 1;
	break;
      case 'c':
	check_file = optarg;
	break;
      case 'h':
	do_help = 1;
	break;
      case 's':
	{
	  if (string == NULL)
	    string = (char **) xmalloc ((argc - 1) * sizeof (char *));

	  if (optarg == NULL)
	    optarg = "";
	  string[n_strings++] = optarg;
	}
	break;
      case 't':
	binary = 0;
	break;
      case 'v':
	verbose = 1;
	break;
      case 'V':
	do_version = 1;
	break;
      default:
	usage (1);
      }

  if (do_version)
    {
      printf ("md5sum - %s\n", version_string);
      exit (0);
    }

  if (do_help)
    usage (0);

  if (n_strings > 0 && check_file != NULL)
    {
      error (0, 0,
	     _("the --string and --check options are mutually exclusive"));
      usage (1);
    }

  if (n_strings > 0)
    {
      if (optind < argc)
	{
	  error (0, 0, _("no files may be specified when using --string"));
	  usage (1);
	}
      for (i = 0; i < n_strings; ++i)
	{
	  size_t cnt;
	  md5_buffer (string[i], strlen (string[i]), md5buffer);

	  for (cnt = 0; cnt < 16; ++cnt)
	    printf ("%02x", md5buffer[cnt]);

	  printf (" b \"%s\"\n", string[i]);
	}
    }
  else if (check_file == NULL)
    {
      if (optind == argc)
	argv[argc++] = "-";

      for (; optind < argc; ++optind)
	{
	  size_t cnt;

	  md5_file (argv[optind], md5buffer, binary);

	  for (cnt = 0; cnt < 16; ++cnt)
	    printf ("%02x", md5buffer[cnt]);

	  printf (" %c %s\n", binary ? 'b' : 't', argv[optind]);
	}
    }
  else
    {
      FILE *cfp;
      int n_tests = 0;
      int n_tests_failed = 0;

      if (optind < argc)
	{
	  error (0, 0,
		 _("no additional files may be specified when using --check"));
	  usage (1);
	}

      if (strcmp (check_file, "-") == 0)
	cfp = stdin;
      else
	{
	  cfp = fopen (check_file, "r");
	  if (cfp == NULL)
	    error (1, errno, _("check file: %s"), check_file);
	}

      do
	{
	  char line[1024];
	  char *filename;
	  char *type_flag;
	  char *md5num;
	  int err;

	  /* FIXME: Use getline, not fgets.  */
	  if (fgets (line, 1024, cfp) == NULL)
	    break;

	  /* Remove any trailing newline.  */
	  if (line[strlen (line) - 1] == '\n')
	    line[strlen (line) - 1] = '\0';

	  /* FIXME: maybe accept the output of --string=STRING.  */
	  err = split_3 (line, &md5num, &type_flag, &filename);

	  if (err
	      || strlen (md5num) != 32
	      || !hex_digits (md5num)
	      || strlen (type_flag) != 1
	      || (*type_flag != 'b' && *type_flag != 't'))
	    {
	      if (verbose)
		error (0, 0, _("invalid line in check file: %s"), line);
	    }
	  else
	    {
	      static const char bin2hex[] = { '0', '1', '2', '3',
					      '4', '5', '6', '7',
					      '8', '9', 'a', 'b',
					      'c', 'd', 'e', 'f' };
	      size_t cnt;

	      printf ("%s: ", filename);
	      if (verbose)
		fflush (stdout);

	      ++n_tests;
	      md5_file (filename, md5buffer, *type_flag == 'b');

	      /* Compare generated binary number with text representation
		 in check file.  Ignore case of hex digits.  */
	      for (cnt = 0; cnt < 16; ++cnt)
		if (TOLOWER (md5num[2 * cnt]) != bin2hex[md5buffer[cnt] >> 4]
		    || TOLOWER (md5num[2 * cnt + 1])
		       != (bin2hex[md5buffer[cnt] & 0xf]))
		  break;

	      puts (cnt < 16 ? (++n_tests_failed, _("FAILED")) : _("OK"));
	    }
	}
      while (!feof (cfp));

      printf (_("%d out of %d tests failed\n"), n_tests_failed, n_tests);
    }

  exit (0);
}

static void
usage (status)
     int status;
{
  if (status != 0)
    fprintf (stderr, _("Try `%s --help' for more information.\n"),
	     program_name);
  else
    printf (_("\
Usage: %s [OPTION] [FILE]...\n\
  or:  %s --check=FILE\n\
  or:  %s --string=STRING\n\
Mandatory arguments to long options are mandatory for short options too.\n\
\n\
  -h, --help              display this help and exit\n\
  -v, --verbose           verbose output level\n\
  -V, --version           output version information and exit\n\
\n\
  -b, --binary            read files in binary mode (default)\n\
  -t, --text              read files in text mode\n\
\n\
  -c, --check=FILE        check MD5 sums against list in FILE\n\
  -s, --string=STRING     compute checksum for STRING\n\
\n\
The sums are computed as described in RFC 1321.  The file given at the -c\n\
option should be a former output of this program.  The default mode is to\n\
produce a list with the checksum informations.  A file name - denotes stdin.\n"),
	    program_name, program_name, program_name);

  exit (status);
}
#endif

/* Initialize structure containing state of computation.
   (RFC 1321, 3.3: Step 3)  */
static INLINE void
init (ctx)
     struct md5_ctx *ctx;
{
  ctx->A = 0x67452301;
  ctx->B = 0xefcdab89;
  ctx->C = 0x98badcfe;
  ctx->D = 0x10325476;
}

/* Put result from CTX in first 16 bytes following RESBUF.  The result must
   be in little endian byte order.  */
static INLINE void *
result (ctx, resbuf)
     const struct md5_ctx *ctx;
     void *resbuf;
{
  ((uint32 *) resbuf)[0] = SWAP (ctx->A);
  ((uint32 *) resbuf)[1] = SWAP (ctx->B);
  ((uint32 *) resbuf)[2] = SWAP (ctx->C);
  ((uint32 *) resbuf)[3] = SWAP (ctx->D);

  return resbuf;
}

/* Read file FILENAME and process it using the MD5 algorithm.  When BINARY
   is non-zero and has a "special" text file format (e.g. MSDOG) conversation
   takes place while reading.  The resulting checksum will be written into
   the 16 bytes beginning at RESBLOCK.  */
/* ARGSUSED */
void *
md5_file (filename, resblock, binary)
     const char *filename;
     void *resblock;
     int binary;
{
  /* Important: BLOCKSIZE must be a multiple of 64.  */
#define BLOCKSIZE 4096
  struct md5_ctx ctx;
  uint32 len[2];
  char buffer[BLOCKSIZE + 72];
  size_t pad, sum;
  FILETYPE f;

  /* File name - means stdin.  */
  if (strcmp (filename, "-") == 0)
    f = STDINFILE;
  else
    {
      /* OPEN and OPENOPTS are macro.  They vary according to the system
	 used.  For UN*X systems it is simply open(), but for dumb systems
	 like MSDOG it is fopen.  Of course are the reading functions
	 chosen according to the open function.  */

      f = OPEN (filename, OPENOPTS);
      if (f == ILLFILEVAL)
	error (1, errno, _("while opening input file `%s'"), filename);
    }

  /* Initialize the computation context.  */
  init (&ctx);

  len[0] = 0;
  len[1] = 0;

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
	  n = READ (f, buffer, BLOCKSIZE - sum);

	  sum += n;
	}
      while (sum < BLOCKSIZE && n != 0);

      /* RFC 1321 specifies the possible length of the file up to 2^64 bits.
	 Here we only compute the number of bytes.  Do a double word
         increment.  */
      len[0] += sum;
      if (len[0] < sum)
	++len[1];

      /* If end of file is reached, end the loop.  */
      if (n == 0)
	break;

      /* Process buffer with BLOCKSIZE bytes.  Note that
			BLOCKSIZE % 64 == 0
       */
      process_buffer (buffer, BLOCKSIZE, &ctx);
    }

  /* The complete file contents is read.  Close it now.  */
  CLOSE (f);

  /* We can copy 64 byte because the buffer is always big enough.  FILLBUF
     contains the needed bits.  */
  memcpy (&buffer[sum], fillbuf, 64);

  /* Compute amount of padding bytes needed.  Alignment is done to
		(N + PAD) % 64 == 56
     There is always at least one byte padded.  I.e. even the alignment
     is correctly aligned 64 padding bytes are added.  */
  pad = sum & 63;
  pad = pad >= 56 ? 64 + 56 - pad : 56 - pad;

  /* Put the 64-bit file length in *bits* at the end of the buffer.  */
  *(uint32 *) &buffer[sum + pad] = SWAP (len[0] << 3);
  *(uint32 *) &buffer[sum + pad + 4] = SWAP ((len[1] << 3) | (len[0] >> 29));

  /* Process last bytes.  */
  process_buffer (buffer, sum + pad + 8, &ctx);

  /* Construct result in desired memory.  */
  return result (&ctx, resblock);
}

void *
md5_buffer (buffer, len, resblock)
     const char *buffer;
     size_t len;
     void *resblock;
{
  struct md5_ctx ctx;
  char restbuf[64 + 72];
  size_t blocks = len & ~63;
  size_t pad, rest;

  /* Initialize the computation context.  */
  init (&ctx);

  /* Process whole buffer but last len % 64 bytes.  */
  process_buffer (buffer, blocks, &ctx);

  /* REST bytes are not processed yet.  */
  rest = len - blocks;
  /* Copy to own buffer.  */
  memcpy (restbuf, &buffer[blocks], rest);
  /* Append needed fill bytes at end of buffer.  We can copy 64 byte
     because the buffer is always big enough.  */
  memcpy (&restbuf[rest], fillbuf, 64);

  /* PAD bytes are used for padding to correct alignment.  Note that
     always at least one byte is padded.  */
  pad = rest >= 56 ? 64 + 56 - rest : 56 - rest;

  /* Put length of buffer in *bits* in last eight bytes.  */
  *(uint32 *) &restbuf[rest + pad] = SWAP (len << 3);
  *(uint32 *) &restbuf[rest + pad + 4] = SWAP (len >> 29);

  /* Process last bytes.  */
  process_buffer (restbuf, rest + pad + 8, &ctx);

  /* Put result in desired memory area.  */
  return result (&ctx, resblock);
}


/* These are the four functions used in the four steps of the MD5 algorithm
   and defined in the RFC 1321.  The first function is a little bit optimized
   (as found in Colin Plumbs public domain implementation).  */
/* #define FF(b, c, d) ((b & c) | (~b & d)) */
#define FF(b, c, d) (d ^ (b & (c ^ d)))
#define FG(b, c, d) FF (d, b, c)
#define FH(b, c, d) (b ^ c ^ d)
#define FI(b, c, d) (c ^ (b | ~d))

/* Process LEN bytes of BUFFER, accumulating context into CTX.
   It is assumed that LEN % 64 == 0.  */

static void
process_buffer (buffer, len, ctx)
     const void *buffer;
     size_t len;
     struct md5_ctx *ctx;
{
  uint32 correct_words[16];
  const uint32 *words = buffer;
  size_t nwords = len / sizeof (uint32);
  const uint32 *endp = words + nwords;
  uint32 A = ctx->A;
  uint32 B = ctx->B;
  uint32 C = ctx->C;
  uint32 D = ctx->D;

  /* Process all bytes in the buffer with 64 bytes in each round of
     the loop.  */
  while (words < endp)
    {
      uint32 *cwp = correct_words;
      uint32 A_save = A;
      uint32 B_save = B;
      uint32 C_save = C;
      uint32 D_save = D;

      /* First round: using the given function, the context and a constant
	 the next context is computed.  Because the algorithms processing
	 unit is a 32-bit word and it is determined to work on words in
	 little endian byte order we perhaps have to change the byte order
	 before the computation.  To reduce the work for the next steps
	 we store the swapped words in the array CORRECT_WORDS.  */

#define OP(a, b, c, d, s, T)						\
      do								\
        {								\
	  a += FF (b, c, d) + (*cwp++ = SWAP (*words)) + T;		\
	  ++words;							\
	  CYCLIC (a, s);						\
	  a += b;							\
        }								\
      while (0)

      /* It is unfortunate that C does not provide an operator for
	 cyclic rotation.  Hope the C compiler is smart enough.  */
#define CYCLIC(w, s) (w = (w << s) | (w >> (32 - s)))

      /* Before we start, one word to the strange constants.
	 They are defined in RFC 1321 as

	 T[i] = (int) (4294967296.0 * fabs (sin (i))), i=1..64
       */

      /* Round 1.  */
      OP (A, B, C, D,  7, 0xd76aa478);
      OP (D, A, B, C, 12, 0xe8c7b756);
      OP (C, D, A, B, 17, 0x242070db);
      OP (B, C, D, A, 22, 0xc1bdceee);
      OP (A, B, C, D,  7, 0xf57c0faf);
      OP (D, A, B, C, 12, 0x4787c62a);
      OP (C, D, A, B, 17, 0xa8304613);
      OP (B, C, D, A, 22, 0xfd469501);
      OP (A, B, C, D,  7, 0x698098d8);
      OP (D, A, B, C, 12, 0x8b44f7af);
      OP (C, D, A, B, 17, 0xffff5bb1);
      OP (B, C, D, A, 22, 0x895cd7be);
      OP (A, B, C, D,  7, 0x6b901122);
      OP (D, A, B, C, 12, 0xfd987193);
      OP (C, D, A, B, 17, 0xa679438e);
      OP (B, C, D, A, 22, 0x49b40821);

      /* For the second to fourth round we have the possibly swapped words
	 in CORRECT_WORDS.  Redefine the macro to take an additional first
	 argument specifying the function to use.  */
#undef OP
#define OP(f, a, b, c, d, k, s, T)					\
      do 								\
	{								\
	  a += f (b, c, d) + correct_words[k] + T;			\
	  CYCLIC (a, s);						\
	  a += b;							\
	}								\
      while (0)

      /* Round 2.  */
      OP (FG, A, B, C, D,  1,  5, 0xf61e2562);
      OP (FG, D, A, B, C,  6,  9, 0xc040b340);
      OP (FG, C, D, A, B, 11, 14, 0x265e5a51);
      OP (FG, B, C, D, A,  0, 20, 0xe9b6c7aa);
      OP (FG, A, B, C, D,  5,  5, 0xd62f105d);
      OP (FG, D, A, B, C, 10,  9, 0x02441453);
      OP (FG, C, D, A, B, 15, 14, 0xd8a1e681);
      OP (FG, B, C, D, A,  4, 20, 0xe7d3fbc8);
      OP (FG, A, B, C, D,  9,  5, 0x21e1cde6);
      OP (FG, D, A, B, C, 14,  9, 0xc33707d6);
      OP (FG, C, D, A, B,  3, 14, 0xf4d50d87);
      OP (FG, B, C, D, A,  8, 20, 0x455a14ed);
      OP (FG, A, B, C, D, 13,  5, 0xa9e3e905);
      OP (FG, D, A, B, C,  2,  9, 0xfcefa3f8);
      OP (FG, C, D, A, B,  7, 14, 0x676f02d9);
      OP (FG, B, C, D, A, 12, 20, 0x8d2a4c8a);

      /* Round 3.  */
      OP (FH, A, B, C, D,  5,  4, 0xfffa3942);
      OP (FH, D, A, B, C,  8, 11, 0x8771f681);
      OP (FH, C, D, A, B, 11, 16, 0x6d9d6122);
      OP (FH, B, C, D, A, 14, 23, 0xfde5380c);
      OP (FH, A, B, C, D,  1,  4, 0xa4beea44);
      OP (FH, D, A, B, C,  4, 11, 0x4bdecfa9);
      OP (FH, C, D, A, B,  7, 16, 0xf6bb4b60);
      OP (FH, B, C, D, A, 10, 23, 0xbebfbc70);
      OP (FH, A, B, C, D, 13,  4, 0x289b7ec6);
      OP (FH, D, A, B, C,  0, 11, 0xeaa127fa);
      OP (FH, C, D, A, B,  3, 16, 0xd4ef3085);
      OP (FH, B, C, D, A,  6, 23, 0x04881d05);
      OP (FH, A, B, C, D,  9,  4, 0xd9d4d039);
      OP (FH, D, A, B, C, 12, 11, 0xe6db99e5);
      OP (FH, C, D, A, B, 15, 16, 0x1fa27cf8);
      OP (FH, B, C, D, A,  2, 23, 0xc4ac5665);

      /* Round 4.  */
      OP (FI, A, B, C, D,  0,  6, 0xf4292244);
      OP (FI, D, A, B, C,  7, 10, 0x432aff97);
      OP (FI, C, D, A, B, 14, 15, 0xab9423a7);
      OP (FI, B, C, D, A,  5, 21, 0xfc93a039);
      OP (FI, A, B, C, D, 12,  6, 0x655b59c3);
      OP (FI, D, A, B, C,  3, 10, 0x8f0ccc92);
      OP (FI, C, D, A, B, 10, 15, 0xffeff47d);
      OP (FI, B, C, D, A,  1, 21, 0x85845dd1);
      OP (FI, A, B, C, D,  8,  6, 0x6fa87e4f);
      OP (FI, D, A, B, C, 15, 10, 0xfe2ce6e0);
      OP (FI, C, D, A, B,  6, 15, 0xa3014314);
      OP (FI, B, C, D, A, 13, 21, 0x4e0811a1);
      OP (FI, A, B, C, D,  4,  6, 0xf7537e82);
      OP (FI, D, A, B, C, 11, 10, 0xbd3af235);
      OP (FI, C, D, A, B,  2, 15, 0x2ad7d2bb);
      OP (FI, B, C, D, A,  9, 21, 0xeb86d391);

      /* Add the starting values of the context.  */
      A += A_save;
      B += B_save;
      C += C_save;
      D += D_save;
    }

  /* Put checksum in context given as argument.  */
  ctx->A = A;
  ctx->B = B;
  ctx->C = C;
  ctx->D = D;
}
