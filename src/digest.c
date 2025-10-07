/* Compute checksums of files or strings.
   Copyright (C) 1995-2025 Free Software Foundation, Inc.

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

/* Written by Ulrich Drepper <drepper@gnu.ai.mit.edu>.  */

#include <config.h>

#include <getopt.h>
#include <sys/types.h>

#include "assure.h"
#include "system.h"
#include "argmatch.h"
#include "c-ctype.h"
#include "quote.h"
#include "xdectoint.h"
#include "xstrtol.h"

#ifndef HASH_ALGO_CKSUM
# define HASH_ALGO_CKSUM 0
#endif

#if HASH_ALGO_SUM || HASH_ALGO_CKSUM
# include "sum.h"
#endif
#if HASH_ALGO_CKSUM
# include "cksum.h"
# include "base64.h"
#endif
#if HASH_ALGO_BLAKE2 || HASH_ALGO_CKSUM
# include "blake2/b2sum.h"
#endif
#if HASH_ALGO_MD5 || HASH_ALGO_CKSUM
# include "md5.h"
#endif
#if HASH_ALGO_SHA1 || HASH_ALGO_CKSUM
# include "sha1.h"
#endif
#if HASH_ALGO_SHA256 || HASH_ALGO_SHA224 || HASH_ALGO_CKSUM
# include "sha256.h"
#endif
#if HASH_ALGO_SHA512 || HASH_ALGO_SHA384 || HASH_ALGO_CKSUM
# include "sha512.h"
#endif
#if HASH_ALGO_CKSUM
# include "sha3.h"
#endif
#if HASH_ALGO_CKSUM
# include "sm3.h"
#endif
#include "fadvise.h"
#include "stdio--.h"
#include "xbinary-io.h"

/* The official name of this program (e.g., no 'g' prefix).  */
#if HASH_ALGO_SUM
# define PROGRAM_NAME "sum"
# define DIGEST_TYPE_STRING "BSD"
# define DIGEST_STREAM sumfns[sum_algorithm]
# define DIGEST_OUT sum_output_fns[sum_algorithm]
# define DIGEST_BITS 16
# define DIGEST_ALIGN 4
#elif HASH_ALGO_CKSUM
# define MAX_DIGEST_BITS 512
# define MAX_DIGEST_ALIGN 8
# define PROGRAM_NAME "cksum"
# define DIGEST_TYPE_STRING algorithm_tags[cksum_algorithm]
# define DIGEST_STREAM cksumfns[cksum_algorithm]
# define DIGEST_OUT cksum_output_fns[cksum_algorithm]
# define DIGEST_BITS MAX_DIGEST_BITS
# define DIGEST_ALIGN MAX_DIGEST_ALIGN
#elif HASH_ALGO_MD5
# define PROGRAM_NAME "md5sum"
# define DIGEST_TYPE_STRING "MD5"
# define DIGEST_STREAM md5_stream
# define DIGEST_BITS 128
# define DIGEST_REFERENCE "RFC 1321"
# define DIGEST_ALIGN 4
#elif HASH_ALGO_BLAKE2
# define PROGRAM_NAME "b2sum"
# define DIGEST_TYPE_STRING "BLAKE2b"
# define DIGEST_STREAM blake2b_stream
# define DIGEST_BITS 512
# define DIGEST_REFERENCE "RFC 7693"
# define DIGEST_ALIGN 8
#elif HASH_ALGO_SHA1
# define PROGRAM_NAME "sha1sum"
# define DIGEST_TYPE_STRING "SHA1"
# define DIGEST_STREAM sha1_stream
# define DIGEST_BITS 160
# define DIGEST_REFERENCE "FIPS-180-1"
# define DIGEST_ALIGN 4
#elif HASH_ALGO_SHA256
# define PROGRAM_NAME "sha256sum"
# define DIGEST_TYPE_STRING "SHA256"
# define DIGEST_STREAM sha256_stream
# define DIGEST_BITS 256
# define DIGEST_REFERENCE "FIPS-180-2"
# define DIGEST_ALIGN 4
#elif HASH_ALGO_SHA224
# define PROGRAM_NAME "sha224sum"
# define DIGEST_TYPE_STRING "SHA224"
# define DIGEST_STREAM sha224_stream
# define DIGEST_BITS 224
# define DIGEST_REFERENCE "RFC 3874"
# define DIGEST_ALIGN 4
#elif HASH_ALGO_SHA512
# define PROGRAM_NAME "sha512sum"
# define DIGEST_TYPE_STRING "SHA512"
# define DIGEST_STREAM sha512_stream
# define DIGEST_BITS 512
# define DIGEST_REFERENCE "FIPS-180-2"
# define DIGEST_ALIGN 8
#elif HASH_ALGO_SHA384
# define PROGRAM_NAME "sha384sum"
# define DIGEST_TYPE_STRING "SHA384"
# define DIGEST_STREAM sha384_stream
# define DIGEST_BITS 384
# define DIGEST_REFERENCE "FIPS-180-2"
# define DIGEST_ALIGN 8
#else
# error "Can't decide which hash algorithm to compile."
#endif
#if !HASH_ALGO_SUM && !HASH_ALGO_CKSUM
# define DIGEST_OUT output_file
#endif

#if HASH_ALGO_SUM
# define AUTHORS \
  proper_name ("Kayvan Aghaiepour"), \
  proper_name ("David MacKenzie")
#elif HASH_ALGO_CKSUM
# define AUTHORS \
  proper_name_lite ("Padraig Brady", "P\303\241draig Brady"), \
  proper_name ("Q. Frank Xia")
#elif HASH_ALGO_BLAKE2
# define AUTHORS \
  proper_name_lite ("Padraig Brady", "P\303\241draig Brady"), \
  proper_name ("Samuel Neves")
#else
# define AUTHORS \
  proper_name ("Ulrich Drepper"), \
  proper_name ("Scott Miller"), \
  proper_name ("David Madore")
#endif
#if !HASH_ALGO_BLAKE2 && !HASH_ALGO_CKSUM
# define DIGEST_HEX_BYTES (DIGEST_BITS / 4)
#endif
#define DIGEST_BIN_BYTES (DIGEST_BITS / 8)

/* The minimum length of a valid digest line.  This length does
   not include any newline character at the end of a line.  */
#if HASH_ALGO_BLAKE2 || HASH_ALGO_CKSUM
# define MIN_DIGEST_LINE_LENGTH 3 /* With -l 8.  */
#else
# define MIN_DIGEST_LINE_LENGTH \
   (DIGEST_HEX_BYTES /* length of hexadecimal message digest */ \
    + 1 /* blank */ \
    + 1 /* minimum filename length */ )
#endif

#if !HASH_ALGO_SUM
static void
output_file (char const *file, int binary_file, void const *digest,
             bool raw, bool tagged, unsigned char delim, bool args,
             uintmax_t length);
#endif

/* True if any of the files read were the standard input. */
static bool have_read_stdin;

/* The minimum length of a valid checksum line for the selected algorithm.  */
static size_t min_digest_line_length;

/* Set to the length of a digest hex string for the selected algorithm.  */
static size_t digest_hex_bytes;

/* With --check, don't generate any output.
   The exit code indicates success or failure.  */
static bool status_only = false;

/* With --check, print a message to standard error warning about each
   improperly formatted checksum line.  */
static bool warn = false;

/* With --check, ignore missing files.  */
static bool ignore_missing = false;

/* With --check, suppress the "OK" printed for each verified file.  */
static bool quiet = false;

/* With --check, exit with a non-zero return code if any line is
   improperly formatted. */
static bool strict = false;

/* Whether a BSD reversed format checksum is detected.  */
static int bsd_reversed = -1;

/* line delimiter.  */
static unsigned char digest_delim = '\n';

#if HASH_ALGO_CKSUM
/* If true, print base64-encoded digests, not hex.  */
static bool base64_digest = false;
#endif

/* If true, print binary digests, not hex.  */
static bool raw_digest = false;

/* blake2b and sha3 allow the -l option.  Luckily they both have the same
   maximum digest size.  */
#if HASH_ALGO_BLAKE2 || HASH_ALGO_CKSUM
# if HASH_ALGO_CKSUM
static_assert (BLAKE2B_OUTBYTES == SHA3_512_DIGEST_SIZE);
# endif
# define DIGEST_MAX_LEN BLAKE2B_OUTBYTES
static uintmax_t digest_length;
#endif /* HASH_ALGO_BLAKE2 || HASH_ALGO_CKSUM */

typedef void (*digest_output_fn)(char const *, int, void const *, bool,
                                 bool, unsigned char, bool, uintmax_t);
#if HASH_ALGO_SUM
enum Algorithm
{
  bsd,
  sysv,
};

static enum Algorithm sum_algorithm;
static sumfn sumfns[]=
{
  bsd_sum_stream,
  sysv_sum_stream,
};
static digest_output_fn sum_output_fns[]=
{
  output_bsd,
  output_sysv,
};
#endif

#if HASH_ALGO_CKSUM
static int
md5_sum_stream (FILE *stream, void *resstream, MAYBE_UNUSED uintmax_t *length)
{
  return md5_stream (stream, resstream);
}
static int
sha1_sum_stream (FILE *stream, void *resstream, MAYBE_UNUSED uintmax_t *length)
{
  return sha1_stream (stream, resstream);
}
static int
sha224_sum_stream (FILE *stream, void *resstream,
                   MAYBE_UNUSED uintmax_t *length)
{
  return sha224_stream (stream, resstream);
}
static int
sha256_sum_stream (FILE *stream, void *resstream,
                   MAYBE_UNUSED uintmax_t *length)
{
  return sha256_stream (stream, resstream);
}
static int
sha384_sum_stream (FILE *stream, void *resstream,
                   MAYBE_UNUSED uintmax_t *length)
{
  return sha384_stream (stream, resstream);
}
static int
sha512_sum_stream (FILE *stream, void *resstream,
                   MAYBE_UNUSED uintmax_t *length)
{
  return sha512_stream (stream, resstream);
}
static int
sha2_sum_stream (FILE *stream, void *resstream, uintmax_t *length)
{
  switch (*length)
    {
    case SHA224_DIGEST_SIZE:
      return sha224_stream (stream, resstream);
    case SHA256_DIGEST_SIZE:
      return sha256_stream (stream, resstream);
    case SHA384_DIGEST_SIZE:
      return sha384_stream (stream, resstream);
    case SHA512_DIGEST_SIZE:
      return sha512_stream (stream, resstream);
    default:
      affirm (false);
    }
}
static int
sha3_sum_stream (FILE *stream, void *resstream, uintmax_t *length)
{
  switch (*length)
    {
    case SHA3_224_DIGEST_SIZE:
      return sha3_224_stream (stream, resstream);
    case SHA3_256_DIGEST_SIZE:
      return sha3_256_stream (stream, resstream);
    case SHA3_384_DIGEST_SIZE:
      return sha3_384_stream (stream, resstream);
    case SHA3_512_DIGEST_SIZE:
      return sha3_512_stream (stream, resstream);
    default:
      affirm (false);
    }
}
static int
blake2b_sum_stream (FILE *stream, void *resstream, uintmax_t *length)
{
  return blake2b_stream (stream, resstream, *length);
}
static int
sm3_sum_stream (FILE *stream, void *resstream, MAYBE_UNUSED uintmax_t *length)
{
  return sm3_stream (stream, resstream);
}

enum Algorithm
{
  bsd,
  sysv,
  crc,
  crc32b,
  md5,
  sha1,
  sha224,
  sha256,
  sha384,
  sha512,
  sha2,
  sha3,
  blake2b,
  sm3,
};

static char const *const algorithm_args[] =
{
  "bsd", "sysv", "crc", "crc32b", "md5", "sha1",
  "sha224", "sha256", "sha384", "sha512", /* Legacy naming */
  "sha2", "sha3", "blake2b", "sm3", nullptr
};
static enum Algorithm const algorithm_types[] =
{
  bsd, sysv, crc, crc32b, md5, sha1,
  sha224, sha256, sha384, sha512,
  sha2, sha3, blake2b, sm3,
};
ARGMATCH_VERIFY (algorithm_args, algorithm_types);

static char const *const algorithm_tags[] =
{
  "BSD", "SYSV", "CRC", "CRC32B", "MD5", "SHA1",
  "SHA224", "SHA256", "SHA384", "SHA512",
  "SHA2", "SHA3", "BLAKE2b", "SM3", nullptr
};
static int const algorithm_bits[] =
{
  16, 16, 32, 32, 128, 160,
  224, 256, 384, 512,
  512, 512, 512, 256, 0
};

static_assert (countof (algorithm_bits) == countof (algorithm_args));

static bool algorithm_specified = false;
static enum Algorithm cksum_algorithm = crc;
static sumfn cksumfns[]=
{
  bsd_sum_stream,
  sysv_sum_stream,
  crc_sum_stream,
  crc32b_sum_stream,
  md5_sum_stream,
  sha1_sum_stream,
  sha224_sum_stream,
  sha256_sum_stream,
  sha384_sum_stream,
  sha512_sum_stream,
  sha2_sum_stream,
  sha3_sum_stream,
  blake2b_sum_stream,
  sm3_sum_stream,
};
static digest_output_fn cksum_output_fns[]=
{
  output_bsd,
  output_sysv,
  output_crc,
  output_crc,
  output_file,
  output_file,
  output_file,
  output_file,
  output_file,
  output_file,
  output_file,
  output_file,
  output_file,
  output_file,
};
bool cksum_debug;
#endif

/* For long options that have no equivalent short option, use a
   non-character as a pseudo short option, starting with CHAR_MAX + 1.  */

enum
{
  IGNORE_MISSING_OPTION = CHAR_MAX + 1,
  STATUS_OPTION,
  QUIET_OPTION,
  STRICT_OPTION,
  TAG_OPTION,
  UNTAG_OPTION,
  DEBUG_PROGRAM_OPTION,
  RAW_OPTION,
  BASE64_OPTION,
};

static struct option const long_options[] =
{
#if HASH_ALGO_BLAKE2 || HASH_ALGO_CKSUM
  { "length", required_argument, nullptr, 'l'},
#endif

#if !HASH_ALGO_SUM
  { "check", no_argument, nullptr, 'c' },
  { "ignore-missing", no_argument, nullptr, IGNORE_MISSING_OPTION},
  { "quiet", no_argument, nullptr, QUIET_OPTION },
  { "status", no_argument, nullptr, STATUS_OPTION },
  { "warn", no_argument, nullptr, 'w' },
  { "strict", no_argument, nullptr, STRICT_OPTION },
  { "tag", no_argument, nullptr, TAG_OPTION },
  { "zero", no_argument, nullptr, 'z' },

# if HASH_ALGO_CKSUM
  { "algorithm", required_argument, nullptr, 'a'},
  { "base64", no_argument, nullptr, BASE64_OPTION },
  { "debug", no_argument, nullptr, DEBUG_PROGRAM_OPTION},
  { "raw", no_argument, nullptr, RAW_OPTION},
  { "untagged", no_argument, nullptr, UNTAG_OPTION },
# endif
  { "binary", no_argument, nullptr, 'b' },
  { "text", no_argument, nullptr, 't' },

#else
  {"sysv", no_argument, nullptr, 's'},
#endif

  { GETOPT_HELP_OPTION_DECL },
  { GETOPT_VERSION_OPTION_DECL },
  { nullptr, 0, nullptr, 0 }
};

void
usage (int status)
{
  if (status != EXIT_SUCCESS)
    emit_try_help ();
  else
    {
      printf (_("\
Usage: %s [OPTION]... [FILE]...\n\
"), program_name);
#if HASH_ALGO_CKSUM
      fputs (_("\
Print or verify checksums.\n\
By default use the 32 bit CRC algorithm.\n\
"), stdout);
#else
      printf (_("\
Print or check %s (%d-bit) checksums.\n\
"),
              DIGEST_TYPE_STRING,
              DIGEST_BITS);
#endif

      emit_stdin_note ();
#if HASH_ALGO_SUM
      fputs (_("\
\n\
  -r              use BSD sum algorithm (the default), use 1K blocks\n\
  -s, --sysv      use System V sum algorithm, use 512 bytes blocks\n\
"), stdout);
#endif
#if HASH_ALGO_BLAKE2 || HASH_ALGO_CKSUM
        emit_mandatory_arg_note ();
#endif
#if HASH_ALGO_CKSUM
        fputs (_("\
  -a, --algorithm=TYPE  select the digest type to use.  See DIGEST below\
\n\
"), stdout);
        fputs (_("\
      --base64          emit base64-encoded digests, not hexadecimal\
\n\
"), stdout);
#endif
#if !HASH_ALGO_SUM
# if !HASH_ALGO_CKSUM
      if (O_BINARY)
        fputs (_("\
  -b, --binary          read in binary mode (default unless reading tty stdin)\
\n\
"), stdout);
      else
        fputs (_("\
  -b, --binary          read in binary mode\n\
"), stdout);
# endif
        fputs (_("\
  -c, --check           read checksums from the FILEs and check them\n\
"), stdout);
# if HASH_ALGO_BLAKE2 || HASH_ALGO_CKSUM
        fputs (_("\
  -l, --length=BITS     digest length in bits; must not exceed the max size\n\
                          and must be a multiple of 8 for blake2b;\n\
                          must be 224, 256, 384, or 512 for sha2 or sha3\n\
"), stdout);
# endif
# if HASH_ALGO_CKSUM
        fputs (_("\
      --raw             emit a raw binary digest, not hexadecimal\
\n\
"), stdout);
      fputs (_("\
      --tag             create a BSD-style checksum (the default)\n\
"), stdout);
      fputs (_("\
      --untagged        create a reversed style checksum, without digest type\n\
"), stdout);
# else
      fputs (_("\
      --tag             create a BSD-style checksum\n\
"), stdout);
# endif
# if !HASH_ALGO_CKSUM
      if (O_BINARY)
        fputs (_("\
  -t, --text            read in text mode (default if reading tty stdin)\n\
"), stdout);
      else
        fputs (_("\
  -t, --text            read in text mode (default)\n\
"), stdout);
# endif
      fputs (_("\
  -z, --zero            end each output line with NUL, not newline,\n\
                          and disable file name escaping\n\
"), stdout);
      fputs (_("\
\n\
The following five options are useful only when verifying checksums:\n\
      --ignore-missing  don't fail or report status for missing files\n\
      --quiet           don't print OK for each successfully verified file\n\
      --status          don't output anything, status code shows success\n\
      --strict          exit non-zero for improperly formatted checksum lines\n\
  -w, --warn            warn about improperly formatted checksum lines\n\
\n\
"), stdout);
#endif
#if HASH_ALGO_CKSUM
      fputs (_("\
      --debug           indicate which implementation used\n\
"), stdout);
#endif
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
#if HASH_ALGO_CKSUM
      fputs (_("\
\n\
DIGEST determines the digest algorithm and default output format:\n\
  sysv      (equivalent to sum -s)\n\
  bsd       (equivalent to sum -r)\n\
  crc       (equivalent to cksum)\n\
  crc32b    (only available through cksum)\n\
  md5       (equivalent to md5sum)\n\
  sha1      (equivalent to sha1sum)\n\
  sha2      (equivalent to sha{224,256,384,512}sum)\n\
  sha3      (only available through cksum)\n\
  blake2b   (equivalent to b2sum)\n\
  sm3       (only available through cksum)\n\
\n"), stdout);
#endif
#if !HASH_ALGO_SUM && !HASH_ALGO_CKSUM
      printf (_("\
\n\
The sums are computed as described in %s.\n"), DIGEST_REFERENCE);
      fputs (_("\
When checking, the input should be a former output of this program.\n\
The default mode is to print a line with: checksum, a space,\n\
a character indicating input mode ('*' for binary, ' ' for text\n\
or where binary is insignificant), and name for each FILE.\n\
\n\
There is no difference between binary mode and text mode on GNU systems.\
\n"), stdout);
#endif
#if HASH_ALGO_CKSUM
      fputs (_("\
When checking, the input should be a former output of this program,\n\
or equivalent standalone program.\
\n"), stdout);
#endif
      emit_ancillary_info (PROGRAM_NAME);
    }

  exit (status);
}

/* Given a string S, return TRUE if it contains problematic characters
   that need escaping.  Note we escape '\' itself to provide some forward
   compat to introduce escaping of other characters.  */

ATTRIBUTE_PURE
static bool
problematic_chars (char const *s)
{
  size_t length = strcspn (s, "\\\n\r");
  return s[length] != '\0';
}

#define ISWHITE(c) ((c) == ' ' || (c) == '\t')

/* Given a file name, S of length S_LEN, that is not NUL-terminated,
   modify it in place, performing the equivalent of this sed substitution:
   's/\\n/\n/g;s/\\r/\r/g;s/\\\\/\\/g' i.e., replacing each "\\n" string
   with a newline, each "\\r" string with a carriage return,
   and each "\\\\" with a single backslash, NUL-terminate it and return S.
   If S is not a valid escaped file name, i.e., if it ends with an odd number
   of backslashes or if it contains a backslash followed by anything other
   than "n" or another backslash, return nullptr.  */

static char *
filename_unescape (char *s, size_t s_len)
{
  char *dst = s;

  for (size_t i = 0; i < s_len; i++)
    {
      switch (s[i])
        {
        case '\\':
          if (i == s_len - 1)
            {
              /* File name ends with an unescaped backslash: invalid.  */
              return nullptr;
            }
          ++i;
          switch (s[i])
            {
            case 'n':
              *dst++ = '\n';
              break;
            case 'r':
              *dst++ = '\r';
              break;
            case '\\':
              *dst++ = '\\';
              break;
            default:
              /* Only '\', 'n' or 'r' may follow a backslash.  */
              return nullptr;
            }
          break;

        case '\0':
          /* The file name may not contain a NUL.  */
          return nullptr;

        default:
          *dst++ = s[i];
          break;
        }
    }
  if (dst < s + s_len)
    *dst = '\0';

  return s;
}

/* Return true if S is a LEN-byte NUL-terminated string of hex or base64
   digits and has the expected length.  Otherwise, return false.  */
ATTRIBUTE_PURE
static bool
valid_digits (unsigned char const *s, size_t len)
{
#if HASH_ALGO_CKSUM
  if (len == BASE64_LENGTH (digest_length / 8))
    {
      size_t i;
      for (i = 0; i < len - digest_length % 3; i++)
        {
          if (!isbase64 (*s))
            return false;
          ++s;
        }
      for ( ; i < len; i++)
        {
          if (*s != '=')
            return false;
          ++s;
        }
    }
  else
#endif
  if (len == digest_hex_bytes)
    {
      for (idx_t i = 0; i < digest_hex_bytes; i++)
        {
          if (!c_isxdigit (*s))
            return false;
          ++s;
        }
    }
  else
    return false;

  return *s == '\0';
}

/* Split the checksum string S (of length S_LEN) from a BSD 'md5' or
   'sha1' command into two parts: a hexadecimal digest, and the file
   name.  S is modified.  Set *D_LEN to the length of the digest string.
   Return true if successful.  */

static bool
bsd_split_3 (char *s, size_t s_len,
             unsigned char **digest, size_t *d_len,
             char **file_name, bool escaped_filename)
{
  if (s_len == 0)
    return false;

  /* Find end of filename.  */
  size_t i = s_len - 1;
  while (i && s[i] != ')')
    i--;

  if (s[i] != ')')
    return false;

  *file_name = s;

  if (escaped_filename && filename_unescape (s, i) == nullptr)
    return false;

  s[i++] = '\0';

  while (ISWHITE (s[i]))
    i++;

  if (s[i] != '=')
    return false;

  i++;

  while (ISWHITE (s[i]))
    i++;

  *digest = (unsigned char *) &s[i];

  *d_len = s_len - i;
  return valid_digits (*digest, *d_len);
}

#if HASH_ALGO_CKSUM
/* Return the corresponding Algorithm for the string S,
   or -1 for no match.  */

static ptrdiff_t
algorithm_from_tag (char *s)
{
  /* Limit check size to this length for perf reasons.  */
  static size_t max_tag_len;
  if (! max_tag_len)
    {
      char const * const * tag = algorithm_tags;
      while (*tag)
        {
          size_t tag_len = strlen (*tag++);
          max_tag_len = MAX (tag_len, max_tag_len);
        }
    }

  size_t i = 0;

  /* Find end of tag */
  while (i <= max_tag_len && s[i] && ! ISWHITE (s[i])
         && s[i] != '-' && s[i] != '(')
    ++i;

  if (i > max_tag_len)
    return -1;

  /* Terminate tag, and lookup.  */
  char sep = s[i];
  s[i] = '\0';
  ptrdiff_t algo = argmatch_exact (s, algorithm_tags);
  s[i] = sep;

  return algo;
}
#endif

/* Split the string S (of length S_LEN) into three parts:
   a hexadecimal digest, binary flag, and the file name.
   S is modified.  Set *D_LEN to the length of the digest string.
   Return true if successful.  */

static bool
split_3 (char *s, size_t s_len,
         unsigned char **digest, size_t *d_len, int *binary, char **file_name)
{
  bool escaped_filename = false;
  size_t algo_name_len;

  size_t i = 0;
  while (ISWHITE (s[i]))
    ++i;

  if (s[i] == '\\')
    {
      ++i;
      escaped_filename = true;
    }

  /* Check for BSD-style checksum line. */

#if HASH_ALGO_CKSUM
  if (! algorithm_specified || cksum_algorithm == sha2)
    {
      ptrdiff_t algo_tag = algorithm_from_tag (s + i);
      if (! algorithm_specified)
        {
          if (algo_tag >= 0)
            {
              if (algo_tag <= crc32b)
                return false;  /* We don't support checking these formats.  */
              cksum_algorithm = algo_tag;
            }
          else
            return false;      /* We only support tagged format without -a.  */
        }
      else
        {
          if (cksum_algorithm == sha2 && (algo_tag == sha2
              || algo_tag == sha224 || algo_tag == sha256
              || algo_tag == sha384 || algo_tag == sha512))
            cksum_algorithm = algo_tag;
        }
    }
#endif

  /* Try to parse BSD or OpenSSL tagged format.  I.e.:
     openssl: MD5(f)= d41d8cd98f00b204e9800998ecf8427e
     bsd:     MD5 (f) = d41d8cd98f00b204e9800998ecf8427e  */

  size_t parse_offset = i;
  algo_name_len = strlen (DIGEST_TYPE_STRING);
  if (STREQ_LEN (s + i, DIGEST_TYPE_STRING, algo_name_len))
    {
      i += algo_name_len;
#if HASH_ALGO_BLAKE2 || HASH_ALGO_CKSUM

# if HASH_ALGO_BLAKE2
      digest_length = DIGEST_MAX_LEN * 8;
# else
      digest_length = algorithm_bits[cksum_algorithm];
# endif
      if (s[i] == '-')  /* length specified. Not base64 */
        {
          ++i;
          uintmax_t length;
          char *siend;
          if (xstrtoumax (s + i, &siend, 0, &length, nullptr) != LONGINT_OK)
            return false;
# if HASH_ALGO_CKSUM
          else if (cksum_algorithm == sha2 || cksum_algorithm == sha3)
            {
              if (length != SHA224_DIGEST_SIZE * 8
                  && length != SHA256_DIGEST_SIZE * 8
                  && length != SHA384_DIGEST_SIZE * 8
                  && length != SHA512_DIGEST_SIZE * 8)
                return false;
            }
# endif
          else if (!(0 < length && length <= digest_length && length % 8 == 0))
            return false;

          i = siend - s;
          digest_length = length;
        }
      digest_hex_bytes = digest_length / 4;
#endif
      if (s[i] == ' ')
        ++i;
      if (s[i] == '(')  /* not base64 */
        {
          ++i;
          *binary = 0;
          return bsd_split_3 (s + i, s_len - i,
                              digest, d_len, file_name, escaped_filename);
        }

      /* Note with --base64 --untagged format, we may have matched a "tag".
         Even very short digests with: cksum -a blake2b -l24 --untagged --base64
         So fallback to checking untagged format if issues detecting tags.  */
      i = parse_offset;
    }

  /* Ignore this line if it is too short.
     Each line must have at least 'min_digest_line_length - 1' (or one more, if
     the first is a backslash) more characters to contain correct message digest
     information.  */
  if (s_len - i < min_digest_line_length + (s[i] == '\\'))
    return false;

  *digest = (unsigned char *) &s[i];

#if HASH_ALGO_BLAKE2 || HASH_ALGO_CKSUM
  /* Auto determine length.  */
# if HASH_ALGO_CKSUM
  if (cksum_algorithm == blake2b
      || cksum_algorithm == sha2 || cksum_algorithm == sha3) {
# endif
  unsigned char const *hp = *digest;
  digest_hex_bytes = 0;
  for (; c_isxdigit (*hp); ++hp, ++digest_hex_bytes)
    ;
# if HASH_ALGO_CKSUM
  /* Check the number of base64 characters.  This works because the hexadecimal
     character set is a subset of the base64 character set.
     Note there is the ambiguity that all characters are hex when they
     are actually base64 encoded, which could be ambiguous with:
        cksum -a sha2 -l 384 --base64 --untagged
        cksum -a sha2 -l 256 --untagged
     Similarly for sha3 and blake2b.
     However at this length the chances are exceedingly rare (1 in 480R),
     and smaller blake2b lengths aren't practical for verification anyway.  */
  size_t digest_base64_bytes = digest_hex_bytes;
  size_t trailing_equals = 0;
  for (; isubase64 (*hp); ++hp, ++digest_base64_bytes)
    ;
  for (; *hp == '='; ++hp, ++trailing_equals)
    ;
  if ((cksum_algorithm == sha2 || cksum_algorithm == sha3)
      && digest_hex_bytes / 2 != SHA224_DIGEST_SIZE
      && digest_hex_bytes / 2 != SHA256_DIGEST_SIZE
      && digest_hex_bytes / 2 != SHA384_DIGEST_SIZE
      && digest_hex_bytes / 2 != SHA512_DIGEST_SIZE)
    {
      if (digest_base64_bytes + trailing_equals
          == BASE64_LENGTH (SHA224_DIGEST_SIZE))
        digest_hex_bytes = SHA224_DIGEST_SIZE * 2;
      else if (digest_base64_bytes + trailing_equals
               == BASE64_LENGTH (SHA256_DIGEST_SIZE))
        digest_hex_bytes = SHA256_DIGEST_SIZE * 2;
      else if (digest_base64_bytes + trailing_equals
               == BASE64_LENGTH (SHA384_DIGEST_SIZE))
        digest_hex_bytes = SHA384_DIGEST_SIZE * 2;
      else if (digest_base64_bytes + trailing_equals
               == BASE64_LENGTH (SHA512_DIGEST_SIZE))
        digest_hex_bytes = SHA512_DIGEST_SIZE * 2;
      else
        return false;
    }
  else if (cksum_algorithm == blake2b
           && digest_hex_bytes < digest_base64_bytes)
    {
      for (int j = 8; j <= DIGEST_MAX_LEN * 8; j += 8)
        {
          if (BASE64_LENGTH (j / 8) == digest_base64_bytes + trailing_equals
              && j % 3 == trailing_equals)
            {
              digest_hex_bytes = j / 4;
              break;
            }
        }
    }
# endif
  if (digest_hex_bytes < 2 || digest_hex_bytes % 2
      || DIGEST_MAX_LEN * 2 < digest_hex_bytes)
    return false;
  digest_length = digest_hex_bytes * 4;
# if HASH_ALGO_CKSUM
  }
# endif
#endif

  /* This field must be the hexadecimal or base64 representation
     of the message digest.  */
  while (s[i] && !ISWHITE (s[i]))
    i++;

  /* The digest must be followed by at least one whitespace character.  */
  if (i == s_len)
    return false;

  *d_len = &s[i] - (char *) *digest;
  s[i++] = '\0';

  if (! valid_digits (*digest, *d_len))
    return false;

  /* If "bsd reversed" format detected.  */
  if ((s_len - i == 1) || (s[i] != ' ' && s[i] != '*'))
    {
      /* Don't allow mixing bsd and standard formats,
         to minimize security issues with attackers
         renaming files with leading spaces.
         This assumes that with bsd format checksums
         that the first file name does not have
         a leading ' ' or '*'.  */
      if (bsd_reversed == 0)
        return false;
      bsd_reversed = 1;
    }
  else if (bsd_reversed != 1)
    {
      bsd_reversed = 0;
      *binary = (s[i++] == '*');
    }

  /* All characters between the type indicator and end of line are
     significant -- that includes leading and trailing white space.  */
  *file_name = &s[i];

  if (escaped_filename)
    return filename_unescape (&s[i], s_len - i) != nullptr;

  return true;
}

/* If ESCAPE is true, then translate each:
   NEWLINE byte to the string, "\\n",
   CARRIAGE RETURN byte to the string, "\\r",
   and each backslash to "\\\\".  */
static void
print_filename (char const *file, bool escape)
{
  if (! escape)
    {
      fputs (file, stdout);
      return;
    }

  while (*file)
    {
      switch (*file)
        {
        case '\n':
          fputs ("\\n", stdout);
          break;

        case '\r':
          fputs ("\\r", stdout);
          break;

        case '\\':
          fputs ("\\\\", stdout);
          break;

        default:
          putchar (*file);
          break;
        }
      file++;
    }
}

/* An interface to the function, DIGEST_STREAM.
   Operate on FILENAME (it may be "-").

   *BINARY indicates whether the file is binary.  BINARY < 0 means it
   depends on whether binary mode makes any difference and the file is
   a terminal; in that case, clear *BINARY if the file was treated as
   text because it was a terminal.

   Put the checksum in *BIN_RESULT, which must be properly aligned.
   Put true in *MISSING if the file can't be opened due to ENOENT.
   Return true if successful.  */

static bool
digest_file (char const *filename, int *binary, unsigned char *bin_result,
             bool *missing, MAYBE_UNUSED uintmax_t *length)
{
  FILE *fp;
  int err;
  bool is_stdin = streq (filename, "-");

  *missing = false;

  if (is_stdin)
    {
      have_read_stdin = true;
      fp = stdin;
      if (O_BINARY && *binary)
        {
          if (*binary < 0)
            *binary = ! isatty (STDIN_FILENO);
          if (*binary)
            xset_binary_mode (STDIN_FILENO, O_BINARY);
        }
    }
  else
    {
      fp = fopen (filename, (O_BINARY && *binary ? "rb" : "r"));
      if (fp == nullptr)
        {
          if (ignore_missing && errno == ENOENT)
            {
              *missing = true;
              return true;
            }
          error (0, errno, "%s", quotef (filename));
          return false;
        }
    }

  fadvise (fp, FADVISE_SEQUENTIAL);

#if HASH_ALGO_CKSUM
  if (cksum_algorithm == blake2b
      || cksum_algorithm == sha2 || cksum_algorithm == sha3)
    *length = digest_length / 8;
  err = DIGEST_STREAM (fp, bin_result, length);
#elif HASH_ALGO_SUM
  err = DIGEST_STREAM (fp, bin_result, length);
#elif HASH_ALGO_BLAKE2
  err = DIGEST_STREAM (fp, bin_result, digest_length / 8);
#else
  err = DIGEST_STREAM (fp, bin_result);
#endif
  err = err ? errno : 0;
  if (is_stdin)
    clearerr (fp);
  else if (fclose (fp) != 0 && !err)
    err = errno;

  if (err)
    {
      error (0, err, "%s", quotef (filename));
      return false;
    }

  return true;
}

#if !HASH_ALGO_SUM
static void
output_file (char const *file, int binary_file, void const *digest,
             MAYBE_UNUSED bool raw, bool tagged, unsigned char delim,
             MAYBE_UNUSED bool args, MAYBE_UNUSED uintmax_t length)
{
# if HASH_ALGO_CKSUM
  if (raw)
    {
      fwrite (digest, 1, digest_length / 8, stdout);
      return;
    }
# endif

  unsigned char const *bin_buffer = digest;

  /* Output a leading backslash if the file name contains problematic chars.  */
  bool needs_escape = delim == '\n' && problematic_chars (file);

  if (needs_escape)
    putchar ('\\');

  if (tagged)
    {
# if HASH_ALGO_CKSUM
      if (cksum_algorithm == sha2)
        printf ("SHA%ju", digest_length);
      else
# endif
      fputs (DIGEST_TYPE_STRING, stdout);
# if HASH_ALGO_BLAKE2
      if (digest_length < DIGEST_MAX_LEN * 8)
        printf ("-%ju", digest_length);
# elif HASH_ALGO_CKSUM
      if (cksum_algorithm == sha3)
        printf ("-%ju", digest_length);
      if (cksum_algorithm == blake2b)
        {
          if (digest_length < DIGEST_MAX_LEN * 8)
            printf ("-%ju", digest_length);
        }
# endif
      fputs (" (", stdout);
      print_filename (file, needs_escape);
      fputs (") = ", stdout);
    }

# if HASH_ALGO_CKSUM
  if (base64_digest)
    {
      char b64[BASE64_LENGTH (DIGEST_BIN_BYTES) + 1];
      base64_encode ((char const *) bin_buffer, digest_length / 8,
                     b64, sizeof b64);
      fputs (b64, stdout);
    }
  else
# endif
    {
      for (size_t i = 0; i < (digest_hex_bytes / 2); ++i)
        printf ("%02x", bin_buffer[i]);
    }

  if (!tagged)
    {
      putchar (' ');
      putchar (binary_file ? '*' : ' ');
      print_filename (file, needs_escape);
    }

  putchar (delim);
}
#endif

#if HASH_ALGO_CKSUM
/* Return true if B64_DIGEST is the same as the base64 digest of the
   DIGEST_LENGTH/8 bytes at BIN_BUFFER.  */
static bool
b64_equal (unsigned char const *b64_digest, unsigned char const *bin_buffer)
{
  size_t b64_n_bytes = BASE64_LENGTH (digest_length / 8);
  char b64[BASE64_LENGTH (DIGEST_BIN_BYTES) + 1];
  base64_encode ((char const *) bin_buffer, digest_length / 8, b64, sizeof b64);
  return memeq (b64_digest, b64, b64_n_bytes + 1);
}
#endif

/* Return true if HEX_DIGEST is the same as the hex-encoded digest of the
   DIGEST_LENGTH/8 bytes at BIN_BUFFER.  */
static bool
hex_equal (unsigned char const *hex_digest, unsigned char const *bin_buffer)
{
  static const char bin2hex[] = { '0', '1', '2', '3',
                                  '4', '5', '6', '7',
                                  '8', '9', 'a', 'b',
                                  'c', 'd', 'e', 'f' };
  size_t digest_bin_bytes = digest_hex_bytes / 2;

  /* Compare generated binary number with text representation
     in check file.  Ignore case of hex digits.  */
  size_t cnt;
  for (cnt = 0; cnt < digest_bin_bytes; ++cnt)
    {
      if (c_tolower (hex_digest[2 * cnt])
          != bin2hex[bin_buffer[cnt] >> 4]
          || (c_tolower (hex_digest[2 * cnt + 1])
              != (bin2hex[bin_buffer[cnt] & 0xf])))
        break;
    }
  return cnt == digest_bin_bytes;
}

static bool
digest_check (char const *checkfile_name)
{
  FILE *checkfile_stream;
  uintmax_t n_misformatted_lines = 0;
  uintmax_t n_mismatched_checksums = 0;
  uintmax_t n_open_or_read_failures = 0;
  bool properly_formatted_lines = false;
  bool matched_checksums = false;
  unsigned char bin_buffer_unaligned[DIGEST_BIN_BYTES + DIGEST_ALIGN];
  /* Make sure bin_buffer is properly aligned. */
  unsigned char *bin_buffer = ptr_align (bin_buffer_unaligned, DIGEST_ALIGN);
  uintmax_t line_number;
  char *line;
  size_t line_chars_allocated;
  bool is_stdin = streq (checkfile_name, "-");

  if (is_stdin)
    {
      have_read_stdin = true;
      checkfile_name = _("standard input");
      checkfile_stream = stdin;
    }
  else
    {
      checkfile_stream = fopen (checkfile_name, "r");
      if (checkfile_stream == nullptr)
        {
          error (0, errno, "%s", quotef (checkfile_name));
          return false;
        }
    }

  line_number = 0;
  line = nullptr;
  line_chars_allocated = 0;
  do
    {
      char *filename;
      int binary;
      unsigned char *digest;
      ssize_t line_length;

      ++line_number;
      if (line_number == 0)
        error (EXIT_FAILURE, 0, _("%s: too many checksum lines"),
               quotef (checkfile_name));

      line_length = getline (&line, &line_chars_allocated, checkfile_stream);
      if (line_length <= 0)
        break;

      /* Ignore comment lines, which begin with a '#' character.  */
      if (line[0] == '#')
        continue;

      /* Remove any trailing newline.  */
      line_length -= line[line_length - 1] == '\n';
      /* Remove any trailing carriage return.  */
      line_length -= line[line_length - (0 < line_length)] == '\r';

      /* Ignore empty lines.  */
      if (line_length == 0)
        continue;

      line[line_length] = '\0';

      size_t d_len;
      if (! (split_3 (line, line_length, &digest, &d_len, &binary, &filename)
             && ! (is_stdin && streq (filename, "-"))))
        {
          ++n_misformatted_lines;

          if (warn)
            {
              error (0, 0,
                     _("%s: %ju"
                       ": improperly formatted %s checksum line"),
                     quotef (checkfile_name), line_number,
                     DIGEST_TYPE_STRING);
            }
        }
      else
        {
          bool ok;
          bool missing;
          bool needs_escape = ! status_only && problematic_chars (filename);

          properly_formatted_lines = true;

          uintmax_t length;
          ok = digest_file (filename, &binary, bin_buffer, &missing, &length);

          if (!ok)
            {
              ++n_open_or_read_failures;
              if (!status_only)
                {
                  if (needs_escape)
                    putchar ('\\');
                  print_filename (filename, needs_escape);
                  printf (": %s\n", _("FAILED open or read"));
                }
            }
          else if (ignore_missing && missing)
            {
              /* Ignore missing files with --ignore-missing.  */
              ;
            }
          else
            {
              bool match = false;
#if HASH_ALGO_CKSUM
              if (d_len == BASE64_LENGTH (digest_length / 8))
                match = b64_equal (digest, bin_buffer);
              else
#endif
                if (d_len == digest_hex_bytes)
                  match = hex_equal (digest, bin_buffer);

              if (match)
                matched_checksums = true;
              else
                ++n_mismatched_checksums;

              if (!status_only)
                {
                  if (! match || ! quiet)
                    {
                      if (needs_escape)
                        putchar ('\\');
                      print_filename (filename, needs_escape);
                    }

                  if (! match)
                    printf (": %s\n", _("FAILED"));
                  else if (!quiet)
                    printf (": %s\n", _("OK"));
                }
            }
        }
    }
  while (!feof (checkfile_stream) && !ferror (checkfile_stream));

  free (line);

  int err = ferror (checkfile_stream) ? 0 : -1;
  if (is_stdin)
    clearerr (checkfile_stream);
  else if (fclose (checkfile_stream) != 0 && err < 0)
    err = errno;

  if (0 <= err)
    {
      error (0, err, err ? "%s" : _("%s: read error"),
             quotef (checkfile_name));
      return false;
    }

  if (! properly_formatted_lines)
    {
      /* Warn if no tests are found.  */
      error (0, 0, _("%s: no properly formatted checksum lines found"),
             quotef (checkfile_name));
    }
  else
    {
      if (!status_only)
        {
          if (n_misformatted_lines != 0)
            error (0, 0,
                   (ngettext
                    ("WARNING: %ju line is improperly formatted",
                     "WARNING: %ju lines are improperly formatted",
                     select_plural (n_misformatted_lines))),
                   n_misformatted_lines);

          if (n_open_or_read_failures != 0)
            error (0, 0,
                   (ngettext
                    ("WARNING: %ju listed file could not be read",
                     "WARNING: %ju listed files could not be read",
                     select_plural (n_open_or_read_failures))),
                   n_open_or_read_failures);

          if (n_mismatched_checksums != 0)
            error (0, 0,
                   (ngettext
                    ("WARNING: %ju computed checksum did NOT match",
                     "WARNING: %ju computed checksums did NOT match",
                     select_plural (n_mismatched_checksums))),
                   n_mismatched_checksums);

          if (ignore_missing && ! matched_checksums)
            error (0, 0, _("%s: no file was verified"),
                   quotef (checkfile_name));
        }
    }

  return (properly_formatted_lines
          && matched_checksums
          && n_mismatched_checksums == 0
          && n_open_or_read_failures == 0
          && (!strict || n_misformatted_lines == 0));
}

int
main (int argc, char **argv)
{
  unsigned char bin_buffer_unaligned[DIGEST_BIN_BYTES + DIGEST_ALIGN];
  /* Make sure bin_buffer is properly aligned. */
  unsigned char *bin_buffer = ptr_align (bin_buffer_unaligned, DIGEST_ALIGN);
  bool do_check = false;
  int opt;
  bool ok = true;
  int binary = -1;
  int prefix_tag = -1;

  /* Setting values of global variables.  */
  initialize_main (&argc, &argv);
  set_program_name (argv[0]);
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  /* Line buffer stdout to ensure lines are written atomically and immediately
     so that processes running in parallel do not intersperse their output.  */
  setvbuf (stdout, nullptr, _IOLBF, 0);

#if HASH_ALGO_SUM
  char const *short_opts = "rs";
#elif HASH_ALGO_CKSUM
  char const *short_opts = "a:l:bctwz";
  char const *digest_length_str = "";
#elif HASH_ALGO_BLAKE2
  char const *short_opts = "l:bctwz";
  char const *digest_length_str = "";
#else
  char const *short_opts = "bctwz";
#endif

  while ((opt = getopt_long (argc, argv, short_opts, long_options, nullptr))
         != -1)
    switch (opt)
      {
#if HASH_ALGO_CKSUM
      case 'a':
        cksum_algorithm = XARGMATCH_EXACT ("--algorithm", optarg,
                                           algorithm_args, algorithm_types);
        algorithm_specified = true;
        break;

      case DEBUG_PROGRAM_OPTION:
        cksum_debug = true;
        break;
#endif
#if HASH_ALGO_BLAKE2 || HASH_ALGO_CKSUM
      case 'l':
        digest_length = xnumtoumax (optarg, 10, 0, UINTMAX_MAX, "",
                                    _("invalid length"), 0,
                                    XTOINT_MAX_QUIET);
        digest_length_str = optarg;
        break;
#endif
#if !HASH_ALGO_SUM
      case 'c':
        do_check = true;
        break;
      case STATUS_OPTION:
        status_only = true;
        warn = false;
        quiet = false;
        break;
      case 'b':
        binary = 1;
        break;
      case 't':
        binary = 0;
        break;
      case 'w':
        status_only = false;
        warn = true;
        quiet = false;
        break;
      case IGNORE_MISSING_OPTION:
        ignore_missing = true;
        break;
      case QUIET_OPTION:
        status_only = false;
        warn = false;
        quiet = true;
        break;
      case STRICT_OPTION:
        strict = true;
        break;
# if HASH_ALGO_CKSUM
      case BASE64_OPTION:
        base64_digest = true;
        break;
      case RAW_OPTION:
        raw_digest = true;
        break;
      case UNTAG_OPTION:
        if (prefix_tag == 1)
          binary = -1;
        prefix_tag = 0;
        break;
# endif
      case TAG_OPTION:
        prefix_tag = 1;
        binary = 1;
        break;
      case 'z':
        digest_delim = '\0';
        break;
#endif
#if HASH_ALGO_SUM
      case 'r':		/* For SysV compatibility. */
        sum_algorithm = bsd;
        break;

      case 's':
        sum_algorithm = sysv;
        break;
#endif
      case_GETOPT_HELP_CHAR;
      case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);
      default:
        usage (EXIT_FAILURE);
      }

  min_digest_line_length = MIN_DIGEST_LINE_LENGTH;
#if HASH_ALGO_BLAKE2 || HASH_ALGO_CKSUM
# if HASH_ALGO_CKSUM
  if (digest_length && (cksum_algorithm != blake2b
                        && cksum_algorithm != sha2
                        && cksum_algorithm != sha3))
    error (EXIT_FAILURE, 0,
           _("--length is only supported with --algorithm "
             "blake2b, sha2, or sha3"));
  if (cksum_algorithm == sha2 || cksum_algorithm == sha3)
    {
      /* Do not require --length with --check.  */
      if (digest_length == 0 && *digest_length_str == '\0' && ! do_check)
        error (EXIT_FAILURE, 0, _("--algorithm=%s requires specifying "
                                  "--length 224, 256, 384, or 512"),
               algorithm_args[cksum_algorithm]);
      /* If --check and --length are used we verify the digest length.  */
      if ((! do_check || *digest_length_str != '\0')
          && digest_length != SHA224_DIGEST_SIZE * 8
          && digest_length != SHA256_DIGEST_SIZE * 8
          && digest_length != SHA384_DIGEST_SIZE * 8
          && digest_length != SHA512_DIGEST_SIZE * 8)
        {
          error (0, 0, _("invalid length: %s"), quote (digest_length_str));
          error (EXIT_FAILURE, 0, _("digest length for %s must be "
                                    "224, 256, 384, or 512"),
                 quote (DIGEST_TYPE_STRING));
        }
    }
  else
    {
      /* If the digest length checks for SHA-3 are satisfied, the less strict
         checks for BLAKE2 will also be.  */
# else
    {
# endif
      if (digest_length > DIGEST_MAX_LEN * 8)
        {
          error (0, 0, _("invalid length: %s"), quote (digest_length_str));
          error (EXIT_FAILURE, 0,
                 _("maximum digest length for %s is %d bits"),
                 quote (DIGEST_TYPE_STRING),
                 DIGEST_MAX_LEN * 8);
        }
      if (digest_length % 8 != 0)
        {
          error (0, 0, _("invalid length: %s"), quote (digest_length_str));
          error (EXIT_FAILURE, 0, _("length is not a multiple of 8"));
        }
    }
  if (digest_length == 0)
    {
# if HASH_ALGO_BLAKE2
      digest_length = DIGEST_MAX_LEN * 8;
# else
      digest_length = algorithm_bits[cksum_algorithm];
# endif
    }
  digest_hex_bytes = digest_length / 4;
#else
  digest_hex_bytes = DIGEST_HEX_BYTES;
#endif

#if HASH_ALGO_CKSUM
  switch (+cksum_algorithm)
    {
    case bsd:
    case sysv:
    case crc:
    case crc32b:
        if (do_check && algorithm_specified)
          error (EXIT_FAILURE, 0,
                 _("--check is not supported with "
                   "--algorithm={bsd,sysv,crc,crc32b}"));
        break;
    }

  if (base64_digest && raw_digest)
   {
     error (0, 0, _("--base64 and --raw are mutually exclusive"));
     usage (EXIT_FAILURE);
   }
#endif

  if (prefix_tag == -1)
    prefix_tag = HASH_ALGO_CKSUM;

  if (prefix_tag && !binary)
   {
     /* This could be supported in a backwards compatible way
        by prefixing the output line with a space in text mode.
        However that's invasive enough that it was agreed to
        not support this mode with --tag, as --text use cases
        are adequately supported by the default output format.  */
#if !HASH_ALGO_CKSUM
     error (0, 0, _("--tag does not support --text mode"));
#else
     error (0, 0, _("--text mode is only supported with --untagged"));
#endif
     usage (EXIT_FAILURE);
   }

  if (digest_delim != '\n' && do_check)
    {
      error (0, 0, _("the --zero option is not supported when "
                     "verifying checksums"));
      usage (EXIT_FAILURE);
    }
#if !HASH_ALGO_CKSUM
  if (prefix_tag && do_check)
    {
      error (0, 0, _("the --tag option is meaningless when "
                     "verifying checksums"));
      usage (EXIT_FAILURE);
    }
#endif

  if (0 <= binary && do_check)
    {
      error (0, 0, _("the --binary and --text options are meaningless when "
                     "verifying checksums"));
      usage (EXIT_FAILURE);
    }

  if (ignore_missing && !do_check)
    {
      error (0, 0,
             _("the --ignore-missing option is meaningful only when "
               "verifying checksums"));
      usage (EXIT_FAILURE);
    }

  if (status_only && !do_check)
    {
      error (0, 0,
       _("the --status option is meaningful only when verifying checksums"));
      usage (EXIT_FAILURE);
    }

  if (warn && !do_check)
    {
      error (0, 0,
       _("the --warn option is meaningful only when verifying checksums"));
      usage (EXIT_FAILURE);
    }

  if (quiet && !do_check)
    {
      error (0, 0,
       _("the --quiet option is meaningful only when verifying checksums"));
      usage (EXIT_FAILURE);
    }

  if (strict & !do_check)
   {
     error (0, 0,
        _("the --strict option is meaningful only when verifying checksums"));
     usage (EXIT_FAILURE);
   }

  if (!O_BINARY && binary < 0)
    binary = 0;

  char **operand_lim = argv + argc;
  if (optind == argc)
    *operand_lim++ = bad_cast ("-");
  else if (1 < argc - optind && raw_digest)
    error (EXIT_FAILURE, 0,
           _("the --raw option is not supported with multiple files"));

  for (char **operandp = argv + optind; operandp < operand_lim; operandp++)
    {
      char *file = *operandp;
      if (do_check)
        ok &= digest_check (file);
      else
        {
          int binary_file = binary;
          bool missing;
          uintmax_t length;

          if (! digest_file (file, &binary_file, bin_buffer, &missing, &length))
            ok = false;
          else
            {
              DIGEST_OUT (file, binary_file, bin_buffer, raw_digest, prefix_tag,
                          digest_delim, optind != argc, length);
            }
        }
    }

  if (have_read_stdin && fclose (stdin) == EOF)
    error (EXIT_FAILURE, errno, _("standard input"));

  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
