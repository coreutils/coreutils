/* od -- dump files in octal and other formats
   Copyright (C) 92, 1995-2002 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

/* Written by Jim Meyering.  */

#include <config.h>

#include <stdio.h>
#include <assert.h>
#include <getopt.h>
#include <sys/types.h>
#include "system.h"
#include "closeout.h"
#include "error.h"
#include "posixver.h"
#include "xstrtol.h"

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "od"

#define AUTHORS "Jim Meyering"

#if defined(__GNUC__) || defined(STDC_HEADERS)
# include <float.h>
#endif

#ifdef HAVE_LONG_DOUBLE
typedef long double LONG_DOUBLE;
#else
typedef double LONG_DOUBLE;
#endif

/* The default number of input bytes per output line.  */
#define DEFAULT_BYTES_PER_BLOCK 16

/* The number of decimal digits of precision in a float.  */
#ifndef FLT_DIG
# define FLT_DIG 7
#endif

/* The number of decimal digits of precision in a double.  */
#ifndef DBL_DIG
# define DBL_DIG 15
#endif

/* The number of decimal digits of precision in a long double.  */
#ifndef LDBL_DIG
# define LDBL_DIG DBL_DIG
#endif

#if HAVE_UNSIGNED_LONG_LONG
typedef unsigned long long ulonglong_t;
#else
/* This is just a place-holder to avoid a few `#if' directives.
   In this case, the type isn't actually used.  */
typedef unsigned long int ulonglong_t;
#endif

enum size_spec
  {
    NO_SIZE,
    CHAR,
    SHORT,
    INT,
    LONG,
    LONG_LONG,
    /* FIXME: add INTMAX support, too */
    FLOAT_SINGLE,
    FLOAT_DOUBLE,
    FLOAT_LONG_DOUBLE,
    N_SIZE_SPECS
  };

enum output_format
  {
    SIGNED_DECIMAL,
    UNSIGNED_DECIMAL,
    OCTAL,
    HEXADECIMAL,
    FLOATING_POINT,
    NAMED_CHARACTER,
    CHARACTER
  };

/* Each output format specification (from `-t spec' or from
   old-style options) is represented by one of these structures.  */
struct tspec
  {
    enum output_format fmt;
    enum size_spec size;
    void (*print_function) (size_t, const char *, const char *);
    char *fmt_string;
    int hexl_mode_trailer;
    int field_width;
  };

/* The name this program was run with.  */
char *program_name;

/* Convert the number of 8-bit bytes of a binary representation to
   the number of characters (digits + sign if the type is signed)
   required to represent the same quantity in the specified base/type.
   For example, a 32-bit (4-byte) quantity may require a field width
   as wide as the following for these types:
   11	unsigned octal
   11	signed decimal
   10	unsigned decimal
   8	unsigned hexadecimal  */

static const unsigned int bytes_to_oct_digits[] =
{0, 3, 6, 8, 11, 14, 16, 19, 22, 25, 27, 30, 32, 35, 38, 41, 43};

static const unsigned int bytes_to_signed_dec_digits[] =
{1, 4, 6, 8, 11, 13, 16, 18, 20, 23, 25, 28, 30, 33, 35, 37, 40};

static const unsigned int bytes_to_unsigned_dec_digits[] =
{0, 3, 5, 8, 10, 13, 15, 17, 20, 22, 25, 27, 29, 32, 34, 37, 39};

static const unsigned int bytes_to_hex_digits[] =
{0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32};

/* Convert enum size_spec to the size of the named type.  */
static const int width_bytes[] =
{
  -1,
  sizeof (char),
  sizeof (short int),
  sizeof (int),
  sizeof (long int),
  sizeof (ulonglong_t),
  sizeof (float),
  sizeof (double),
  sizeof (LONG_DOUBLE)
};

/* Ensure that for each member of `enum size_spec' there is an
   initializer in the width_bytes array.  */
struct dummy
{
  int assert_width_bytes_matches_size_spec_decl
    [sizeof width_bytes / sizeof width_bytes[0] == N_SIZE_SPECS ? 1 : -1];
};

/* Names for some non-printing characters.  */
static const char *const charname[33] =
{
  "nul", "soh", "stx", "etx", "eot", "enq", "ack", "bel",
  "bs", "ht", "nl", "vt", "ff", "cr", "so", "si",
  "dle", "dc1", "dc2", "dc3", "dc4", "nak", "syn", "etb",
  "can", "em", "sub", "esc", "fs", "gs", "rs", "us",
  "sp"
};

/* Address base (8, 10 or 16).  */
static int address_base;

/* The number of octal digits required to represent the largest
   address value.  */
#define MAX_ADDRESS_LENGTH \
  ((sizeof (uintmax_t) * CHAR_BIT + CHAR_BIT - 1) / 3)

/* Width of a normal address.  */
static int address_pad_len;

static size_t string_min;
static int flag_dump_strings;

/* Non-zero if we should recognize the older non-option arguments
   that specified at most one file and optional arguments specifying
   offset and pseudo-start address.  */
static int traditional;

/* Non-zero if an old-style `pseudo-address' was specified.  */
static int flag_pseudo_start;

/* The difference between the old-style pseudo starting address and
   the number of bytes to skip.  */
static uintmax_t pseudo_offset;

/* Function that accepts an address and an optional following char,
   and prints the address and char to stdout.  */
static void (*format_address) (uintmax_t, char);

/* The number of input bytes to skip before formatting and writing.  */
static uintmax_t n_bytes_to_skip = 0;

/* When zero, MAX_BYTES_TO_FORMAT and END_OFFSET are ignored, and all
   input is formatted.  */
static int limit_bytes_to_format = 0;

/* The maximum number of bytes that will be formatted.  */
static uintmax_t max_bytes_to_format;

/* The offset of the first byte after the last byte to be formatted.  */
static uintmax_t end_offset;

/* When nonzero and two or more consecutive blocks are equal, format
   only the first block and output an asterisk alone on the following
   line to indicate that identical blocks have been elided.  */
static int abbreviate_duplicate_blocks = 1;

/* An array of specs describing how to format each input block.  */
static struct tspec *spec;

/* The number of format specs.  */
static size_t n_specs;

/* The allocated length of SPEC.  */
static size_t n_specs_allocated;

/* The number of input bytes formatted per output line.  It must be
   a multiple of the least common multiple of the sizes associated with
   the specified output types.  It should be as large as possible, but
   no larger than 16 -- unless specified with the -w option.  */
static size_t bytes_per_block;

/* Human-readable representation of *file_list (for error messages).
   It differs from *file_list only when *file_list is "-".  */
static char const *input_filename;

/* A NULL-terminated list of the file-arguments from the command line.  */
static char const *const *file_list;

/* Initializer for file_list if no file-arguments
   were specified on the command line.  */
static char const *const default_file_list[] = {"-", NULL};

/* The input stream associated with the current file.  */
static FILE *in_stream;

/* If nonzero, at least one of the files we read was standard input.  */
static int have_read_stdin;

#define MAX_INTEGRAL_TYPE_SIZE sizeof (ulonglong_t)
static enum size_spec integral_type_size[MAX_INTEGRAL_TYPE_SIZE + 1];

#define MAX_FP_TYPE_SIZE sizeof(LONG_DOUBLE)
static enum size_spec fp_type_size[MAX_FP_TYPE_SIZE + 1];

#define COMMON_SHORT_OPTIONS "A:N:abcdfhij:lot:vx"

/* For long options that have no equivalent short option, use a
   non-character as a pseudo short option, starting with CHAR_MAX + 1.  */
enum
{
  TRADITIONAL_OPTION = CHAR_MAX + 1
};

static struct option const long_options[] =
{
  {"skip-bytes", required_argument, NULL, 'j'},
  {"address-radix", required_argument, NULL, 'A'},
  {"read-bytes", required_argument, NULL, 'N'},
  {"format", required_argument, NULL, 't'},
  {"output-duplicates", no_argument, NULL, 'v'},
  {"strings", optional_argument, NULL, 's'},
  {"traditional", no_argument, NULL, TRADITIONAL_OPTION},
  {"width", optional_argument, NULL, 'w'},

  {GETOPT_HELP_OPTION_DECL},
  {GETOPT_VERSION_OPTION_DECL},
  {NULL, 0, NULL, 0}
};

void
usage (int status)
{
  if (status != 0)
    fprintf (stderr, _("Try `%s --help' for more information.\n"),
	     program_name);
  else
    {
      printf (_("\
Usage: %s [OPTION]... [FILE]...\n\
  or:  %s --traditional [FILE] [[+]OFFSET [[+]LABEL]]\n\
"),
	      program_name, program_name);
      fputs (_("\n\
Write an unambiguous representation, octal bytes by default,\n\
of FILE to standard output.  With more than one FILE argument,\n\
concatenate them in the listed order to form the input.\n\
With no FILE, or when FILE is -, read standard input.\n\
\n\
"), stdout);
      fputs (_("\
All arguments to long options are mandatory for short options.\n\
"), stdout);
      fputs (_("\
  -A, --address-radix=RADIX   decide how file offsets are printed\n\
  -j, --skip-bytes=BYTES      skip BYTES input bytes first\n\
"), stdout);
      fputs (_("\
  -N, --read-bytes=BYTES      limit dump to BYTES input bytes\n\
  -s, --strings[=BYTES]       output strings of at least BYTES graphic chars\n\
  -t, --format=TYPE           select output format or formats\n\
  -v, --output-duplicates     do not use * to mark line suppression\n\
  -w, --width[=BYTES]         output BYTES bytes per output line\n\
      --traditional           accept arguments in traditional form\n\
"), stdout);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      fputs (_("\
\n\
Traditional format specifications may be intermixed; they accumulate:\n\
  -a   same as -t a,  select named characters\n\
  -b   same as -t oC, select octal bytes\n\
  -c   same as -t c,  select ASCII characters or backslash escapes\n\
  -d   same as -t u2, select unsigned decimal shorts\n\
"), stdout);
      fputs (_("\
  -f   same as -t fF, select floats\n\
  -h   same as -t x2, select hexadecimal shorts\n\
  -i   same as -t d2, select decimal shorts\n\
  -l   same as -t d4, select decimal longs\n\
  -o   same as -t o2, select octal shorts\n\
  -x   same as -t x2, select hexadecimal shorts\n\
"), stdout);
      fputs (_("\
\n\
For older syntax (second call format), OFFSET means -j OFFSET.  LABEL\n\
is the pseudo-address at first byte printed, incremented when dump is\n\
progressing.  For OFFSET and LABEL, a 0x or 0X prefix indicates\n\
hexadecimal, suffixes may be . for octal and b for multiply by 512.\n\
\n\
TYPE is made up of one or more of these specifications:\n\
\n\
  a          named character\n\
  c          ASCII character or backslash escape\n\
"), stdout);
      fputs (_("\
  d[SIZE]    signed decimal, SIZE bytes per integer\n\
  f[SIZE]    floating point, SIZE bytes per integer\n\
  o[SIZE]    octal, SIZE bytes per integer\n\
  u[SIZE]    unsigned decimal, SIZE bytes per integer\n\
  x[SIZE]    hexadecimal, SIZE bytes per integer\n\
"), stdout);
      fputs (_("\
\n\
SIZE is a number.  For TYPE in doux, SIZE may also be C for\n\
sizeof(char), S for sizeof(short), I for sizeof(int) or L for\n\
sizeof(long).  If TYPE is f, SIZE may also be F for sizeof(float), D\n\
for sizeof(double) or L for sizeof(long double).\n\
"), stdout);
      fputs (_("\
\n\
RADIX is d for decimal, o for octal, x for hexadecimal or n for none.\n\
BYTES is hexadecimal with 0x or 0X prefix, it is multiplied by 512\n\
with b suffix, by 1024 with k and by 1048576 with m.  Adding a z suffix to\n\
any type adds a display of printable characters to the end of each line\n\
of output.  \
"), stdout);
      fputs (_("\
--string without a number implies 3.  --width without a number\n\
implies 32.  By default, od uses -A o -t d2 -w 16.\n\
"), stdout);
      printf (_("\nReport bugs to <%s>.\n"), PACKAGE_BUGREPORT);
    }
  exit (status == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}

/* Compute the greatest common denominator of U and V
   using Euclid's algorithm.  */

static unsigned int
gcd (unsigned int u, unsigned int v)
{
  unsigned int t;
  while (v != 0)
    {
      t = u % v;
      u = v;
      v = t;
    }
  return u;
}

/* Compute the least common multiple of U and V.  */

static unsigned int
lcm (unsigned int u, unsigned int v)
{
  unsigned int t = gcd (u, v);
  if (t == 0)
    return 0;
  return u * v / t;
}

static void
print_s_char (size_t n_bytes, const char *block, const char *fmt_string)
{
  size_t i;
  for (i = n_bytes; i > 0; i--)
    {
      int tmp = (unsigned) *(const unsigned char *) block;
      if (tmp > SCHAR_MAX)
	tmp -= SCHAR_MAX - SCHAR_MIN + 1;
      assert (tmp <= SCHAR_MAX);
      printf (fmt_string, tmp);
      block += sizeof (unsigned char);
    }
}

static void
print_char (size_t n_bytes, const char *block, const char *fmt_string)
{
  size_t i;
  for (i = n_bytes; i > 0; i--)
    {
      unsigned int tmp = *(const unsigned char *) block;
      printf (fmt_string, tmp);
      block += sizeof (unsigned char);
    }
}

static void
print_s_short (size_t n_bytes, const char *block, const char *fmt_string)
{
  size_t i;
  for (i = n_bytes / sizeof (unsigned short); i > 0; i--)
    {
      int tmp = (unsigned) *(const unsigned short *) block;
      if (tmp > SHRT_MAX)
	tmp -= SHRT_MAX - SHRT_MIN + 1;
      assert (tmp <= SHRT_MAX);
      printf (fmt_string, tmp);
      block += sizeof (unsigned short);
    }
}

static void
print_short (size_t n_bytes, const char *block, const char *fmt_string)
{
  size_t i;
  for (i = n_bytes / sizeof (unsigned short); i > 0; i--)
    {
      unsigned int tmp = *(const unsigned short *) block;
      printf (fmt_string, tmp);
      block += sizeof (unsigned short);
    }
}

static void
print_int (size_t n_bytes, const char *block, const char *fmt_string)
{
  size_t i;
  for (i = n_bytes / sizeof (unsigned int); i > 0; i--)
    {
      unsigned int tmp = *(const unsigned int *) block;
      printf (fmt_string, tmp);
      block += sizeof (unsigned int);
    }
}

static void
print_long (size_t n_bytes, const char *block, const char *fmt_string)
{
  size_t i;
  for (i = n_bytes / sizeof (unsigned long); i > 0; i--)
    {
      unsigned long tmp = *(const unsigned long *) block;
      printf (fmt_string, tmp);
      block += sizeof (unsigned long);
    }
}

static void
print_long_long (size_t n_bytes, const char *block, const char *fmt_string)
{
  size_t i;
  for (i = n_bytes / sizeof (ulonglong_t); i > 0; i--)
    {
      ulonglong_t tmp = *(const ulonglong_t *) block;
      printf (fmt_string, tmp);
      block += sizeof (ulonglong_t);
    }
}

static void
print_float (size_t n_bytes, const char *block, const char *fmt_string)
{
  size_t i;
  for (i = n_bytes / sizeof (float); i > 0; i--)
    {
      float tmp = *(const float *) block;
      printf (fmt_string, tmp);
      block += sizeof (float);
    }
}

static void
print_double (size_t n_bytes, const char *block, const char *fmt_string)
{
  size_t i;
  for (i = n_bytes / sizeof (double); i > 0; i--)
    {
      double tmp = *(const double *) block;
      printf (fmt_string, tmp);
      block += sizeof (double);
    }
}

#ifdef HAVE_LONG_DOUBLE
static void
print_long_double (size_t n_bytes, const char *block, const char *fmt_string)
{
  size_t i;
  for (i = n_bytes / sizeof (LONG_DOUBLE); i > 0; i--)
    {
      LONG_DOUBLE tmp = *(const LONG_DOUBLE *) block;
      printf (fmt_string, tmp);
      block += sizeof (LONG_DOUBLE);
    }
}

#endif

static void
dump_hexl_mode_trailer (size_t n_bytes, const char *block)
{
  size_t i;
  fputs ("  >", stdout);
  for (i = n_bytes; i > 0; i--)
    {
      unsigned int c = *(const unsigned char *) block;
      unsigned int c2 = (ISPRINT(c) ? c : '.');
      putchar (c2);
      block += sizeof (unsigned char);
    }
  putchar ('<');
}

static void
print_named_ascii (size_t n_bytes, const char *block,
		   const char *unused_fmt_string ATTRIBUTE_UNUSED)
{
  size_t i;
  for (i = n_bytes; i > 0; i--)
    {
      unsigned int c = *(const unsigned char *) block;
      unsigned int masked_c = (0x7f & c);
      const char *s;
      char buf[5];

      if (masked_c == 127)
	s = "del";
      else if (masked_c <= 040)
	s = charname[masked_c];
      else
	{
	  sprintf (buf, "  %c", masked_c);
	  s = buf;
	}

      printf (" %3s", s);
      block += sizeof (unsigned char);
    }
}

static void
print_ascii (size_t n_bytes, const char *block,
	     const char *unused_fmt_string ATTRIBUTE_UNUSED)
{
  size_t i;
  for (i = n_bytes; i > 0; i--)
    {
      unsigned int c = *(const unsigned char *) block;
      const char *s;
      char buf[5];

      switch (c)
	{
	case '\0':
	  s = " \\0";
	  break;

	case '\007':
	  s = " \\a";
	  break;

	case '\b':
	  s = " \\b";
	  break;

	case '\f':
	  s = " \\f";
	  break;

	case '\n':
	  s = " \\n";
	  break;

	case '\r':
	  s = " \\r";
	  break;

	case '\t':
	  s = " \\t";
	  break;

	case '\v':
	  s = " \\v";
	  break;

	default:
	  sprintf (buf, (ISPRINT (c) ? "  %c" : "%03o"), c);
	  s = (const char *) buf;
	}

      printf (" %3s", s);
      block += sizeof (unsigned char);
    }
}

/* Convert a null-terminated (possibly zero-length) string S to an
   unsigned long integer value.  If S points to a non-digit set *P to S,
   *VAL to 0, and return 0.  Otherwise, accumulate the integer value of
   the string of digits.  If the string of digits represents a value
   larger than ULONG_MAX, don't modify *VAL or *P and return nonzero.
   Otherwise, advance *P to the first non-digit after S, set *VAL to
   the result of the conversion and return zero.  */

static int
simple_strtoul (const char *s, const char **p, long unsigned int *val)
{
  unsigned long int sum;

  sum = 0;
  while (ISDIGIT (*s))
    {
      unsigned int c = *s++ - '0';
      if (sum > (ULONG_MAX - c) / 10)
	return 1;
      sum = sum * 10 + c;
    }
  *p = s;
  *val = sum;
  return 0;
}

/* If S points to a single valid modern od format string, put
   a description of that format in *TSPEC, make *NEXT point at the
   character following the just-decoded format (if *NEXT is non-NULL),
   and return zero.  If S is not valid, don't modify *NEXT or *TSPEC,
   give a diagnostic, and return nonzero.  For example, if S were
   "d4afL" *NEXT would be set to "afL" and *TSPEC would be
     {
       fmt = SIGNED_DECIMAL;
       size = INT or LONG; (whichever integral_type_size[4] resolves to)
       print_function = print_int; (assuming size == INT)
       fmt_string = "%011d%c";
      }
   S_ORIG is solely for reporting errors.  It should be the full format
   string argument.
   */

static int
decode_one_format (const char *s_orig, const char *s, const char **next,
		   struct tspec *tspec)
{
  enum size_spec size_spec;
  unsigned long int size;
  enum output_format fmt;
  const char *pre_fmt_string;
  char *fmt_string;
  void (*print_function) (size_t, const char *, const char *);
  const char *p;
  unsigned int c;
  unsigned int field_width = 0;

  assert (tspec != NULL);

  switch (*s)
    {
    case 'd':
    case 'o':
    case 'u':
    case 'x':
      c = *s;
      ++s;
      switch (*s)
	{
	case 'C':
	  ++s;
	  size = sizeof (char);
	  break;

	case 'S':
	  ++s;
	  size = sizeof (short);
	  break;

	case 'I':
	  ++s;
	  size = sizeof (int);
	  break;

	case 'L':
	  ++s;
	  size = sizeof (long int);
	  break;

	default:
	  if (simple_strtoul (s, &p, &size) != 0)
	    {
	      /* The integer at P in S would overflow an unsigned long.
		 A digit string that long is sufficiently odd looking
		 that the following diagnostic is sufficient.  */
	      error (0, 0, _("invalid type string `%s'"), s_orig);
	      return 1;
	    }
	  if (p == s)
	    size = sizeof (int);
	  else
	    {
	      if (MAX_INTEGRAL_TYPE_SIZE < size
		  || integral_type_size[size] == NO_SIZE)
		{
		  error (0, 0, _("invalid type string `%s';\n\
this system doesn't provide a %lu-byte integral type"), s_orig, size);
		  return 1;
		}
	      s = p;
	    }
	  break;
	}

#define ISPEC_TO_FORMAT(Spec, Min_format, Long_format, Max_format)	\
  ((Spec) == LONG_LONG ? (Max_format)					\
   : ((Spec) == LONG ? (Long_format)					\
      : (Min_format)))							\

#define FMT_BYTES_ALLOCATED 9
      fmt_string = xmalloc (FMT_BYTES_ALLOCATED);

      size_spec = integral_type_size[size];

      switch (c)
	{
	case 'd':
	  fmt = SIGNED_DECIMAL;
	  sprintf (fmt_string, " %%%u%s",
		   (field_width = bytes_to_signed_dec_digits[size]),
		   ISPEC_TO_FORMAT (size_spec, "d", "ld", PRIdMAX));
	  break;

	case 'o':
	  fmt = OCTAL;
	  sprintf (fmt_string, " %%0%u%s",
		   (field_width = bytes_to_oct_digits[size]),
		   ISPEC_TO_FORMAT (size_spec, "o", "lo", PRIoMAX));
	  break;

	case 'u':
	  fmt = UNSIGNED_DECIMAL;
	  sprintf (fmt_string, " %%%u%s",
		   (field_width = bytes_to_unsigned_dec_digits[size]),
		   ISPEC_TO_FORMAT (size_spec, "u", "lu", PRIuMAX));
	  break;

	case 'x':
	  fmt = HEXADECIMAL;
	  sprintf (fmt_string, " %%0%u%s",
		   (field_width = bytes_to_hex_digits[size]),
		   ISPEC_TO_FORMAT (size_spec, "x", "lx", PRIxMAX));
	  break;

	default:
	  abort ();
	}

      assert (strlen (fmt_string) < FMT_BYTES_ALLOCATED);

      switch (size_spec)
	{
	case CHAR:
	  print_function = (fmt == SIGNED_DECIMAL
			    ? print_s_char
			    : print_char);
	  break;

	case SHORT:
	  print_function = (fmt == SIGNED_DECIMAL
			    ? print_s_short
			    : print_short);
	  break;

	case INT:
	  print_function = print_int;
	  break;

	case LONG:
	  print_function = print_long;
	  break;

	case LONG_LONG:
	  print_function = print_long_long;
	  break;

	default:
	  abort ();
	}
      break;

    case 'f':
      fmt = FLOATING_POINT;
      ++s;
      switch (*s)
	{
	case 'F':
	  ++s;
	  size = sizeof (float);
	  break;

	case 'D':
	  ++s;
	  size = sizeof (double);
	  break;

	case 'L':
	  ++s;
	  size = sizeof (LONG_DOUBLE);
	  break;

	default:
	  if (simple_strtoul (s, &p, &size) != 0)
	    {
	      /* The integer at P in S would overflow an unsigned long.
		 A digit string that long is sufficiently odd looking
		 that the following diagnostic is sufficient.  */
	      error (0, 0, _("invalid type string `%s'"), s_orig);
	      return 1;
	    }
	  if (p == s)
	    size = sizeof (double);
	  else
	    {
	      if (size > MAX_FP_TYPE_SIZE
		  || fp_type_size[size] == NO_SIZE)
		{
		  error (0, 0, _("invalid type string `%s';\n\
this system doesn't provide a %lu-byte floating point type"), s_orig, size);
		  return 1;
		}
	      s = p;
	    }
	  break;
	}
      size_spec = fp_type_size[size];

      switch (size_spec)
	{
	case FLOAT_SINGLE:
	  print_function = print_float;
	  /* Don't use %#e; not all systems support it.  */
	  pre_fmt_string = " %%%d.%de";
	  fmt_string = xmalloc (strlen (pre_fmt_string));
	  sprintf (fmt_string, pre_fmt_string,
		   (field_width = FLT_DIG + 8), FLT_DIG);
	  break;

	case FLOAT_DOUBLE:
	  print_function = print_double;
	  pre_fmt_string = " %%%d.%de";
	  fmt_string = xmalloc (strlen (pre_fmt_string));
	  sprintf (fmt_string, pre_fmt_string,
		   (field_width = DBL_DIG + 8), DBL_DIG);
	  break;

#ifdef HAVE_LONG_DOUBLE
	case FLOAT_LONG_DOUBLE:
	  print_function = print_long_double;
	  pre_fmt_string = " %%%d.%dLe";
	  fmt_string = xmalloc (strlen (pre_fmt_string));
	  sprintf (fmt_string, pre_fmt_string,
		   (field_width = LDBL_DIG + 8), LDBL_DIG);
	  break;
#endif

	default:
	  abort ();
	}
      break;

    case 'a':
      ++s;
      fmt = NAMED_CHARACTER;
      size_spec = CHAR;
      fmt_string = NULL;
      print_function = print_named_ascii;
      field_width = 3;
      break;

    case 'c':
      ++s;
      fmt = CHARACTER;
      size_spec = CHAR;
      fmt_string = NULL;
      print_function = print_ascii;
      field_width = 3;
      break;

    default:
      error (0, 0, _("invalid character `%c' in type string `%s'"),
	     *s, s_orig);
      return 1;
    }

  tspec->size = size_spec;
  tspec->fmt = fmt;
  tspec->print_function = print_function;
  tspec->fmt_string = fmt_string;

  tspec->field_width = field_width;
  tspec->hexl_mode_trailer = (*s == 'z');
  if (tspec->hexl_mode_trailer)
    s++;

  if (next != NULL)
    *next = s;

  return 0;
}

/* Given a list of one or more input filenames FILE_LIST, set the global
   file pointer IN_STREAM and the global string INPUT_FILENAME to the
   first one that can be successfully opened. Modify FILE_LIST to
   reference the next filename in the list.  A file name of "-" is
   interpreted as standard input.  If any file open fails, give an error
   message and return nonzero.  */

static int
open_next_file (void)
{
  int err = 0;

  do
    {
      input_filename = *file_list;
      if (input_filename == NULL)
	return err;
      ++file_list;

      if (STREQ (input_filename, "-"))
	{
	  input_filename = _("standard input");
	  in_stream = stdin;
	  have_read_stdin = 1;
	}
      else
	{
	  in_stream = fopen (input_filename, "r");
	  if (in_stream == NULL)
	    {
	      error (0, errno, "%s", input_filename);
	      err = 1;
	    }
	}
    }
  while (in_stream == NULL);

  if (limit_bytes_to_format && !flag_dump_strings)
    SETVBUF (in_stream, NULL, _IONBF, 0);
  SET_BINARY (fileno (in_stream));

  return err;
}

/* Test whether there have been errors on in_stream, and close it if
   it is not standard input.  Return nonzero if there has been an error
   on in_stream or stdout; return zero otherwise.  This function will
   report more than one error only if both a read and a write error
   have occurred.  */

static int
check_and_close (void)
{
  int err = 0;

  if (in_stream != NULL)
    {
      if (ferror (in_stream))
	{
	  error (0, errno, "%s", input_filename);
	  if (in_stream != stdin)
	    fclose (in_stream);
	  err = 1;
	}
      else if (in_stream != stdin && fclose (in_stream) == EOF)
	{
	  error (0, errno, "%s", input_filename);
	  err = 1;
	}

      in_stream = NULL;
    }

  if (ferror (stdout))
    {
      error (0, errno, _("standard output"));
      err = 1;
    }

  return err;
}

/* Decode the modern od format string S.  Append the decoded
   representation to the global array SPEC, reallocating SPEC if
   necessary.  Return zero if S is valid, nonzero otherwise.  */

static int
decode_format_string (const char *s)
{
  const char *s_orig = s;
  assert (s != NULL);

  while (*s != '\0')
    {
      struct tspec tspec;
      const char *next;

      if (decode_one_format (s_orig, s, &next, &tspec))
	return 1;

      assert (s != next);
      s = next;

      if (n_specs >= n_specs_allocated)
	{
	  n_specs_allocated = 1 + (3 * n_specs_allocated) / 2;
	  spec = (struct tspec *) xrealloc ((char *) spec,
					    (n_specs_allocated
					     * sizeof (struct tspec)));
	}

      memcpy ((char *) &spec[n_specs], (char *) &tspec,
	      sizeof (struct tspec));
      ++n_specs;
    }

  return 0;
}

/* Given a list of one or more input filenames FILE_LIST, set the global
   file pointer IN_STREAM to position N_SKIP in the concatenation of
   those files.  If any file operation fails or if there are fewer than
   N_SKIP bytes in the combined input, give an error message and return
   nonzero.  When possible, use seek rather than read operations to
   advance IN_STREAM.  */

static int
skip (uintmax_t n_skip)
{
  int err = 0;

  if (n_skip == 0)
    return 0;

  while (in_stream != NULL)	/* EOF.  */
    {
      struct stat file_stats;

      /* First try seeking.  For large offsets, this extra work is
	 worthwhile.  If the offset is below some threshold it may be
	 more efficient to move the pointer by reading.  There are two
	 issues when trying to seek:
	   - the file must be seekable.
	   - before seeking to the specified position, make sure
	     that the new position is in the current file.
	     Try to do that by getting file's size using fstat.
	     But that will work only for regular files.  */

      if (fstat (fileno (in_stream), &file_stats) == 0)
	{
	  /* The st_size field is valid only for regular files
	     (and for symbolic links, which cannot occur here).
	     If the number of bytes left to skip is at least
	     as large as the size of the current file, we can
	     decrement n_skip and go on to the next file.  */

	  if (S_ISREG (file_stats.st_mode) && 0 <= file_stats.st_size)
	    {
	      if ((uintmax_t) file_stats.st_size <= n_skip)
		n_skip -= file_stats.st_size;
	      else
		{
		  if (fseeko (in_stream, n_skip, SEEK_CUR) != 0)
		    {
		      error (0, errno, "%s", input_filename);
		      err = 1;
		    }
		  n_skip = 0;
		}
	    }

	  /* If it's not a regular file with nonnegative size,
	     position the file pointer by reading.  */

	  else
	    {
	      char buf[BUFSIZ];
	      size_t n_bytes_read, n_bytes_to_read = BUFSIZ;

	      while (0 < n_skip)
		{
		  if (n_skip < n_bytes_to_read)
		    n_bytes_to_read = n_skip;
		  n_bytes_read = fread (buf, 1, n_bytes_to_read, in_stream);
		  n_skip -= n_bytes_read;
		  if (n_bytes_read != n_bytes_to_read)
		    break;
		}
	    }

	  if (n_skip == 0)
	    break;
	}

      else   /* cannot fstat() file */
	{
	  error (0, errno, "%s", input_filename);
	  err = 1;
	}

      err |= check_and_close ();

      err |= open_next_file ();
    }

  if (n_skip != 0)
    error (EXIT_FAILURE, 0, _("cannot skip past end of combined input"));

  return err;
}

static void
format_address_none (uintmax_t address ATTRIBUTE_UNUSED, char c ATTRIBUTE_UNUSED)
{
}

static void
format_address_std (uintmax_t address, char c)
{
  char buf[MAX_ADDRESS_LENGTH + 2];
  char *p = buf + sizeof buf;
  char const *pbound;

  *--p = '\0';
  *--p = c;
  pbound = p - address_pad_len;

  /* Use a special case of the code for each base.  This is measurably
     faster than generic code.  */
  switch (address_base)
    {
    case 8:
      do
	*--p = '0' + (address & 7);
      while ((address >>= 3) != 0);
      break;

    case 10:
      do
	*--p = '0' + (address % 10);
      while ((address /= 10) != 0);
      break;

    case 16:
      do
	*--p = "0123456789abcdef"[address & 15];
      while ((address >>= 4) != 0);
      break;
    }

  while (pbound < p)
    *--p = '0';

  fputs (p, stdout);
}

static void
format_address_paren (uintmax_t address, char c)
{
  putchar ('(');
  format_address_std (address, ')');
  putchar (c);
}

static void
format_address_label (uintmax_t address, char c)
{
  format_address_std (address, ' ');
  format_address_paren (address + pseudo_offset, c);
}

/* Write N_BYTES bytes from CURR_BLOCK to standard output once for each
   of the N_SPEC format specs.  CURRENT_OFFSET is the byte address of
   CURR_BLOCK in the concatenation of input files, and it is printed
   (optionally) only before the output line associated with the first
   format spec.  When duplicate blocks are being abbreviated, the output
   for a sequence of identical input blocks is the output for the first
   block followed by an asterisk alone on a line.  It is valid to compare
   the blocks PREV_BLOCK and CURR_BLOCK only when N_BYTES == BYTES_PER_BLOCK.
   That condition may be false only for the last input block -- and then
   only when it has not been padded to length BYTES_PER_BLOCK.  */

static void
write_block (uintmax_t current_offset, size_t n_bytes,
	     const char *prev_block, const char *curr_block)
{
  static int first = 1;
  static int prev_pair_equal = 0;

#define EQUAL_BLOCKS(b1, b2) (memcmp ((b1), (b2), bytes_per_block) == 0)

  if (abbreviate_duplicate_blocks
      && !first && n_bytes == bytes_per_block
      && EQUAL_BLOCKS (prev_block, curr_block))
    {
      if (prev_pair_equal)
	{
	  /* The two preceding blocks were equal, and the current
	     block is the same as the last one, so print nothing.  */
	}
      else
	{
	  printf ("*\n");
	  prev_pair_equal = 1;
	}
    }
  else
    {
      size_t i;

      prev_pair_equal = 0;
      for (i = 0; i < n_specs; i++)
	{
	  if (i == 0)
	    format_address (current_offset, '\0');
	  else
	    printf ("%*s", address_pad_len, "");
	  (*spec[i].print_function) (n_bytes, curr_block, spec[i].fmt_string);
	  if (spec[i].hexl_mode_trailer)
	    {
	      /* space-pad out to full line width, then dump the trailer */
	      int datum_width = width_bytes[spec[i].size];
	      int blank_fields = (bytes_per_block - n_bytes) / datum_width;
	      int field_width = spec[i].field_width + 1;
	      printf ("%*s", blank_fields * field_width, "");
	      dump_hexl_mode_trailer (n_bytes, curr_block);
	    }
	  putchar ('\n');
	}
    }
  first = 0;
}

/* Read a single byte into *C from the concatenation of the input files
   named in the global array FILE_LIST.  On the first call to this
   function, the global variable IN_STREAM is expected to be an open
   stream associated with the input file INPUT_FILENAME.  If IN_STREAM
   is at end-of-file, close it and update the global variables IN_STREAM
   and INPUT_FILENAME so they correspond to the next file in the list.
   Then try to read a byte from the newly opened file.  Repeat if
   necessary until EOF is reached for the last file in FILE_LIST, then
   set *C to EOF and return.  Subsequent calls do likewise.  The return
   value is nonzero if any errors occured, zero otherwise.  */

static int
read_char (int *c)
{
  int err = 0;

  *c = EOF;

  while (in_stream != NULL)	/* EOF.  */
    {
      *c = fgetc (in_stream);

      if (*c != EOF)
	break;

      err |= check_and_close ();

      err |= open_next_file ();
    }

  return err;
}

/* Read N bytes into BLOCK from the concatenation of the input files
   named in the global array FILE_LIST.  On the first call to this
   function, the global variable IN_STREAM is expected to be an open
   stream associated with the input file INPUT_FILENAME.  If all N
   bytes cannot be read from IN_STREAM, close IN_STREAM and update
   the global variables IN_STREAM and INPUT_FILENAME.  Then try to
   read the remaining bytes from the newly opened file.  Repeat if
   necessary until EOF is reached for the last file in FILE_LIST.
   On subsequent calls, don't modify BLOCK and return zero.  Set
   *N_BYTES_IN_BUFFER to the number of bytes read.  If an error occurs,
   it will be detected through ferror when the stream is about to be
   closed.  If there is an error, give a message but continue reading
   as usual and return nonzero.  Otherwise return zero.  */

static int
read_block (size_t n, char *block, size_t *n_bytes_in_buffer)
{
  int err = 0;

  assert (0 < n && n <= bytes_per_block);

  *n_bytes_in_buffer = 0;

  if (n == 0)
    return 0;

  while (in_stream != NULL)	/* EOF.  */
    {
      size_t n_needed;
      size_t n_read;

      n_needed = n - *n_bytes_in_buffer;
      n_read = fread (block + *n_bytes_in_buffer, 1, n_needed, in_stream);

      *n_bytes_in_buffer += n_read;

      if (n_read == n_needed)
	break;

      err |= check_and_close ();

      err |= open_next_file ();
    }

  return err;
}

/* Return the least common multiple of the sizes associated
   with the format specs.  */

static int
get_lcm (void)
{
  size_t i;
  int l_c_m = 1;

  for (i = 0; i < n_specs; i++)
    l_c_m = lcm (l_c_m, width_bytes[(int) spec[i].size]);
  return l_c_m;
}

/* If S is a valid traditional offset specification with an optional
   leading '+' return nonzero and set *OFFSET to the offset it denotes.  */

static int
parse_old_offset (const char *s, uintmax_t *offset)
{
  int radix;
  enum strtol_error s_err;

  if (*s == '\0')
    return 0;

  /* Skip over any leading '+'. */
  if (s[0] == '+')
    ++s;

  /* Determine the radix we'll use to interpret S.  If there is a `.',
     it's decimal, otherwise, if the string begins with `0X'or `0x',
     it's hexadecimal, else octal.  */
  if (strchr (s, '.') != NULL)
    radix = 10;
  else
    {
      if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
	radix = 16;
      else
	radix = 8;
    }

  s_err = xstrtoumax (s, NULL, radix, offset, "Bb");
  if (s_err != LONGINT_OK)
    {
      STRTOL_FAIL_WARN (s, _("old-style offset"), s_err);
      return 0;
    }
  return 1;
}

/* Read a chunk of size BYTES_PER_BLOCK from the input files, write the
   formatted block to standard output, and repeat until the specified
   maximum number of bytes has been read or until all input has been
   processed.  If the last block read is smaller than BYTES_PER_BLOCK
   and its size is not a multiple of the size associated with a format
   spec, extend the input block with zero bytes until its length is a
   multiple of all format spec sizes.  Write the final block.  Finally,
   write on a line by itself the offset of the byte after the last byte
   read.  Accumulate return values from calls to read_block and
   check_and_close, and if any was nonzero, return nonzero.
   Otherwise, return zero.  */

static int
dump (void)
{
  char *block[2];
  uintmax_t current_offset;
  int idx;
  int err;
  size_t n_bytes_read;

  block[0] = (char *) alloca (bytes_per_block);
  block[1] = (char *) alloca (bytes_per_block);

  current_offset = n_bytes_to_skip;

  idx = 0;
  err = 0;
  if (limit_bytes_to_format)
    {
      while (1)
	{
	  size_t n_needed;
	  if (current_offset >= end_offset)
	    {
	      n_bytes_read = 0;
	      break;
	    }
	  n_needed = MIN (end_offset - current_offset,
			  (uintmax_t) bytes_per_block);
	  err |= read_block (n_needed, block[idx], &n_bytes_read);
	  if (n_bytes_read < bytes_per_block)
	    break;
	  assert (n_bytes_read == bytes_per_block);
	  write_block (current_offset, n_bytes_read,
		       block[!idx], block[idx]);
	  current_offset += n_bytes_read;
	  idx = !idx;
	}
    }
  else
    {
      while (1)
	{
	  err |= read_block (bytes_per_block, block[idx], &n_bytes_read);
	  if (n_bytes_read < bytes_per_block)
	    break;
	  assert (n_bytes_read == bytes_per_block);
	  write_block (current_offset, n_bytes_read,
		       block[!idx], block[idx]);
	  current_offset += n_bytes_read;
	  idx = !idx;
	}
    }

  if (n_bytes_read > 0)
    {
      int l_c_m;
      size_t bytes_to_write;

      l_c_m = get_lcm ();

      /* Make bytes_to_write the smallest multiple of l_c_m that
	 is at least as large as n_bytes_read.  */
      bytes_to_write = l_c_m * ((n_bytes_read + l_c_m - 1) / l_c_m);

      memset (block[idx] + n_bytes_read, 0, bytes_to_write - n_bytes_read);
      write_block (current_offset, bytes_to_write,
		   block[!idx], block[idx]);
      current_offset += n_bytes_read;
    }

  format_address (current_offset, '\n');

  if (limit_bytes_to_format && current_offset >= end_offset)
    err |= check_and_close ();

  return err;
}

/* STRINGS mode.  Find each "string constant" in the input.
   A string constant is a run of at least `string_min' ASCII
   graphic (or formatting) characters terminated by a null.
   Based on a function written by Richard Stallman for a
   traditional version of od.  Return nonzero if an error
   occurs.  Otherwise, return zero.  */

static int
dump_strings (void)
{
  size_t bufsize = MAX (100, string_min);
  char *buf = xmalloc (bufsize);
  uintmax_t address = n_bytes_to_skip;
  int err;

  err = 0;
  while (1)
    {
      size_t i;
      int c;

      /* See if the next `string_min' chars are all printing chars.  */
    tryline:

      if (limit_bytes_to_format
	  && (end_offset < string_min || end_offset - string_min <= address))
	break;

      for (i = 0; i < string_min; i++)
	{
	  err |= read_char (&c);
	  address++;
	  if (c < 0)
	    {
	      free (buf);
	      return err;
	    }
	  if (!ISPRINT (c))
	    /* Found a non-printing.  Try again starting with next char.  */
	    goto tryline;
	  buf[i] = c;
	}

      /* We found a run of `string_min' printable characters.
	 Now see if it is terminated with a null byte.  */
      while (!limit_bytes_to_format || address < end_offset)
	{
	  if (i == bufsize)
	    {
	      bufsize = 1 + 3 * bufsize / 2;
	      buf = xrealloc (buf, bufsize);
	    }
	  err |= read_char (&c);
	  address++;
	  if (c < 0)
	    {
	      free (buf);
	      return err;
	    }
	  if (c == '\0')
	    break;		/* It is; print this string.  */
	  if (!ISPRINT (c))
	    goto tryline;	/* It isn't; give up on this string.  */
	  buf[i++] = c;		/* String continues; store it all.  */
	}

      /* If we get here, the string is all printable and null-terminated,
	 so print it.  It is all in `buf' and `i' is its length.  */
      buf[i] = 0;
      format_address (address - i - 1, ' ');

      for (i = 0; (c = buf[i]); i++)
	{
	  switch (c)
	    {
	    case '\007':
	      fputs ("\\a", stdout);
	      break;

	    case '\b':
	      fputs ("\\b", stdout);
	      break;

	    case '\f':
	      fputs ("\\f", stdout);
	      break;

	    case '\n':
	      fputs ("\\n", stdout);
	      break;

	    case '\r':
	      fputs ("\\r", stdout);
	      break;

	    case '\t':
	      fputs ("\\t", stdout);
	      break;

	    case '\v':
	      fputs ("\\v", stdout);
	      break;

	    default:
	      putc (c, stdout);
	    }
	}
      putchar ('\n');
    }

  /* We reach this point only if we search through
     (max_bytes_to_format - string_min) bytes before reaching EOF.  */

  free (buf);

  err |= check_and_close ();
  return err;
}

int
main (int argc, char **argv)
{
  int c;
  int n_files;
  size_t i;
  int l_c_m;
  size_t desired_width IF_LINT (= 0);
  int width_specified = 0;
  int n_failed_decodes = 0;
  int err;
  char const *short_options = (posix2_version () < 200112
			       ? COMMON_SHORT_OPTIONS "s::w::"
			       : COMMON_SHORT_OPTIONS "s:w:");

  /* The old-style `pseudo starting address' to be printed in parentheses
     after any true address.  */
  uintmax_t pseudo_start IF_LINT (= 0);

  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  err = 0;

  for (i = 0; i <= MAX_INTEGRAL_TYPE_SIZE; i++)
    integral_type_size[i] = NO_SIZE;

  integral_type_size[sizeof (char)] = CHAR;
  integral_type_size[sizeof (short int)] = SHORT;
  integral_type_size[sizeof (int)] = INT;
  integral_type_size[sizeof (long int)] = LONG;
#if HAVE_UNSIGNED_LONG_LONG
  /* If `long' and `long long' have the same size, it's fine
     to overwrite the entry for `long' with this one.  */
  integral_type_size[sizeof (ulonglong_t)] = LONG_LONG;
#endif

  for (i = 0; i <= MAX_FP_TYPE_SIZE; i++)
    fp_type_size[i] = NO_SIZE;

  fp_type_size[sizeof (float)] = FLOAT_SINGLE;
  /* The array entry for `double' is filled in after that for LONG_DOUBLE
     so that if `long double' is the same type or if long double isn't
     supported FLOAT_LONG_DOUBLE will never be used.  */
  fp_type_size[sizeof (LONG_DOUBLE)] = FLOAT_LONG_DOUBLE;
  fp_type_size[sizeof (double)] = FLOAT_DOUBLE;

  n_specs = 0;
  n_specs_allocated = 5;
  spec = (struct tspec *) xmalloc (n_specs_allocated * sizeof (struct tspec));

  format_address = format_address_std;
  address_base = 8;
  address_pad_len = 7;
  flag_dump_strings = 0;

  while ((c = getopt_long (argc, argv, short_options, long_options, NULL))
	 != -1)
    {
      uintmax_t tmp;
      enum strtol_error s_err;

      switch (c)
	{
	case 0:
	  break;

	case 'A':
	  switch (optarg[0])
	    {
	    case 'd':
	      format_address = format_address_std;
	      address_base = 10;
	      address_pad_len = 7;
	      break;
	    case 'o':
	      format_address = format_address_std;
	      address_base = 8;
	      address_pad_len = 7;
	      break;
	    case 'x':
	      format_address = format_address_std;
	      address_base = 16;
	      address_pad_len = 6;
	      break;
	    case 'n':
	      format_address = format_address_none;
	      address_pad_len = 0;
	      break;
	    default:
	      error (EXIT_FAILURE, 0,
		     _("invalid output address radix `%c'; \
it must be one character from [doxn]"),
		     optarg[0]);
	      break;
	    }
	  break;

	case 'j':
	  s_err = xstrtoumax (optarg, NULL, 0, &n_bytes_to_skip, "bkm");
	  if (s_err != LONGINT_OK)
	    STRTOL_FATAL_ERROR (optarg, _("skip argument"), s_err);
	  break;

	case 'N':
	  limit_bytes_to_format = 1;

	  s_err = xstrtoumax (optarg, NULL, 0, &max_bytes_to_format, "bkm");
	  if (s_err != LONGINT_OK)
	    STRTOL_FATAL_ERROR (optarg, _("limit argument"), s_err);
	  break;

	case 's':
	  if (optarg == NULL)
	    string_min = 3;
	  else
	    {
	      s_err = xstrtoumax (optarg, NULL, 0, &tmp, "bkm");
	      if (s_err != LONGINT_OK)
		STRTOL_FATAL_ERROR (optarg, _("minimum string length"), s_err);

	      /* The minimum string length may be no larger than SIZE_MAX,
		 since we may allocate a buffer of this size.  */
	      if (SIZE_MAX < tmp)
		error (EXIT_FAILURE, 0, _("%s is too large"), optarg);

	      string_min = tmp;
	    }
	  flag_dump_strings = 1;
	  break;

	case 't':
	  if (decode_format_string (optarg))
	    ++n_failed_decodes;
	  break;

	case 'v':
	  abbreviate_duplicate_blocks = 0;
	  break;

	case TRADITIONAL_OPTION:
	  traditional = 1;
	  break;

	  /* The next several cases map the traditional format
	     specification options to the corresponding modern format
	     specs.  GNU od accepts any combination of old- and
	     new-style options.  Format specification options accumulate.  */

#define CASE_OLD_ARG(old_char,new_string)		\
	case old_char:					\
	  {						\
	    if (decode_format_string (new_string))	\
	      ++n_failed_decodes;			\
	  }						\
	  break

	  CASE_OLD_ARG ('a', "a");
	  CASE_OLD_ARG ('b', "oC");
	  CASE_OLD_ARG ('c', "c");
	  CASE_OLD_ARG ('d', "u2");
	  CASE_OLD_ARG ('f', "fF");
	  CASE_OLD_ARG ('h', "x2");
	  CASE_OLD_ARG ('i', "d2");
	  CASE_OLD_ARG ('l', "d4");
	  CASE_OLD_ARG ('o', "o2");
	  CASE_OLD_ARG ('x', "x2");

	  /* FIXME: POSIX 1003.1-2001 with XSI requires this:

	     CASE_OLD_ARG ('s', "d2");

	     for the traditional syntax, but this conflicts with case
	     's' above. */

#undef CASE_OLD_ARG

	case 'w':
	  width_specified = 1;
	  if (optarg == NULL)
	    {
	      desired_width = 32;
	    }
	  else
	    {
	      uintmax_t w_tmp;
	      s_err = xstrtoumax (optarg, NULL, 10, &w_tmp, "");
	      if (s_err != LONGINT_OK)
		STRTOL_FATAL_ERROR (optarg, _("width specification"), s_err);
	      if (SIZE_MAX < w_tmp)
		error (EXIT_FAILURE, 0, _("%s is too large"), optarg);
	      desired_width = w_tmp;
	    }
	  break;

	case_GETOPT_HELP_CHAR;

	case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);

	default:
	  usage (EXIT_FAILURE);
	  break;
	}
    }

  if (n_failed_decodes > 0)
    exit (EXIT_FAILURE);

  if (flag_dump_strings && n_specs > 0)
    error (EXIT_FAILURE, 0,
	   _("no type may be specified when dumping strings"));

  n_files = argc - optind;

  /* If the --traditional option is used, there may be from
     0 to 3 remaining command line arguments;  handle each case
     separately.
	od [file] [[+]offset[.][b] [[+]label[.][b]]]
     The offset and pseudo_start have the same syntax.

     FIXME: POSIX 1003.1-2001 with XSI requires support for the
     traditional syntax even if --traditional is not given.  */

  if (traditional)
    {
      uintmax_t offset;

      if (n_files == 1)
	{
	  if (parse_old_offset (argv[optind], &offset))
	    {
	      n_bytes_to_skip = offset;
	      --n_files;
	      ++argv;
	    }
	}
      else if (n_files == 2)
	{
	  uintmax_t o1, o2;
	  if (parse_old_offset (argv[optind], &o1)
	      && parse_old_offset (argv[optind + 1], &o2))
	    {
	      n_bytes_to_skip = o1;
	      flag_pseudo_start = 1;
	      pseudo_start = o2;
	      argv += 2;
	      n_files -= 2;
	    }
	  else if (parse_old_offset (argv[optind + 1], &o2))
	    {
	      n_bytes_to_skip = o2;
	      --n_files;
	      argv[optind + 1] = argv[optind];
	      ++argv;
	    }
	  else
	    {
	      error (0, 0,
		     _("invalid second operand in compatibility mode `%s'"),
		     argv[optind + 1]);
	      usage (EXIT_FAILURE);
	    }
	}
      else if (n_files == 3)
	{
	  uintmax_t o1, o2;
	  if (parse_old_offset (argv[optind + 1], &o1)
	      && parse_old_offset (argv[optind + 2], &o2))
	    {
	      n_bytes_to_skip = o1;
	      flag_pseudo_start = 1;
	      pseudo_start = o2;
	      argv[optind + 2] = argv[optind];
	      argv += 2;
	      n_files -= 2;
	    }
	  else
	    {
	      error (0, 0,
	    _("in compatibility mode, the last two arguments must be offsets"));
	      usage (EXIT_FAILURE);
	    }
	}
      else if (n_files > 3)
	{
	  error (0, 0,
		 _("compatibility mode supports at most three arguments"));
	  usage (EXIT_FAILURE);
	}

      if (flag_pseudo_start)
	{
	  if (format_address == format_address_none)
	    {
	      address_base = 8;
	      address_pad_len = 7;
	      format_address = format_address_paren;
	    }
	  else
	    format_address = format_address_label;
	}
    }

  if (limit_bytes_to_format)
    {
      end_offset = n_bytes_to_skip + max_bytes_to_format;
      if (end_offset < n_bytes_to_skip)
	error (EXIT_FAILURE, 0, "skip-bytes + read-bytes is too large");
    }

  if (n_specs == 0)
    {
      if (decode_one_format ("o2", "o2", NULL, &(spec[0])))
	{
	  /* This happens on Cray systems that don't have a 2-byte
	     integral type.  */
	  exit (EXIT_FAILURE);
	}

      n_specs = 1;
    }

  if (n_files > 0)
    {
      /* Set the global pointer FILE_LIST so that it
	 references the first file-argument on the command-line.  */

      file_list = (char const *const *) &argv[optind];
    }
  else
    {
      /* No files were listed on the command line.
	 Set the global pointer FILE_LIST so that it
	 references the null-terminated list of one name: "-".  */

      file_list = default_file_list;
    }

  /* open the first input file */
  err |= open_next_file ();
  if (in_stream == NULL)
    goto cleanup;

  /* skip over any unwanted header bytes */
  err |= skip (n_bytes_to_skip);
  if (in_stream == NULL)
    goto cleanup;

  pseudo_offset = (flag_pseudo_start ? pseudo_start - n_bytes_to_skip : 0);

  /* Compute output block length.  */
  l_c_m = get_lcm ();

  if (width_specified)
    {
      if (desired_width != 0 && desired_width % l_c_m == 0)
	bytes_per_block = desired_width;
      else
	{
	  error (0, 0, _("warning: invalid width %lu; using %d instead"),
		 (unsigned long) desired_width, l_c_m);
	  bytes_per_block = l_c_m;
	}
    }
  else
    {
      if (l_c_m < DEFAULT_BYTES_PER_BLOCK)
	bytes_per_block = l_c_m * (DEFAULT_BYTES_PER_BLOCK / l_c_m);
      else
	bytes_per_block = l_c_m;
    }

#ifdef DEBUG
  for (i = 0; i < n_specs; i++)
    {
      printf (_("%d: fmt=\"%s\" width=%d\n"),
	      i, spec[i].fmt_string, width_bytes[spec[i].size]);
    }
#endif

  err |= (flag_dump_strings ? dump_strings () : dump ());

cleanup:;

  if (have_read_stdin && fclose (stdin) == EOF)
    error (EXIT_FAILURE, errno, _("standard input"));

  exit (err == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}
