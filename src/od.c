/* od -- dump files in octal and other formats
   Copyright (C) 1992-2025 Free Software Foundation, Inc.

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

/* Written by Jim Meyering.  */

#include <config.h>

#include <ctype.h>
#include <endian.h>
#include <float.h>
#include <stdio.h>
#include <getopt.h>
#include <sys/types.h>
#include "system.h"
#include "argmatch.h"
#include "assure.h"
#include "c-ctype.h"
#include "ftoastr.h"
#include "quote.h"
#include "stat-size.h"
#include "xbinary-io.h"
#include "xprintf.h"
#include "xstrtol.h"
#include "xstrtol-error.h"

/* The official name of this program (e.g., no 'g' prefix).  */
#define PROGRAM_NAME "od"

#define AUTHORS proper_name ("Jim Meyering")

/* The default number of input bytes per output line.  */
#define DEFAULT_BYTES_PER_BLOCK 16

#if FLOAT16_SUPPORTED
  /* Available since clang 6 (2018), and gcc 7 (2017).  */
  typedef _Float16 float16;
#else
# define FLOAT16_SUPPORTED 0
  /* This is just a place-holder to avoid a few '#if' directives.
     In this case, the type isn't actually used.  */
  typedef float float16;
#endif

#if BF16_SUPPORTED
  /* Available since clang 11 (2020), and gcc 13 (2023). */
  typedef __bf16 bfloat16;
#else
# define BF16_SUPPORTED 0
  /* This is just a place-holder to avoid a few '#if' directives.
     In this case, the type isn't actually used.  */
  typedef float bfloat16;
#endif

enum size_spec
  {
    NO_SIZE = 0,
    CHAR,
    SHORT = CHAR + (UCHAR_MAX < USHRT_MAX),
    INT = SHORT + (USHRT_MAX < UINT_MAX),
    LONG = INT + (UINT_MAX < ULONG_MAX),
    LONG_LONG = LONG + (ULONG_MAX < ULLONG_MAX),
    INTMAX = LONG_LONG + (ULLONG_MAX < UINTMAX_MAX),
    FLOAT_HALF, /* Used only if (FLOAT16_SUPPORTED || BF16_SUPPORTED).  */
    FLOAT_SINGLE = FLOAT_HALF + (FLOAT16_SUPPORTED || BF16_SUPPORTED),
    FLOAT_DOUBLE = FLOAT_SINGLE + (FLT_MANT_DIG < DBL_MANT_DIG
                                   || FLT_MAX_EXP < DBL_MAX_EXP),
    FLOAT_LONG_DOUBLE = FLOAT_DOUBLE + (DBL_MANT_DIG < LDBL_MANT_DIG
                                        || DBL_MAX_EXP < LDBL_MAX_EXP),
    N_SIZE_SPECS
  };

enum output_format
  {
    SIGNED_DECIMAL,
    UNSIGNED_DECIMAL,
    OCTAL,
    HEXADECIMAL,
    FLOATING_POINT,
    HFLOATING_POINT,
    BFLOATING_POINT,
    NAMED_CHARACTER,
    CHARACTER
  };

enum { MAX_INTEGRAL_TYPE_WIDTH = UINTMAX_WIDTH };

/* The maximum number of bytes needed for a format string, including
   the trailing nul.  Each format string expects a variable amount of
   padding (guaranteed to be at least 1 plus the field width), then an
   element that will be formatted in the field.  */
enum
  {
    FMT_BYTES_ALLOCATED =
      sizeof "%*.%dlld" - sizeof "%d" + INT_STRLEN_BOUND (int) + 1
  };

/* Ensure that our choice for FMT_BYTES_ALLOCATED is reasonable.  */
static_assert (MAX_INTEGRAL_TYPE_WIDTH <= 3 * 99);

/* The type of a print function.  FIELDS is the number of fields per
   line, BLANK is the number of fields to leave blank.  WIDTH is width
   of one field, excluding leading space, and PAD is total pad to
   divide among FIELDS.  PAD is at least as large as FIELDS.  */
typedef void (*print_function_type)
  (idx_t fields, idx_t blank, void const *data,
   char const *fmt, int width, idx_t pad);

/* Each output format specification (from '-t spec' or from
   old-style options) is represented by one of these structures.  */
struct tspec
  {
    enum output_format fmt;
    enum size_spec size; /* Type of input object.  */
    print_function_type print_function;
    char fmt_string[FMT_BYTES_ALLOCATED]; /* Of the style "%*d".  */
    bool hexl_mode_trailer;
    int field_width; /* Minimum width of a field, excluding leading space.  */
    idx_t pad_width; /* Total padding to be divided among fields.  */
  };

/* Convert enum size_spec to the size of the named type.  */
static const int width_bytes[] =
{
  -1,
  sizeof (unsigned char),
#if UCHAR_MAX < USHRT_MAX
  sizeof (unsigned short int),
#endif
#if USHRT_MAX < UINT_MAX
  sizeof (unsigned int),
#endif
#if UINT_MAX < ULONG_MAX
  sizeof (unsigned long int),
#endif
#if ULONG_MAX < ULLONG_MAX
  sizeof (unsigned long long int),
#endif
#if ULLONG_MAX < UINTMAX_MAX
  sizeof (uintmax_t),
#endif
#if BF16_SUPPORTED
  sizeof (bfloat16),
#elif FLOAT16_SUPPORTED
  sizeof (float16),
#endif
#if FLT_MANT_DIG < DBL_MANT_DIG || FLT_MAX_EXP < DBL_MAX_EXP
  sizeof (float),
#endif
  sizeof (double),
#if DBL_MANT_DIG < LDBL_MANT_DIG || DBL_MAX_EXP < LDBL_MAX_EXP
  sizeof (long double),
#endif
};

/* Ensure that for each member of 'enum size_spec' there is an
   initializer in the width_bytes array.  */
static_assert (countof (width_bytes) == N_SIZE_SPECS);

/* Names for some non-printing characters.  */
static char const charname[33][4] =
{
  "nul", "soh", "stx", "etx", "eot", "enq", "ack", "bel",
  "bs", "ht", "nl", "vt", "ff", "cr", "so", "si",
  "dle", "dc1", "dc2", "dc3", "dc4", "nak", "syn", "etb",
  "can", "em", "sub", "esc", "fs", "gs", "rs", "us",
  "sp"
};

/* Address base (8, 10 or 16).  */
static int address_base = 8;

/* The number of octal digits required to represent the largest
   address value.  */
enum { MAX_ADDRESS_LENGTH = ((INTMAX_WIDTH - 1) / 3
                             + ((INTMAX_WIDTH - 1) % 3 != 0)) };

/* Width of a normal address.  */
static int address_pad_len = 7;

/* Minimum length when detecting --strings.  */
static idx_t string_min;

/* True when in --strings mode.  */
static bool flag_dump_strings;

/* True if we should recognize the older non-option arguments
   that specified at most one file and optional arguments specifying
   offset and pseudo-start address.  */
static bool traditional;

/* True if an old-style 'pseudo-address' was specified.  */
static bool flag_pseudo_start;

/* The difference between the old-style pseudo starting address and
   the number of bytes to skip.  */
static intmax_t pseudo_offset;

/* Function that accepts an address and an optional following char,
   and prints the address and char to stdout.  */
static void format_address_std (intmax_t, char);
static void (*format_address) (intmax_t, char) = format_address_std;

/* The number of input bytes to skip before formatting and writing.  */
static intmax_t n_bytes_to_skip;

/* The offset of the first byte after the last byte to be formatted.
   If negative, there is no limit.  */
static intmax_t end_offset = -1;

/* When true and two or more consecutive blocks are equal, format
   only the first block and output an asterisk alone on the following
   line to indicate that identical blocks have been elided.  */
static bool abbreviate_duplicate_blocks = true;

/* An array of specs describing how to format each input block.  */
static struct tspec *spec;

/* The number of format specs.  */
static idx_t n_specs;

/* The allocated length of SPEC.  */
static idx_t n_specs_allocated;

/* The number of input bytes formatted per output line.  It must be
   a multiple of the least common multiple of the sizes associated with
   the specified output types.  It should be as large as possible, but
   no larger than 16 -- unless specified with the -w option.  */
static idx_t bytes_per_block;

/* Human-readable representation of *file_list (for error messages).
   It differs from file_list[-1] only when file_list[-1] is "-".  */
static char const *input_filename;

/* A null-terminated list of the file-arguments from the command line.  */
static char const *const *file_list;

/* Initializer for file_list if no file-arguments
   were specified on the command line.  */
static char const *const default_file_list[] = {"-", nullptr};

/* The input stream associated with the current file.  */
static FILE *in_stream;

/* If true, at least one of the files we read was standard input.  */
static bool have_read_stdin;

/* Map the size in bytes to a type identifier.
   When two types have the same machine layout:
     - Prefer unsigned int to other types, as its format is shorter.
     - Prefer unsigned long to higher-ranked types, as it is older.
     - Prefer uintmax_t to unsigned long long int; this wins if %lld
       does not work but %jd does (e.g., MS-Windows).  */
static enum size_spec const integral_type_size[] =
  {
#if UCHAR_MAX < USHRT_MAX
    [sizeof (unsigned char)] = CHAR,
#endif
#if USHRT_MAX < UINT_MAX
    [sizeof (unsigned short int)] = SHORT,
#endif
    [sizeof (unsigned int)] = INT,
#if UINT_MAX < ULONG_MAX
    [sizeof (unsigned long int)] = LONG,
#endif
#if ULONG_MAX < ULLONG_MAX && ULLONG_MAX < UINTMAX_MAX
    [sizeof (unsigned long long int)] = LONG_LONG,
#endif
#if ULONG_MAX < UINTMAX_MAX
    [sizeof (uintmax_t)] = INTMAX,
#endif
  };

/* Map the size in bytes to a floating type identifier.
   When two types have the same machine layout:
     - Prefer double to the other types, as its format is shorter.  */
static enum size_spec const fp_type_size[] =
  {
#if FLOAT16_SUPPORTED
    [sizeof (float16)] = FLOAT_HALF,
#elif BF16_SUPPORTED
    [sizeof (bfloat16)] = FLOAT_HALF,
#endif
#if FLT_MANT_DIG < DBL_MANT_DIG || FLT_MAX_EXP < DBL_MAX_EXP
    [sizeof (float)] = FLOAT_SINGLE,
#endif
    [sizeof (double)] = FLOAT_DOUBLE,
#if DBL_MANT_DIG < LDBL_MANT_DIG || DBL_MAX_EXP < LDBL_MAX_EXP
    [sizeof (long double)] = FLOAT_LONG_DOUBLE,
#endif
  };

/* Use native endianness by default.  */
static bool input_swap;

static char const short_options[] = "A:aBbcDdeFfHhIij:LlN:OoS:st:vw::Xx";

/* For long options that have no equivalent short option, use a
   non-character as a pseudo short option, starting with CHAR_MAX + 1.  */
enum
{
  TRADITIONAL_OPTION = CHAR_MAX + 1,
  ENDIAN_OPTION,
};

enum endian_type
{
  endian_little,
  endian_big
};

static char const *const endian_args[] =
{
  "little", "big", nullptr
};

static enum endian_type const endian_types[] =
{
  endian_little, endian_big
};

static struct option const long_options[] =
{
  {"skip-bytes", required_argument, nullptr, 'j'},
  {"address-radix", required_argument, nullptr, 'A'},
  {"read-bytes", required_argument, nullptr, 'N'},
  {"format", required_argument, nullptr, 't'},
  {"output-duplicates", no_argument, nullptr, 'v'},
  {"strings", optional_argument, nullptr, 'S'},
  {"traditional", no_argument, nullptr, TRADITIONAL_OPTION},
  {"width", optional_argument, nullptr, 'w'},
  {"endian", required_argument, nullptr, ENDIAN_OPTION },

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
Usage: %s [OPTION]... [FILE]...\n\
  or:  %s [-abcdfilosx]... [FILE] [[+]OFFSET[.][b]]\n\
  or:  %s --traditional [OPTION]... [FILE] [[+]OFFSET[.][b] [+][LABEL][.][b]]\n\
"),
              program_name, program_name, program_name);
      fputs (_("\n\
Write an unambiguous representation, octal bytes by default,\n\
of FILE to standard output.  With more than one FILE argument,\n\
concatenate them in the listed order to form the input.\n\
"), stdout);

      emit_stdin_note ();

      fputs (_("\
\n\
If first and second call formats both apply, the second format is assumed\n\
if the last operand begins with + or (if there are 2 operands) a digit.\n\
An OFFSET operand means -j OFFSET.  LABEL is the pseudo-address\n\
at first byte printed, incremented when dump is progressing.\n\
For OFFSET and LABEL, a 0x or 0X prefix indicates hexadecimal;\n\
suffixes may be . for octal and b for multiply by 512.\n\
"), stdout);

      emit_mandatory_arg_note ();

      fputs (_("\
  -A, --address-radix=RADIX   output format for file offsets; RADIX is one\n\
                                of [doxn], for Decimal, Octal, Hex or None\n\
      --endian={big|little}   swap input bytes according the specified order\n\
  -j, --skip-bytes=BYTES      skip BYTES input bytes first\n\
"), stdout);
      fputs (_("\
  -N, --read-bytes=BYTES      limit dump to BYTES input bytes\n\
  -S BYTES, --strings[=BYTES]  show only NUL terminated strings\n\
                                of at least BYTES (3) printable characters\n\
  -t, --format=TYPE           select output format or formats\n\
  -v, --output-duplicates     do not use * to mark line suppression\n\
  -w[BYTES], --width[=BYTES]  output BYTES bytes per output line;\n\
                                32 is implied when BYTES is not specified\n\
      --traditional           accept arguments in third form above\n\
"), stdout);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      fputs (_("\
\n\
\n\
Traditional format specifications may be intermixed; they accumulate:\n\
  -a   same as -t a,  select named characters, ignoring high-order bit\n\
  -b   same as -t o1, select octal bytes\n\
  -c   same as -t c,  select printable characters or backslash escapes\n\
  -d   same as -t u2, select unsigned decimal 2-byte units\n\
"), stdout);
      fputs (_("\
  -f   same as -t fF, select floats\n\
  -i   same as -t dI, select decimal ints\n\
  -l   same as -t dL, select decimal longs\n\
  -o   same as -t o2, select octal 2-byte units\n\
  -s   same as -t d2, select decimal 2-byte units\n\
  -x   same as -t x2, select hexadecimal 2-byte units\n\
"), stdout);
      fputs (_("\
\n\
\n\
TYPE is made up of one or more of these specifications:\n\
  a          named character, ignoring high-order bit\n\
  c          printable character or backslash escape\n\
"), stdout);
      fputs (_("\
  d[SIZE]    signed decimal, SIZE bytes per integer\n\
  f[SIZE]    floating point, SIZE bytes per float\n\
  o[SIZE]    octal, SIZE bytes per integer\n\
  u[SIZE]    unsigned decimal, SIZE bytes per integer\n\
  x[SIZE]    hexadecimal, SIZE bytes per integer\n\
"), stdout);
      fputs (_("\
\n\
SIZE is a number.  For TYPE in [doux], SIZE may also be C for\n\
sizeof(char), S for sizeof(short), I for sizeof(int) or L for\n\
sizeof(long).  If TYPE is f, SIZE may also be B for Brain 16 bit,\n\
H for Half precision float, F for sizeof(float), D for sizeof(double),\n\
or L for sizeof(long double).\n\
"), stdout);
      fputs (_("\
\n\
Adding a z suffix to any type displays printable characters at the end of\n\
each output line.\n\
"), stdout);
      fputs (_("\
\n\
\n\
BYTES is hex with 0x or 0X prefix, and may have a multiplier suffix:\n\
  b    512\n\
  KB   1000\n\
  K    1024\n\
  MB   1000*1000\n\
  M    1024*1024\n\
and so on for G, T, P, E, Z, Y, R, Q.\n\
Binary prefixes can be used, too: KiB=K, MiB=M, and so on.\n\
"), stdout);
      emit_ancillary_info (PROGRAM_NAME);
    }
  exit (status);
}

/* Define the print functions.  */

/* Print N spaces, where 0 <= N.
   Do not rely on printf ("%*s", N, "") as N may exceed INT_MAX.  */
static void
print_n_spaces (intmax_t n)
{
  for (; 0 < n; n--)
    putchar (' ');
}

/* If there are FIELDS fields, return the total padding up to the
   start of field I, where I < FIELDS.  PAD is the total padding for
   all fields.  The result equals (PAD * I) / FIELDS, except it does
   not suffer from internal overflow.  */
static idx_t
pad_at (idx_t fields, idx_t i, idx_t pad)
{
  /* This implementation assumes that (FIELDS - 1)^2 does not overflow
     intmax_t, an assumption checked by pad_at_overflow.  */
  intmax_t m = pad % fields;
  return pad / fields * i + m * i / fields;
}
static bool
pad_at_overflow (idx_t fields)
{
  intmax_t product;
  return ckd_mul (&product, fields - 1, fields - 1);
}

#define PRINT_FIELDS(N, T, FMT_STRING_DECL, ACTION)                     \
static void                                                             \
N (idx_t fields, idx_t blank, void const *block,			\
   FMT_STRING_DECL, int width, idx_t pad)				\
{                                                                       \
  T const *p = block;                                                   \
  idx_t pad_remaining = pad;						\
  for (idx_t i = fields; blank < i; i--)				\
    {                                                                   \
      idx_t next_pad = pad_at (fields, i - 1, pad);			\
      int adjusted_width = pad_remaining - next_pad + width;            \
      T x;                                                              \
      if (input_swap && sizeof (T) > 1)                                 \
        {                                                               \
          union {                                                       \
            T x;                                                        \
            char b[sizeof (T)];                                         \
          } u;                                                          \
          for (idx_t j = 0; j < sizeof (T); j++)			\
            u.b[j] = ((char const *) p)[sizeof (T) - 1 - j];            \
          x = u.x;                                                      \
        }                                                               \
      else                                                              \
        x = *p;                                                         \
      p++;                                                              \
      ACTION;                                                           \
      pad_remaining = next_pad;                                         \
    }                                                                   \
}

#define PRINT_TYPE(N, T)                                                \
  PRINT_FIELDS (N, T, char const *fmt_string,                           \
                xprintf (fmt_string, adjusted_width, x))

#define PRINT_FLOATTYPE(N, T, FTOASTR, BUFSIZE)                         \
  PRINT_FIELDS (N, T, MAYBE_UNUSED char const *fmt_string,              \
                char buf[BUFSIZE];                                      \
                FTOASTR (buf, sizeof buf, 0, 0, x);                     \
                xprintf ("%*s", adjusted_width, buf))

PRINT_TYPE (print_s_char, signed char)
PRINT_TYPE (print_char, unsigned char)
PRINT_TYPE (print_s_short, short int)
PRINT_TYPE (print_short, unsigned short int)
PRINT_TYPE (print_int, unsigned int)
PRINT_TYPE (print_long, unsigned long int)
PRINT_TYPE (print_long_long, unsigned long long int)
PRINT_TYPE (print_intmax, uintmax_t)

PRINT_FLOATTYPE (print_bfloat, bfloat16, ftoastr, FLT_BUFSIZE_BOUND)
PRINT_FLOATTYPE (print_halffloat, float16, ftoastr, FLT_BUFSIZE_BOUND)
PRINT_FLOATTYPE (print_float, float, ftoastr, FLT_BUFSIZE_BOUND)
PRINT_FLOATTYPE (print_double, double, dtoastr, DBL_BUFSIZE_BOUND)
PRINT_FLOATTYPE (print_long_double, long double, ldtoastr, LDBL_BUFSIZE_BOUND)

#undef PRINT_TYPE
#undef PRINT_FLOATTYPE

static void
dump_hexl_mode_trailer (idx_t n_bytes, char const *block)
{
  fputs ("  >", stdout);
  for (idx_t i = n_bytes; i > 0; i--)
    {
      unsigned char c = *block++;
      unsigned char c2 = (isprint (c) ? c : '.');
      putchar (c2);
    }
  putchar ('<');
}

static void
print_named_ascii (idx_t fields, idx_t blank, void const *block,
                   MAYBE_UNUSED char const *unused_fmt_string,
                   int width, idx_t pad)
{
  unsigned char const *p = block;
  idx_t pad_remaining = pad;
  for (idx_t i = fields; blank < i; i--)
    {
      int masked_c = *p++ & 0x7f;
      char const *s;
      char buf[2];

      if (masked_c == 127)
        s = "del";
      else if (masked_c <= 040)
        s = charname[masked_c];
      else
        {
          buf[0] = masked_c;
          buf[1] = 0;
          s = buf;
        }

      idx_t next_pad = pad_at (fields, i - 1, pad);
      int adjusted_width = pad_remaining - next_pad + width;
      xprintf ("%*s", adjusted_width, s);
      pad_remaining = next_pad;
    }
}

static void
print_ascii (idx_t fields, idx_t blank, void const *block,
             MAYBE_UNUSED char const *unused_fmt_string, int width,
             idx_t pad)
{
  unsigned char const *p = block;
  idx_t pad_remaining = pad;
  for (idx_t i = fields; blank < i; i--)
    {
      unsigned char c = *p++;
      char const *s;
      char buf[4];

      switch (c)
        {
        case '\0':
          s = "\\0";
          break;

        case '\a':
          s = "\\a";
          break;

        case '\b':
          s = "\\b";
          break;

        case '\f':
          s = "\\f";
          break;

        case '\n':
          s = "\\n";
          break;

        case '\r':
          s = "\\r";
          break;

        case '\t':
          s = "\\t";
          break;

        case '\v':
          s = "\\v";
          break;

        default:
          sprintf (buf, (isprint (c) ? "%c" : "%03o"), c);
          s = buf;
        }

      idx_t next_pad = pad_at (fields, i - 1, pad);
      int adjusted_width = pad_remaining - next_pad + width;
      xprintf ("%*s", adjusted_width, s);
      pad_remaining = next_pad;
    }
}

/* Convert a null-terminated (possibly zero-length) string S to an
   int value.  If S points to a non-digit set *P to S,
   *VAL to 0, and return true.  Otherwise, accumulate the integer value of
   the string of digits.  If the string of digits represents a value
   larger than INT_MAX, don't modify *VAL or *P and return false.
   Otherwise, advance *P to the first non-digit after S, set *VAL to
   the result of the conversion and return true.  */

static bool
simple_strtoi (char const *s, char const **p, int *val)
{
  int sum;

  for (sum = 0; c_isdigit (*s); s++)
    if (ckd_mul (&sum, sum, 10) || ckd_add (&sum, sum, *s - '0'))
      return false;
  *p = s;
  *val = sum;
  return true;
}

/* Return a printf format for SPEC, given that remaining arguments are
   formats for int, long, long long, and intmax_t, respectively.  */
static char const *
ispec_to_format (enum size_spec size_spec,
                 char const *int_format,
                 char const *long_format,
                 char const *long_long_format,
                 char const *intmax_format)
{
  return (LONG < INTMAX && size_spec == INTMAX ? intmax_format
          : LONG < LONG_LONG && size_spec == LONG_LONG ? long_long_format
          : INT < LONG && size_spec == LONG ? long_format
          : int_format);
}

/* If S points to a single valid modern od format string, put
   a description of that format in *TSPEC, make *NEXT point at the
   character following the just-decoded format (if *NEXT is non-null),
   and return true.  If S is not valid, don't modify *NEXT or *TSPEC,
   give a diagnostic, and return false.  For example, if S were
   "d4afL" *NEXT would be set to "afL" and *TSPEC would be
     {
       fmt = SIGNED_DECIMAL;
       size = INT or LONG; (whichever integral_type_size[4] resolves to)
       print_function = print_int; (assuming size == INT)
       field_width = 11;
       fmt_string = "%*d";
      }
   pad_width is determined later, but is at least as large as the
   number of fields printed per row.
   S_ORIG is solely for reporting errors.  It should be the full format
   string argument.
   */

static bool ATTRIBUTE_NONNULL ()
decode_one_format (char const *s_orig, char const *s, char const **next,
                   struct tspec *tspec)
{
  enum size_spec size_spec;
  int size;
  enum output_format fmt;
  print_function_type print_function;
  char const *p;
  char c;
  int field_width;

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
          size = sizeof (unsigned char);
          break;

        case 'S':
          ++s;
          size = sizeof (unsigned short int);
          break;

        case 'I':
          ++s;
          size = sizeof (unsigned int);
          break;

        case 'L':
          ++s;
          size = sizeof (unsigned long int);
          break;

        default:
          if (! simple_strtoi (s, &p, &size))
            {
              /* The integer at P in S would overflow an int.
                 A digit string that long is sufficiently odd looking
                 that the following diagnostic is sufficient.  */
              error (0, 0, _("invalid type string %s"), quote (s_orig));
              return false;
            }
          if (p == s)
            size = sizeof (unsigned int);
          else
            {
              if (countof (integral_type_size) <= size
                  || integral_type_size[size] == NO_SIZE)
                {
                  error (0, 0, _("invalid type string %s;\nthis system"
                                 " doesn't provide a %d-byte integral type"),
                         quote (s_orig), size);
                  return false;
                }
              s = p;
            }
          break;
        }

      size_spec = integral_type_size[size];

      switch (c)
        {
        case 'd':
          fmt = SIGNED_DECIMAL;
          field_width = INT_BITS_STRLEN_BOUND (CHAR_BIT * size - 1) + 1;
          sprintf (tspec->fmt_string, "%%*%s",
                   ispec_to_format (size_spec, "d", "ld", "lld", "jd"));
          break;

        case 'o':
          fmt = OCTAL;
          field_width = (CHAR_BIT * size + 2) / 3;
          sprintf (tspec->fmt_string, "%%*.%d%s", field_width,
                   ispec_to_format (size_spec, "o", "lo", "llo", "jo"));
          break;

        case 'u':
          fmt = UNSIGNED_DECIMAL;
          field_width = INT_BITS_STRLEN_BOUND (CHAR_BIT * size);
          sprintf (tspec->fmt_string, "%%*%s",
                   ispec_to_format (size_spec, "u", "lu", "llu", "ju"));
          break;

        case 'x':
          fmt = HEXADECIMAL;
          field_width = (CHAR_BIT * size + 3) / 4;
          sprintf (tspec->fmt_string, "%%*.%d%s", field_width,
                   ispec_to_format (size_spec, "x", "lx", "llx", "jx"));
          break;

        default:
          unreachable ();
        }

      /* Prefer INT, prefer LONG to longer types,
         and prefer INTMAX to LONG_LONG.  */
      print_function
        = (size_spec == INT
           ? print_int
           : SHORT < INT && size_spec == SHORT
           ? (fmt == SIGNED_DECIMAL ? print_s_short : print_short)
           : CHAR < SHORT && size_spec == CHAR
           ? (fmt == SIGNED_DECIMAL ? print_s_char : print_char)
           : INT < LONG && size_spec == LONG
           ? print_long
           : LONG < INTMAX && size_spec == INTMAX
           ? print_intmax
           : LONG < LONG_LONG && LONG_LONG < INTMAX && size_spec == LONG_LONG
           ? print_long_long
           : (affirm (false), (print_function_type) nullptr));
      break;

    case 'f':
      fmt = FLOATING_POINT;
      ++s;
      switch (*s)
        {
        case 'B':
          ++s;
          fmt = BFLOATING_POINT;
          size = sizeof (bfloat16);
          break;

        case 'H':
          ++s;
          fmt = HFLOATING_POINT;
          size = sizeof (float16);
          break;

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
          size = sizeof (long double);
          break;

        default:
          if (! simple_strtoi (s, &p, &size))
            {
              /* The integer at P in S would overflow an int.
                 A digit string that long is sufficiently odd looking
                 that the following diagnostic is sufficient.  */
              error (0, 0, _("invalid type string %s"), quote (s_orig));
              return false;
            }
          if (p == s)
            size = sizeof (double);
          else
            {
              if (countof (fp_type_size) <= size
                  || fp_type_size[size] == NO_SIZE
                  || (! FLOAT16_SUPPORTED && BF16_SUPPORTED
                      && size == sizeof (bfloat16)))
                {
                  error (0, 0,
                         _("invalid type string %s;\n"
                           "this system doesn't provide a %d-byte"
                           " floating point type"),
                         quote (s_orig), size);
                  return false;
                }
              s = p;
            }
          break;
        }
      size_spec = fp_type_size[size];

      if ((! FLOAT16_SUPPORTED && fmt == HFLOATING_POINT)
          || (! BF16_SUPPORTED && fmt == BFLOATING_POINT))
      {
        error (0, 0,
               _("this system doesn't provide a %s floating point type"),
               quote (s_orig));
        return false;
      }

      {
        struct lconv const *locale = localeconv ();
        idx_t decimal_point_len =
          (locale->decimal_point[0] ? strlen (locale->decimal_point) : 1);

        if (size_spec == FLOAT_DOUBLE)
          {
            print_function = print_double;
            field_width = DBL_STRLEN_BOUND_L (decimal_point_len);
          }
        else if (FLOAT_SINGLE < FLOAT_DOUBLE && size_spec == FLOAT_SINGLE)
          {
            print_function = print_float;
            field_width = FLT_STRLEN_BOUND_L (decimal_point_len);
          }
        else if (FLOAT_HALF < FLOAT_SINGLE && size_spec == FLOAT_HALF)
          {
            print_function = fmt == BFLOATING_POINT
                             ? print_bfloat : print_halffloat;
            field_width = FLT_STRLEN_BOUND_L (decimal_point_len);
          }
        else if (FLOAT_DOUBLE < FLOAT_LONG_DOUBLE
                 && size_spec == FLOAT_LONG_DOUBLE)
          {
            print_function = print_long_double;
            field_width = LDBL_STRLEN_BOUND_L (decimal_point_len);
          }
        else
          affirm (false);

        break;
      }

    case 'a':
      ++s;
      fmt = NAMED_CHARACTER;
      size_spec = CHAR;
      print_function = print_named_ascii;
      field_width = 3;
      break;

    case 'c':
      ++s;
      fmt = CHARACTER;
      size_spec = CHAR;
      print_function = print_ascii;
      field_width = 3;
      break;

    default:
      error (0, 0, _("invalid character '%c' in type string %s"),
             *s, quote (s_orig));
      return false;
    }

  tspec->size = size_spec;
  tspec->fmt = fmt;
  tspec->print_function = print_function;

  tspec->field_width = field_width;
  tspec->hexl_mode_trailer = (*s == 'z');
  if (tspec->hexl_mode_trailer)
    s++;

  *next = s;
  return true;
}

/* Given a list of one or more input filenames FILE_LIST, set the global
   file pointer IN_STREAM and the global string INPUT_FILENAME to the
   first one that can be successfully opened. Modify FILE_LIST to
   reference the next filename in the list.  A file name of "-" is
   interpreted as standard input.  If any file open fails, give an error
   message and return false.  */

static bool
open_next_file (void)
{
  bool ok = true;

  do
    {
      input_filename = *file_list;
      if (input_filename == nullptr)
        return ok;
      ++file_list;

      if (streq (input_filename, "-"))
        {
          input_filename = _("standard input");
          in_stream = stdin;
          have_read_stdin = true;
          xset_binary_mode (STDIN_FILENO, O_BINARY);
        }
      else
        {
          in_stream = fopen (input_filename, (O_BINARY ? "rb" : "r"));
          if (in_stream == nullptr)
            {
              error (0, errno, "%s", quotef (input_filename));
              ok = false;
            }
        }
    }
  while (in_stream == nullptr);

  if (0 <= end_offset && !flag_dump_strings)
    setvbuf (in_stream, nullptr, _IONBF, 0);

  return ok;
}

/* Test whether there have been errors on in_stream, and close it if
   it is not standard input.  Return false if there has been an error
   on in_stream or stdout; return true otherwise.  This function will
   report more than one error only if both a read and a write error
   have occurred.  IN_ERRNO, if nonzero, is the error number
   corresponding to the most recent action for IN_STREAM.  */

static bool
check_and_close (int in_errno)
{
  bool ok = true;

  if (in_stream != nullptr)
    {
      if (!ferror (in_stream))
        in_errno = 0;
      if (streq (file_list[-1], "-"))
        clearerr (in_stream);
      else if (fclose (in_stream) != 0 && !in_errno)
        in_errno = errno;
      if (in_errno)
        {
          error (0, in_errno, "%s", quotef (input_filename));
          ok = false;
        }

      in_stream = nullptr;
    }

  if (ferror (stdout))
    {
      error (0, 0, _("write error"));
      ok = false;
    }

  return ok;
}

/* Decode the modern od format string S.  Append the decoded
   representation to the global array SPEC, reallocating SPEC if
   necessary.  Return true if S is valid.  */

static bool ATTRIBUTE_NONNULL ()
decode_format_string (char const *s)
{
  char const *s_orig = s;

  while (*s != '\0')
    {
      char const *next;

      if (n_specs_allocated <= n_specs)
        spec = xpalloc (spec, &n_specs_allocated, 1, -1, sizeof *spec);

      if (! decode_one_format (s_orig, s, &next, &spec[n_specs]))
        return false;

      affirm (s != next);
      s = next;
      ++n_specs;
    }

  return true;
}

/* Given a list of one or more input filenames FILE_LIST, set the global
   file pointer IN_STREAM to position N_SKIP in the concatenation of
   those files.  If any file operation fails or if there are fewer than
   N_SKIP bytes in the combined input, give an error message and return
   false.  When possible, use seek rather than read operations to
   advance IN_STREAM.  */

static bool
skip (intmax_t n_skip)
{
  bool ok = true;
  int in_errno = 0;

  if (n_skip == 0)
    return true;

  while (in_stream != nullptr)	/* EOF.  */
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
          /* The st_size field is valid for regular files.
             If the number of bytes left to skip is larger than
             the size of the current file, we can decrement n_skip
             and go on to the next file.  Skip this optimization also
             when st_size is no greater than the block size, because
             some kernels report nonsense small file sizes for
             proc-like file systems.  */
          if (S_ISREG (file_stats.st_mode)
              && STP_BLKSIZE (&file_stats) < file_stats.st_size)
            {
              if (file_stats.st_size < n_skip)
                n_skip -= file_stats.st_size;
              else
                {
                  if (fseeko (in_stream, n_skip, SEEK_CUR) != 0)
                    {
                      in_errno = errno;
                      ok = false;
                    }
                  n_skip = 0;
                }
            }

          else if (! (S_ISREG (file_stats.st_mode)
                      || S_TYPEISSHM (&file_stats)
                      || S_TYPEISTMO (&file_stats))
                   && fseeko (in_stream, n_skip, SEEK_CUR) == 0)
            n_skip = 0;

          /* If it's neither a seekable file with unusable size, nor a
             regular file so large it can't be in a proc-like file system,
             position the file pointer by reading.  */

          else
            {
              char buf[BUFSIZ];
              idx_t n_bytes_read, n_bytes_to_read = BUFSIZ;

              while (0 < n_skip)
                {
                  if (n_skip < n_bytes_to_read)
                    n_bytes_to_read = n_skip;
                  n_bytes_read = fread (buf, 1, n_bytes_to_read, in_stream);
                  n_skip -= n_bytes_read;
                  if (n_bytes_read != n_bytes_to_read)
                    {
                      if (ferror (in_stream))
                        {
                          in_errno = errno;
                          ok = false;
                          n_skip = 0;
                          break;
                        }
                      if (feof (in_stream))
                        break;
                    }
                }
            }

          if (n_skip == 0)
            break;
        }

      else   /* cannot fstat() file */
        {
          error (0, errno, "%s", quotef (input_filename));
          ok = false;
        }

      ok &= check_and_close (in_errno);

      ok &= open_next_file ();
    }

  if (n_skip != 0)
    error (EXIT_FAILURE, 0, _("cannot skip past end of combined input"));

  return ok;
}

static void
format_address_none (MAYBE_UNUSED intmax_t address,
                     MAYBE_UNUSED char c)
{
}

static void
format_address_std (intmax_t address, char c)
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
format_address_paren (intmax_t address, char c)
{
  putchar ('(');
  format_address_std (address, ')');
  if (c)
    putchar (c);
}

static void
format_address_label (intmax_t address, char c)
{
  format_address_std (address, ' ');

  intmax_t addr;
  if (ckd_add (&addr, address, pseudo_offset))
    error (EXIT_FAILURE, 0, _("pseudo address too large for input"));

  format_address_paren (addr, c);
}

/* Write N_BYTES bytes from CURR_BLOCK to standard output once for each
   of the N_SPEC format specs.  CURRENT_OFFSET is the byte address of
   CURR_BLOCK in the concatenation of input files, and it is printed
   (optionally) only before the output line associated with the first
   format spec.  When duplicate blocks are being abbreviated, the output
   for a sequence of identical input blocks is the output for the first
   block followed by an asterisk alone on a line.  It is valid to compare
   the blocks PREV_BLOCK and CURR_BLOCK only when N_BYTES == BYTES_PER_BLOCK.
   That condition may be false only for the last input block.  */

static void
write_block (intmax_t current_offset, idx_t n_bytes,
             char const *prev_block, char const *curr_block)
{
  static bool first = true;
  static bool prev_pair_equal = false;

  if (abbreviate_duplicate_blocks
      && !first && n_bytes == bytes_per_block
      && memeq (prev_block, curr_block, bytes_per_block))
    {
      if (prev_pair_equal)
        {
          /* The two preceding blocks were equal, and the current
             block is the same as the last one, so print nothing.  */
        }
      else
        {
          printf ("*\n");
          prev_pair_equal = true;
        }
    }
  else
    {
      prev_pair_equal = false;
      for (idx_t i = 0; i < n_specs; i++)
        {
          int datum_width = width_bytes[spec[i].size];
          idx_t fields_per_block = bytes_per_block / datum_width;
          idx_t blank_fields = (bytes_per_block - n_bytes) / datum_width;
          if (i == 0)
            format_address (current_offset, '\0');
          else
            print_n_spaces (address_pad_len);
          (*spec[i].print_function) (fields_per_block, blank_fields,
                                     curr_block, spec[i].fmt_string,
                                     spec[i].field_width, spec[i].pad_width);
          if (spec[i].hexl_mode_trailer)
            {
              /* Space-pad out to full line width, then dump the trailer.  */
              int field_width = spec[i].field_width;
              for (idx_t f = 0; f < blank_fields; f++)
                print_n_spaces (field_width);
              idx_t pad_width = pad_at (fields_per_block, blank_fields,
                                        spec[i].pad_width);
              print_n_spaces (pad_width);
              dump_hexl_mode_trailer (n_bytes, curr_block);
            }
          putchar ('\n');
        }
    }
  first = false;
}

/* Read a single byte into *C from the concatenation of the input files
   named in the global array FILE_LIST.  On the first call to this
   function, the global variable IN_STREAM is expected to be an open
   stream associated with the input file INPUT_FILENAME.  If IN_STREAM
   is at end-of-file, close it and update the global variables IN_STREAM
   and INPUT_FILENAME so they correspond to the next file in the list.
   Then try to read a byte from the newly opened file.  Repeat if
   necessary until EOF is reached for the last file in FILE_LIST, then
   set *C to EOF and return.  Subsequent calls do likewise.  Return
   true if successful.  */

static bool
read_char (int *c)
{
  bool ok = true;

  *c = EOF;

  while (in_stream && (*c = getc (in_stream)) < 0)
    {
      ok &= check_and_close (errno);
      ok &= open_next_file ();
    }

  return ok;
}

/* Read N bytes into BLOCK from the concatenation of the input files
   named in the global array FILE_LIST.  On the first call to this
   function, the global variable IN_STREAM is expected to be an open
   stream associated with the input file INPUT_FILENAME.  If all N
   bytes cannot be read from IN_STREAM, close IN_STREAM and update
   the global variables IN_STREAM and INPUT_FILENAME.  Then try to
   read the remaining bytes from the newly opened file.  Repeat if
   necessary until EOF is reached for the last file in FILE_LIST.
   On subsequent calls, don't modify BLOCK and return true.  Set
   *N_BYTES_IN_BUFFER to the number of bytes read.  If an error occurs,
   it will be detected through ferror when the stream is about to be
   closed.  If there is an error, give a message but continue reading
   as usual and return false.  Otherwise return true.  */

static bool
read_block (idx_t n, char *block, idx_t *n_bytes_in_buffer)
{
  bool ok = true;

  affirm (0 < n && n <= bytes_per_block);

  *n_bytes_in_buffer = 0;

  while (in_stream != nullptr)	/* EOF.  */
    {
      idx_t n_needed = n - *n_bytes_in_buffer;
      idx_t n_read = fread (block + *n_bytes_in_buffer,
                            1, n_needed, in_stream);

      *n_bytes_in_buffer += n_read;

      if (n_read == n_needed)
        break;

      ok &= check_and_close (errno);

      ok &= open_next_file ();
    }

  return ok;
}

/* Return the least common multiple of the sizes associated
   with the format specs.  */

ATTRIBUTE_PURE
static int
get_lcm (void)
{
  int l_c_m = 1;

  for (idx_t i = 0; i < n_specs; i++)
    l_c_m = lcm (l_c_m, width_bytes[spec[i].size]);
  return l_c_m;
}

/* Act like xstrtoimax (NPTR, nullptr, BASE, VAL, VALID_SUFFIXES),
   except reject negative values, and *VAL may be set if
   LONGINT_INVALID is returned.  */
static strtol_error
xstr2nonneg (char const *restrict nptr, int base, intmax_t *val,
             char const *restrict valid_suffixes)
{
  strtol_error s_err = xstrtoimax (nptr, nullptr, base, val, valid_suffixes);
  return s_err != LONGINT_INVALID && *val < 0 ? LONGINT_INVALID : s_err;
}

/* If STR is a valid traditional offset specification with an optional
   leading '+', and can be represented in intmax_t, return true and
   set *OFFSET to the offset it denotes.  If it is valid but cannot be
   represented, diagnose the problem and exit.  Otherwise return false
   and possibly set *OFFSET.  */

static bool
parse_old_offset (char *str, intmax_t *offset)
{
  /* Skip over any leading '+'.  */
  char *s = str + (str[0] == '+');

  if (! c_isdigit (s[0]))
    return false;

  /* Determine the radix for S.  If there is a '.', followed first by
     optional 'b' or (undocumented) 'B' and then by end of string,
     it's decimal, otherwise, if the string begins with '0X' or '0x',
     it's hexadecimal, else octal.  */
  char *dot = strchr (s, '.');
  if (dot && dot[(dot[1] == 'b' || dot[1] == 'B') + 1])
    dot = nullptr;
  int radix = dot ? 10 : s[0] == '0' && (s[1] == 'x' || s[1] == 'X') ? 16 : 8;

  if (dot)
    {
      /* Temporarily remove the '.' from the decimal string.  */
      dot[0] = dot[1];
      dot[1] = '\0';
    }

  enum strtol_error s_err = xstr2nonneg (s, radix, offset, "bB");

  if (dot)
    {
      /* Restore the decimal string's original value.  */
      dot[1] = dot[0];
      dot[0] = '.';
    }

  if (s_err == LONGINT_OVERFLOW)
    error (EXIT_FAILURE, ERANGE, "%s", quotef (str));
  return s_err == LONGINT_OK;
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
   check_and_close, and if any was false, return false.
   Otherwise, return true.  */

static bool
dump (void)
{
  char *block[2];
  bool idx = false;
  bool ok = true;
  idx_t n_bytes_read;

  block[0] = xinmalloc (2, bytes_per_block);
  block[1] = block[0] + bytes_per_block;

  intmax_t current_offset = n_bytes_to_skip;

  do
    {
      intmax_t needed_bound
        = end_offset < 0 ? INTMAX_MAX : end_offset - current_offset;
      if (needed_bound <= 0)
        {
          n_bytes_read = 0;
          break;
        }
      idx_t n_needed = MIN (bytes_per_block, needed_bound);
      ok &= read_block (n_needed, block[idx], &n_bytes_read);
      if (n_bytes_read < bytes_per_block)
        break;
      affirm (n_bytes_read == bytes_per_block);
      write_block (current_offset, n_bytes_read,
                   block[!idx], block[idx]);
      if (ferror (stdout))
        ok = false;
      current_offset += n_bytes_read;
      idx = !idx;
    }
  while (ok);

  if (n_bytes_read > 0)
    {
      int l_c_m = get_lcm ();

      /* Ensure zero-byte padding up to the smallest multiple of l_c_m that
         is at least as large as n_bytes_read.  */
      idx_t bytes_to_write = (n_bytes_read + l_c_m - 1
                              - (n_bytes_read + l_c_m - 1) % l_c_m);

      memset (block[idx] + n_bytes_read, 0, bytes_to_write - n_bytes_read);
      write_block (current_offset, n_bytes_read, block[!idx], block[idx]);
      current_offset += n_bytes_read;
    }

  format_address (current_offset, '\n');

  if (0 <= end_offset && end_offset <= current_offset)
    ok &= check_and_close (0);

  free (block[0]);

  return ok;
}

/* STRINGS mode.  Find each "string constant" in the input.
   A string constant is a run of at least STRING_MIN
   printable characters terminated by a NUL or END_OFFSET.
   Based on a function written by Richard Stallman for a
   traditional version of od.  Return true if successful.  */

static bool
dump_strings (void)
{
  idx_t bufsize = MAX (100, string_min + 1);
  char *buf = ximalloc (bufsize);
  intmax_t address = n_bytes_to_skip;
  bool ok = true;

  while (true)
    {
      idx_t i = 0;
      int c = 1;  /* Init to 1 so can distinguish if NUL read.  */

      if (0 <= end_offset
          && (end_offset < string_min || end_offset - string_min < address))
        break;

      /* Store consecutive printable characters to BUF.  */
      while (! (0 <= end_offset && end_offset <= address))
        {
          if (i == bufsize - 1)
            buf = xpalloc (buf, &bufsize, 1, -1, sizeof *buf);
          ok &= read_char (&c);
          if (c < 0)
            {
              free (buf);
              return ok;
            }
          address++;
          buf[i++] = c;
          if (c == '\0')
            break;		/* Print this string.  */
          if (! isprint (c))
            {
              c = -1;		/* Give up on this string.  */
              break;
            }
        }

      if (c < 0 || i - !c < string_min)
        continue;

      buf[i] = 0;

      /* If we get here, the string is all printable, so print it.  */

      format_address (address - i, ' ');

      for (i = 0; (c = buf[i]); i++)
        {
          switch (c)
            {
            case '\a':
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

  ok &= check_and_close (0);
  return ok;
}

int
main (int argc, char **argv)
{
  int n_files;
  int l_c_m;
  idx_t desired_width = 0;
  bool modern = false;
  bool ok = true;
  idx_t width_per_block = 0;
  static char const multipliers[] = "bEGKkMmPQRTYZ0";

  /* The maximum number of bytes that will be formatted.
     If negative, there is no limit.  */
  intmax_t max_bytes_to_format = -1;

  /* The old-style 'pseudo starting address' to be printed in parentheses
     after any true address.  */
  intmax_t pseudo_start IF_LINT ( = 0);

  initialize_main (&argc, &argv);
  set_program_name (argv[0]);
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  while (true)
    {
      intmax_t tmp;
      enum strtol_error s_err;
      int oi = -1;
      int c = getopt_long (argc, argv, short_options, long_options, &oi);
      if (c == -1)
        break;

      switch (c)
        {
        case 'A':
          modern = true;
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
                     _("invalid output address radix '%c';"
                       " it must be one character from [doxn]"),
                     optarg[0]);
              break;
            }
          break;

        case 'j':
          modern = true;
          s_err = xstr2nonneg (optarg, 0, &n_bytes_to_skip, multipliers);
          if (s_err != LONGINT_OK)
            xstrtol_fatal (s_err, oi, c, long_options, optarg);
          break;

        case 'N':
          modern = true;
          s_err = xstr2nonneg (optarg, 0, &max_bytes_to_format, multipliers);
          if (s_err != LONGINT_OK)
            xstrtol_fatal (s_err, oi, c, long_options, optarg);
          break;

        case 'S':
          modern = true;
          if (optarg == nullptr)
            string_min = 3;
          else
            {
              s_err = xstr2nonneg (optarg, 0, &tmp, multipliers);
              /* The minimum string length + 1 must fit in idx_t,
                 since we may allocate a buffer of this size + 1.  */
              idx_t i;
              if (s_err == LONGINT_OK && ckd_add (&i, tmp, 1))
                s_err = LONGINT_OVERFLOW;
              if (s_err != LONGINT_OK)
                xstrtol_fatal (s_err, oi, c, long_options, optarg);
              string_min = tmp;
            }
          flag_dump_strings = true;
          break;

        case 't':
          modern = true;
          ok &= decode_format_string (optarg);
          break;

        case 'v':
          modern = true;
          abbreviate_duplicate_blocks = false;
          break;

        case TRADITIONAL_OPTION:
          traditional = true;
          break;

        case ENDIAN_OPTION:
          switch (XARGMATCH ("--endian", optarg, endian_args, endian_types))
            {
            case endian_big:
              input_swap = BYTE_ORDER != BIG_ENDIAN;
              break;
            case endian_little:
              input_swap = BYTE_ORDER != LITTLE_ENDIAN;
              break;
            }
          break;

          /* The next several cases map the traditional format
             specification options to the corresponding modern format
             specs.  GNU od accepts any combination of old- and
             new-style options.  Format specification options accumulate.
             The obsolescent and undocumented formats are compatible
             with FreeBSD 4.10 od.  */

#define CASE_OLD_ARG(old_char,new_string)		\
        case old_char:					\
          ok &= decode_format_string (new_string);	\
          break

          CASE_OLD_ARG ('a', "a");
          CASE_OLD_ARG ('b', "o1");
          CASE_OLD_ARG ('c', "c");
          CASE_OLD_ARG ('D', "u4"); /* obsolescent and undocumented */
          CASE_OLD_ARG ('d', "u2");
        case 'F': /* obsolescent and undocumented alias */
          CASE_OLD_ARG ('e', "fD"); /* obsolescent and undocumented */
          CASE_OLD_ARG ('f', "fF");
        case 'X': /* obsolescent and undocumented alias */
          CASE_OLD_ARG ('H', "x4"); /* obsolescent and undocumented */
          CASE_OLD_ARG ('i', "dI");
        case 'I': case 'L': /* obsolescent and undocumented aliases */
          CASE_OLD_ARG ('l', "dL");
          CASE_OLD_ARG ('O', "o4"); /* obsolescent and undocumented */
        case 'B': /* obsolescent and undocumented alias */
          CASE_OLD_ARG ('o', "o2");
          CASE_OLD_ARG ('s', "d2");
        case 'h': /* obsolescent and undocumented alias */
          CASE_OLD_ARG ('x', "x2");

#undef CASE_OLD_ARG

        case 'w':
          modern = true;
          if (optarg == nullptr)
            {
              desired_width = 32;
            }
          else
            {
              intmax_t w_tmp;
              s_err = xstr2nonneg (optarg, 10, &w_tmp, "");
              if (s_err == LONGINT_OK)
                {
                  if (ckd_add (&desired_width, w_tmp, 0))
                    s_err = LONGINT_OVERFLOW;
                  else if (desired_width == 0)
                    s_err = LONGINT_INVALID;
                }
              if (s_err != LONGINT_OK)
                xstrtol_fatal (s_err, oi, c, long_options, optarg);
            }
          break;

        case_GETOPT_HELP_CHAR;

        case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);

        default:
          usage (EXIT_FAILURE);
          break;
        }
    }

  if (!ok)
    return EXIT_FAILURE;

  if (flag_dump_strings && n_specs > 0)
    error (EXIT_FAILURE, 0,
           _("no type may be specified when dumping strings"));

  n_files = argc - optind;

  /* If the --traditional option is used, there may be from
     0 to 3 remaining command line arguments;  handle each case
     separately.
        od [file] [[+]offset[.][b] [[+]label[.][b]]]
     The offset and label have the same syntax.

     If --traditional is not given, and if no modern options are
     given, and if the offset begins with + or (if there are two
     operands) a digit, accept only this form, as per POSIX:
        od [file] [[+]offset[.][b]]
  */

  if (!modern || traditional)
    {
      intmax_t o1;
      intmax_t o2;

      switch (n_files)
        {
        case 1:
          if ((traditional || argv[optind][0] == '+')
              && parse_old_offset (argv[optind], &o1))
            {
              n_bytes_to_skip = o1;
              --n_files;
              ++argv;
            }
          break;

        case 2:
          if ((traditional || argv[optind + 1][0] == '+'
               || c_isdigit (argv[optind + 1][0]))
              && parse_old_offset (argv[optind + 1], &o2))
            {
              if (traditional && parse_old_offset (argv[optind], &o1))
                {
                  n_bytes_to_skip = o1;
                  flag_pseudo_start = true;
                  pseudo_start = o2;
                  argv += 2;
                  n_files -= 2;
                }
              else
                {
                  n_bytes_to_skip = o2;
                  --n_files;
                  argv[optind + 1] = argv[optind];
                  ++argv;
                }
            }
          break;

        case 3:
          if (traditional
              && parse_old_offset (argv[optind + 1], &o1)
              && parse_old_offset (argv[optind + 2], &o2))
            {
              n_bytes_to_skip = o1;
              flag_pseudo_start = true;
              pseudo_start = o2;
              argv[optind + 2] = argv[optind];
              argv += 2;
              n_files -= 2;
            }
          break;
        }

      if (traditional && 1 < n_files)
        {
          error (0, 0, _("extra operand %s"), quote (argv[optind + 1]));
          error (0, 0, "%s",
                 _("compatibility mode supports at most one file"));
          usage (EXIT_FAILURE);
        }
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

  if (0 <= max_bytes_to_format
      && ckd_add (&end_offset, n_bytes_to_skip, max_bytes_to_format))
    error (EXIT_FAILURE, 0, _("skip-bytes + read-bytes is too large"));

  if (n_specs == 0)
    decode_format_string ("oS");

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
  ok = open_next_file ();
  if (in_stream == nullptr)
    goto cleanup;

  /* skip over any unwanted header bytes */
  ok &= skip (n_bytes_to_skip);
  if (in_stream == nullptr)
    goto cleanup;

  pseudo_offset = (flag_pseudo_start ? pseudo_start - n_bytes_to_skip : 0);

  /* Compute output block length.  */
  l_c_m = get_lcm ();

  if (desired_width != 0)
    {
      if (desired_width % l_c_m == 0)
        bytes_per_block = desired_width;
      else
        {
          error (0, 0, _("warning: invalid width %td; using %d instead"),
                 desired_width, l_c_m);
          bytes_per_block = l_c_m;
        }
    }
  else
    {
      if (l_c_m < DEFAULT_BYTES_PER_BLOCK)
        bytes_per_block = (DEFAULT_BYTES_PER_BLOCK
                           - DEFAULT_BYTES_PER_BLOCK % l_c_m);
      else
        bytes_per_block = l_c_m;
    }

  /* Compute padding necessary to align output block.  */
  for (idx_t i = 0; i < n_specs; i++)
    {
      idx_t fields_per_block = bytes_per_block / width_bytes[spec[i].size];
      if (pad_at_overflow (fields_per_block))
        error (EXIT_FAILURE, 0, _("%td is too large"), desired_width);
      idx_t block_width = (spec[i].field_width + 1) * fields_per_block;
      if (width_per_block < block_width)
        width_per_block = block_width;
    }
  for (idx_t i = 0; i < n_specs; i++)
    {
      idx_t fields_per_block = bytes_per_block / width_bytes[spec[i].size];
      idx_t block_width = spec[i].field_width * fields_per_block;
      spec[i].pad_width = width_per_block - block_width;
    }

#ifdef DEBUG
  printf ("lcm=%d, width_per_block=%td\n", l_c_m, width_per_block);
  for (idx_t i = 0; i < n_specs; i++)
    {
      idx_t fields_per_block = bytes_per_block / width_bytes[spec[i].size];
      affirm (bytes_per_block % width_bytes[spec[i].size] == 0);
      affirm (1 <= spec[i].pad_width / fields_per_block);
      printf ("%d: fmt=\"%s\" in_width=%d out_width=%d pad=%d\n",
              i, spec[i].fmt_string, width_bytes[spec[i].size],
              spec[i].field_width, spec[i].pad_width);
    }
#endif

  ok &= (flag_dump_strings ? dump_strings () : dump ());

cleanup:

  if (have_read_stdin && fclose (stdin) < 0)
    error (EXIT_FAILURE, errno, _("standard input"));

  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
