/* Reformat numbers like 11505426432 to the more human-readable 11G
   Copyright (C) 2012-2025 Free Software Foundation, Inc.

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

#include <config.h>
#include <ctype.h>
#include <float.h>
#include <getopt.h>
#include <stdio.h>
#include <sys/types.h>
#include <langinfo.h>

#include "argmatch.h"
#include "c-ctype.h"
#include "mbswidth.h"
#include "quote.h"
#include "skipchars.h"
#include "system.h"
#include "xstrtol.h"

#include "set-fields.h"

#if HAVE_FPSETPREC
# include <ieeefp.h>
#endif

/* The official name of this program (e.g., no 'g' prefix).  */
#define PROGRAM_NAME "numfmt"

#define AUTHORS proper_name ("Assaf Gordon")

/* Exit code when some numbers fail to convert.  */
enum { EXIT_CONVERSION_WARNINGS = 2 };

enum
{
  FROM_OPTION = CHAR_MAX + 1,
  FROM_UNIT_OPTION,
  TO_OPTION,
  TO_UNIT_OPTION,
  ROUND_OPTION,
  SUFFIX_OPTION,
  GROUPING_OPTION,
  PADDING_OPTION,
  FIELD_OPTION,
  DEBUG_OPTION,
  DEV_DEBUG_OPTION,
  HEADER_OPTION,
  FORMAT_OPTION,
  INVALID_OPTION
};

enum scale_type
{
  scale_none,                   /* the default: no scaling.  */
  scale_auto,                   /* --from only.  */
  scale_SI,
  scale_IEC,
  scale_IEC_I                   /* 'i' suffix is required.  */
};

static char const *const scale_from_args[] =
{
  "none", "auto", "si", "iec", "iec-i", nullptr
};

static enum scale_type const scale_from_types[] =
{
  scale_none, scale_auto, scale_SI, scale_IEC, scale_IEC_I
};

static char const *const scale_to_args[] =
{
  "none", "si", "iec", "iec-i", nullptr
};

static enum scale_type const scale_to_types[] =
{
  scale_none, scale_SI, scale_IEC, scale_IEC_I
};


enum round_type
{
  round_ceiling,
  round_floor,
  round_from_zero,
  round_to_zero,
  round_nearest,
};

static char const *const round_args[] =
{
  "up", "down", "from-zero", "towards-zero", "nearest", nullptr
};

static enum round_type const round_types[] =
{
  round_ceiling, round_floor, round_from_zero, round_to_zero, round_nearest
};


enum inval_type
{
  inval_abort,
  inval_fail,
  inval_warn,
  inval_ignore
};

static char const *const inval_args[] =
{
  "abort", "fail", "warn", "ignore", nullptr
};

static enum inval_type const inval_types[] =
{
  inval_abort, inval_fail, inval_warn, inval_ignore
};

static struct option const longopts[] =
{
  {"from", required_argument, nullptr, FROM_OPTION},
  {"from-unit", required_argument, nullptr, FROM_UNIT_OPTION},
  {"to", required_argument, nullptr, TO_OPTION},
  {"to-unit", required_argument, nullptr, TO_UNIT_OPTION},
  {"round", required_argument, nullptr, ROUND_OPTION},
  {"padding", required_argument, nullptr, PADDING_OPTION},
  {"suffix", required_argument, nullptr, SUFFIX_OPTION},
  {"grouping", no_argument, nullptr, GROUPING_OPTION},
  {"delimiter", required_argument, nullptr, 'd'},
  {"field", required_argument, nullptr, FIELD_OPTION},
  {"debug", no_argument, nullptr, DEBUG_OPTION},
  {"-debug", no_argument, nullptr, DEV_DEBUG_OPTION},
  {"header", optional_argument, nullptr, HEADER_OPTION},
  {"format", required_argument, nullptr, FORMAT_OPTION},
  {"invalid", required_argument, nullptr, INVALID_OPTION},
  {"zero-terminated", no_argument, nullptr, 'z'},
  {GETOPT_HELP_OPTION_DECL},
  {GETOPT_VERSION_OPTION_DECL},
  {nullptr, 0, nullptr, 0}
};

/* If delimiter has this value, blanks separate fields.  */
enum { DELIMITER_DEFAULT = CHAR_MAX + 1 };

/* Maximum number of digits we can safely handle
   without precision loss, if scaling is 'none'.  */
enum { MAX_UNSCALED_DIGITS = LDBL_DIG };

/* Maximum number of digits we can work with.
   This is equivalent to 999Q.
   NOTE: 'long double' can handle more than that, but there's
         no official suffix assigned beyond Quetta (1000^10).  */
enum { MAX_ACCEPTABLE_DIGITS = 33 };

static enum scale_type scale_from = scale_none;
static enum scale_type scale_to = scale_none;
static enum round_type round_style = round_from_zero;
static enum inval_type inval_style = inval_abort;
static char const *suffix = nullptr;
static uintmax_t from_unit_size = 1;
static uintmax_t to_unit_size = 1;
static int grouping = 0;
static char *padding_buffer = nullptr;
static idx_t padding_buffer_size = 0;
static intmax_t padding_width = 0;
static int zero_padding_width = 0;
static long int user_precision = -1;
static char const *format_str = nullptr;
static char *format_str_prefix = nullptr;
static char *format_str_suffix = nullptr;

/* By default, any conversion error will terminate the program.  */
static int conv_exit_code = EXIT_CONVERSION_WARNINGS;


/* auto-pad each line based on skipped whitespace.  */
static int auto_padding = 0;

/* field delimiter */
static int delimiter = DELIMITER_DEFAULT;

/* line delimiter.  */
static unsigned char line_delim = '\n';

/* if non-zero, the first 'header' lines from STDIN are skipped.  */
static uintmax_t header = 0;

/* Debug for users: print warnings to STDERR about possible
   error (similar to sort's debug).  */
static bool debug;

/* will be set according to the current locale.  */
static char const *decimal_point;
static int decimal_point_length;

/* debugging for developers.  Enables devmsg().  */
static bool dev_debug = false;


static inline int
default_scale_base (enum scale_type scale)
{
  switch (scale)
    {
    case scale_IEC:
    case scale_IEC_I:
      return 1024;

    case scale_none:
    case scale_auto:
    case scale_SI:
    default:
      return 1000;
    }
}

static char const zero_and_valid_suffixes[] = "0KkMGTPEZYRQ";
static char const *valid_suffixes = 1 + zero_and_valid_suffixes;

static inline bool
valid_suffix (const char suf)
{
  return strchr (valid_suffixes, suf) != nullptr;
}

static inline int
suffix_power (const char suf)
{
  switch (suf)
    {
    case 'k':                  /* kilo.  */
    case 'K':                  /* kilo or kibi.  */
      return 1;

    case 'M':                  /* mega or mebi.  */
      return 2;

    case 'G':                  /* giga or gibi.  */
      return 3;

    case 'T':                  /* tera or tebi.  */
      return 4;

    case 'P':                  /* peta or pebi.  */
      return 5;

    case 'E':                  /* exa or exbi.  */
      return 6;

    case 'Z':                  /* zetta or 2**70.  */
      return 7;

    case 'Y':                  /* yotta or 2**80.  */
      return 8;

    case 'R':                  /* ronna or 2**90.  */
      return 9;

    case 'Q':                  /* quetta or 2**100.  */
      return 10;

    default:                   /* should never happen. assert?  */
      return 0;
    }
}

static inline char const *
suffix_power_char (int power)
{
  switch (power)
    {
    case 0:
      return "";

    case 1:
      return "K";

    case 2:
      return "M";

    case 3:
      return "G";

    case 4:
      return "T";

    case 5:
      return "P";

    case 6:
      return "E";

    case 7:
      return "Z";

    case 8:
      return "Y";

    case 9:
      return "R";

    case 10:
      return "Q";

    default:
      return "(error)";
    }
}

/* Similar to 'powl(3)' but without requiring 'libm'.  */
static long double
powerld (long double base, int x)
{
  long double result = base;
  if (x == 0)
    return 1;                   /* note for test coverage: this is never
                                   reached, as 'powerld' won't be called if
                                   there's no suffix, hence, no "power".  */

  /* TODO: check for overflow, inf?  */
  while (--x)
    result *= base;
  return result;
}

/* Similar to 'fabs(3)' but without requiring 'libm'.  */
static inline long double
absld (long double val)
{
  return val < 0 ? -val : val;
}

/* Scale down 'val', returns 'updated val' and 'x', such that
     val*base^X = original val
     Similar to "frexpl(3)" but without requiring 'libm',
     allowing only integer scale, limited functionality and error checking.  */
static long double
expld (long double val, int base, int /*output */ *x)
{
  int power = 0;

  if (val >= -LDBL_MAX && val <= LDBL_MAX)
    {
      while (absld (val) >= base)
        {
          ++power;
          val /= base;
        }
    }
  if (x)
    *x = power;
  return val;
}

/* EXTREMELY limited 'ceil' - without 'libm'.
   Assumes values that fit in intmax_t.  */
static inline intmax_t
simple_round_ceiling (long double val)
{
  intmax_t intval = val;
  if (intval < val)
    intval++;
  return intval;
}

/* EXTREMELY limited 'floor' - without 'libm'.
   Assumes values that fit in intmax_t.  */
static inline intmax_t
simple_round_floor (long double val)
{
  return -simple_round_ceiling (-val);
}

/* EXTREMELY limited 'round away from zero'.
   Assumes values that fit in intmax_t.  */
static inline intmax_t
simple_round_from_zero (long double val)
{
  return val < 0 ? simple_round_floor (val) : simple_round_ceiling (val);
}

/* EXTREMELY limited 'round away to zero'.
   Assumes values that fit in intmax_t.  */
static inline intmax_t
simple_round_to_zero (long double val)
{
  return val;
}

/* EXTREMELY limited 'round' - without 'libm'.
   Assumes values that fit in intmax_t.  */
static inline intmax_t
simple_round_nearest (long double val)
{
  return val < 0 ? val - 0.5 : val + 0.5;
}

ATTRIBUTE_CONST
static inline long double
simple_round (long double val, enum round_type t)
{
  intmax_t rval;
  intmax_t intmax_mul = val / INTMAX_MAX;
  val -= (long double) INTMAX_MAX * intmax_mul;

  switch (t)
    {
    case round_ceiling:
      rval = simple_round_ceiling (val);
      break;

    case round_floor:
      rval = simple_round_floor (val);
      break;

    case round_from_zero:
      rval = simple_round_from_zero (val);
      break;

    case round_to_zero:
      rval = simple_round_to_zero (val);
      break;

    case round_nearest:
      rval = simple_round_nearest (val);
      break;

    default:
      /* to silence the compiler - this should never happen.  */
      return 0;
    }

  return (long double) INTMAX_MAX * intmax_mul + rval;
}

enum simple_strtod_error
{
  SSE_OK = 0,
  SSE_OK_PRECISION_LOSS,
  SSE_OVERFLOW,
  SSE_INVALID_NUMBER,

  /* the following are returned by 'simple_strtod_human'.  */
  SSE_VALID_BUT_FORBIDDEN_SUFFIX,
  SSE_INVALID_SUFFIX,
  SSE_MISSING_I_SUFFIX
};

/* Read an *integer* INPUT_STR,
   but return the integer value in a 'long double' VALUE
   hence, no UINTMAX_MAX limitation.
   NEGATIVE is updated, and is stored separately from the VALUE
   so that signbit() isn't required to determine the sign of -0..
   ENDPTR is required (unlike strtod) and is used to store a pointer
   to the character after the last character used in the conversion.

   Note locale'd grouping is not supported,
   nor is skipping of white-space supported.

   Returns:
      SSE_OK - valid number.
      SSE_OK_PRECISION_LOSS - if more than 18 digits were used.
      SSE_OVERFLOW          - if more than 33 digits (999Q) were used.
      SSE_INVALID_NUMBER    - if no digits were found.  */
static enum simple_strtod_error
simple_strtod_int (char const *input_str,
                   char **endptr, long double *value, bool *negative)
{
  enum simple_strtod_error e = SSE_OK;

  long double val = 0;
  int digits = 0;
  bool found_digit = false;

  if (*input_str == '-')
    {
      input_str++;
      *negative = true;
    }
  else
    *negative = false;

  *endptr = (char *) input_str;
  while (c_isdigit (**endptr))
    {
      int digit = (**endptr) - '0';

      found_digit = true;

      if (val || digit)
        digits++;

      if (digits > MAX_UNSCALED_DIGITS)
        e = SSE_OK_PRECISION_LOSS;

      if (digits > MAX_ACCEPTABLE_DIGITS)
        return SSE_OVERFLOW;

      val *= 10;
      val += digit;

      ++(*endptr);
    }
  if (! found_digit
      && ! STREQ_LEN (*endptr, decimal_point, decimal_point_length))
    return SSE_INVALID_NUMBER;
  if (*negative)
    val = -val;

  if (value)
    *value = val;

  return e;
}

/* Read a floating-point INPUT_STR represented as "NNNN[.NNNNN]",
   and return the value in a 'long double' VALUE.
   ENDPTR is required (unlike strtod) and is used to store a pointer
   to the character after the last character used in the conversion.
   PRECISION is optional and used to indicate fractions are present.

   Note locale'd grouping is not supported,
   nor is skipping of white-space supported.

   Returns:
      SSE_OK - valid number.
      SSE_OK_PRECISION_LOSS - if more than 18 digits were used.
      SSE_OVERFLOW          - if more than 33 digits (999Q) were used.
      SSE_INVALID_NUMBER    - if no digits were found.  */
static enum simple_strtod_error
simple_strtod_float (char const *input_str,
                     char **endptr,
                     long double *value,
                     size_t *precision)
{
  bool negative;
  enum simple_strtod_error e = SSE_OK;

  if (precision)
    *precision = 0;

  /* TODO: accept locale'd grouped values for the integral part.  */
  e = simple_strtod_int (input_str, endptr, value, &negative);
  if (e != SSE_OK && e != SSE_OK_PRECISION_LOSS)
    return e;

  /* optional decimal point + fraction.  */
  if (STREQ_LEN (*endptr, decimal_point, decimal_point_length))
    {
      char *ptr2;
      long double val_frac = 0;
      bool neg_frac;

      (*endptr) += decimal_point_length;
      enum simple_strtod_error e2 =
        simple_strtod_int (*endptr, &ptr2, &val_frac, &neg_frac);
      if (e2 != SSE_OK && e2 != SSE_OK_PRECISION_LOSS)
        return e2;
      if (e2 == SSE_OK_PRECISION_LOSS)
        e = e2;                       /* propagate warning.  */
      if (neg_frac)
        return SSE_INVALID_NUMBER;

      /* number of digits in the fractions.  */
      size_t exponent = ptr2 - *endptr;

      val_frac = ((long double) val_frac) / powerld (10, exponent);

      /* TODO: detect loss of precision (only really 18 digits
         of precision across all digits (before and after '.')).  */
      if (value)
        {
          if (negative)
            *value -= val_frac;
          else
            *value += val_frac;
        }

      if (precision)
        *precision = exponent;

      *endptr = ptr2;
    }
  return e;
}

/* Read a 'human' INPUT_STR represented as "NNNN[.NNNNN] + suffix",
   and return the value in a 'long double' VALUE,
   with the precision of the input returned in PRECISION.
   ENDPTR is required (unlike strtod) and is used to store a pointer
   to the character after the last character used in the conversion.
   ALLOWED_SCALING determines the scaling supported.

   TODO:
     support locale'd grouping
     accept scientific and hex floats (probably use strtold directly)

   Returns:
      SSE_OK - valid number.
      SSE_OK_PRECISION_LOSS - if more than LDBL_DIG digits were used.
      SSE_OVERFLOW          - if more than 33 digits (999Q) were used.
      SSE_INVALID_NUMBER    - if no digits were found.
      SSE_VALID_BUT_FORBIDDEN_SUFFIX
      SSE_INVALID_SUFFIX
      SSE_MISSING_I_SUFFIX  */
static enum simple_strtod_error
simple_strtod_human (char const *input_str,
                     char **endptr, long double *value, size_t *precision,
                     enum scale_type allowed_scaling)
{
  int power = 0;
  /* 'scale_auto' is checked below.  */
  int scale_base = default_scale_base (allowed_scaling);

  devmsg ("simple_strtod_human:\n  input string: %s\n"
          "  locale decimal-point: %s\n"
          "  MAX_UNSCALED_DIGITS: %d\n",
          quote_n (0, input_str),
          quote_n (1, decimal_point),
          MAX_UNSCALED_DIGITS);

  enum simple_strtod_error e =
    simple_strtod_float (input_str, endptr, value, precision);
  if (e != SSE_OK && e != SSE_OK_PRECISION_LOSS)
    return e;

  devmsg ("  parsed numeric value: %Lf\n"
          "  input precision = %d\n", *value, (int)*precision);

  if (**endptr != '\0')
    {
      /* process suffix.  */

      /* Skip any blanks between the number and suffix.  */
      while (isblank (to_uchar (**endptr)))
        (*endptr)++;

      if (!valid_suffix (**endptr))
        return SSE_INVALID_SUFFIX;

      if (allowed_scaling == scale_none)
        return SSE_VALID_BUT_FORBIDDEN_SUFFIX;

      power = suffix_power (**endptr);
      (*endptr)++;                     /* skip first suffix character.  */

      if (allowed_scaling == scale_auto && **endptr == 'i')
        {
          /* auto-scaling enabled, and the first suffix character
              is followed by an 'i' (e.g. Ki, Mi, Gi).  */
          scale_base = 1024;
          (*endptr)++;              /* skip second  ('i') suffix character.  */
          devmsg ("  Auto-scaling, found 'i', switching to base %d\n",
                  scale_base);
        }
      else if (allowed_scaling == scale_IEC_I)
        {
          if (**endptr == 'i')
            (*endptr)++;
          else
            return SSE_MISSING_I_SUFFIX;
        }

      *precision = 0;  /* Reset, to select precision based on scale.  */
    }

  long double multiplier = powerld (scale_base, power);

  devmsg ("  suffix power=%d^%d = %Lf\n", scale_base, power, multiplier);

  /* TODO: detect loss of precision and overflows.  */
  (*value) = (*value) * multiplier;

  devmsg ("  returning value: %Lf (%LG)\n", *value, *value);

  return e;
}


static void
simple_strtod_fatal (enum simple_strtod_error err, char const *input_str)
{
  char const *msgid = nullptr;

  switch (err)
    {
    case SSE_OK_PRECISION_LOSS:
    case SSE_OK:
      /* should never happen - this function isn't called when OK.  */
      unreachable ();

    case SSE_OVERFLOW:
      msgid = N_("value too large to be converted: %s");
      break;

    case SSE_INVALID_NUMBER:
      msgid = N_("invalid number: %s");
      break;

    case SSE_VALID_BUT_FORBIDDEN_SUFFIX:
      msgid = N_("rejecting suffix in input: %s (consider using --from)");
      break;

    case SSE_INVALID_SUFFIX:
      msgid = N_("invalid suffix in input: %s");
      break;

    case SSE_MISSING_I_SUFFIX:
      msgid = N_("missing 'i' suffix in input: %s (e.g Ki/Mi/Gi)");
      break;

    }

  if (inval_style != inval_ignore)
    error (conv_exit_code, 0, gettext (msgid), quote (input_str));
}

/* Convert VAL to a human format string using PRECISION in BUF of size
   BUF_SIZE.  Use SCALE, GROUP, and ROUND to format.  Return
   the number of bytes needed to represent VAL.  If this number is not
   less than BUF_SIZE, the buffer is too small; if it is negative, the
   formatting failed for some reason.  */
static int
double_to_human (long double val, int precision,
                 char *buf, idx_t buf_size,
                 enum scale_type scale, int group, enum round_type round)
{
  char fmt[sizeof "%'0.*Lfi%s%s%s" + INT_STRLEN_BOUND (zero_padding_width)];
  char *pfmt = fmt;
  *pfmt++ = '%';

  if (group)
    *pfmt++ = '\'';

  if (zero_padding_width)
    pfmt += sprintf (pfmt, "0%d", zero_padding_width);

  devmsg ("double_to_human:\n");

  if (scale == scale_none)
    {
      val *= powerld (10, precision);
      val = simple_round (val, round);
      val /= powerld (10, precision);

      devmsg ((group) ?
              "  no scaling, returning (grouped) value: %'.*Lf\n" :
              "  no scaling, returning value: %.*Lf\n", precision, val);

      strcpy (pfmt, ".*Lf%s");

      return snprintf (buf, buf_size, fmt, precision, val,
                       suffix ? suffix : "");
    }

  /* Scaling requested by user. */
  double scale_base = default_scale_base (scale);

  /* Normalize val to scale. */
  int power = 0;
  val = expld (val, scale_base, &power);
  devmsg ("  scaled value to %Lf * %0.f ^ %d\n", val, scale_base, power);

  /* Perform rounding. */
  int power_adjust = 0;
  if (user_precision != -1)
    power_adjust = MIN (power * 3, user_precision);
  else if (absld (val) < 10)
    {
      /* for values less than 10, we allow one decimal-point digit,
         so adjust before rounding. */
      power_adjust = 1;
    }

  val *= powerld (10, power_adjust);
  val = simple_round (val, round);
  val /= powerld (10, power_adjust);

  /* two special cases after rounding:
     1. a "999.99" can turn into 1000 - so scale down
     2. a "9.99" can turn into 10 - so don't display decimal-point.  */
  if (absld (val) >= scale_base)
    {
      val /= scale_base;
      power++;
    }

  /* should "7.0" be printed as "7" ?
     if removing the ".0" is preferred, enable the fourth condition.  */
  int show_decimal_point = (val != 0) && (absld (val) < 10) && (power > 0);
  /* && (absld (val) > simple_round_floor (val))) */

  devmsg ("  after rounding, value=%Lf * %0.f ^ %d\n", val, scale_base, power);

  strcpy (pfmt, ".*Lf%s%s%s");

  int prec = user_precision == -1 ? show_decimal_point : user_precision;

  return snprintf (buf, buf_size, fmt, prec, val,
                   power == 1 && scale == scale_SI
                   ? "k" : suffix_power_char (power),
                   &"i"[! (scale == scale_IEC_I && 0 < power)],
                   suffix ? suffix : "");
}

/* Convert a string of decimal digits, N_STRING, with an optional suffix
   to an integral value.  Suffixes are handled as with --from=auto.
   Upon successful conversion, return that value.
   If it cannot be converted, give a diagnostic and exit.  */
static uintmax_t
unit_to_umax (char const *n_string)
{
  strtol_error s_err;
  char const *c_string = n_string;
  char *t_string = nullptr;
  size_t n_len = strlen (n_string);
  char *end = nullptr;
  uintmax_t n;
  char const *suffixes = valid_suffixes;

  /* Adjust suffixes so K=1000, Ki=1024, KiB=invalid.  */
  if (n_len && ! c_isdigit (n_string[n_len - 1]))
    {
      t_string = xmalloc (n_len + 2);
      end = t_string + n_len - 1;
      memcpy (t_string, n_string, n_len);

      if (*end == 'i' && 2 <= n_len && ! c_isdigit (*(end - 1)))
        *end = '\0';
      else
        {
          *++end = 'B';
          *++end = '\0';
          suffixes = zero_and_valid_suffixes;
        }

      c_string = t_string;
    }

  s_err = xstrtoumax (c_string, &end, 10, &n, suffixes);

  if (s_err != LONGINT_OK || *end || n == 0)
    {
      free (t_string);
      error (EXIT_FAILURE, 0, _("invalid unit size: %s"), quote (n_string));
    }

  free (t_string);

  return n;
}

void
usage (int status)
{
  if (status != EXIT_SUCCESS)
    emit_try_help ();
  else
    {
      printf (_("\
Usage: %s [OPTION]... [NUMBER]...\n\
"), program_name);
      fputs (_("\
Reformat NUMBER(s), or the numbers from standard input if none are specified.\n\
"), stdout);
      emit_mandatory_arg_note ();
      fputs (_("\
      --debug          print warnings about invalid input\n\
"), stdout);
      fputs (_("\
  -d, --delimiter=X    use X instead of whitespace for field delimiter\n\
"), stdout);
      fputs (_("\
      --field=FIELDS   replace the numbers in these input fields (default=1);\n\
                         see FIELDS below\n\
"), stdout);
      fputs (_("\
      --format=FORMAT  use printf style floating-point FORMAT;\n\
                         see FORMAT below for details\n\
"), stdout);
      fputs (_("\
      --from=UNIT      auto-scale input numbers to UNITs; default is 'none';\n\
                         see UNIT below\n\
"), stdout);
      fputs (_("\
      --from-unit=N    specify the input unit size (instead of the default 1)\n\
"), stdout);
      fputs (_("\
      --grouping       use locale-defined grouping of digits, e.g. 1,000,000\n\
                         (which means it has no effect in the C/POSIX locale)\n\
"), stdout);
      fputs (_("\
      --header[=N]     print (without converting) the first N header lines;\n\
                         N defaults to 1 if not specified\n\
"), stdout);
      fputs (_("\
      --invalid=MODE   failure mode for invalid numbers: MODE can be:\n\
                         abort (default), fail, warn, ignore\n\
"), stdout);
      fputs (_("\
      --padding=N      pad the output to N characters; positive N will\n\
                         right-align; negative N will left-align;\n\
                         padding is ignored if the output is wider than N;\n\
                         the default is to automatically pad if a whitespace\n\
                         is found\n\
"), stdout);
      fputs (_("\
      --round=METHOD   use METHOD for rounding when scaling; METHOD can be:\n\
                         up, down, from-zero (default), towards-zero, nearest\n\
"), stdout);
      fputs (_("\
      --suffix=SUFFIX  add SUFFIX to output numbers, and accept optional\n\
                         SUFFIX in input numbers\n\
"), stdout);
      fputs (_("\
      --to=UNIT        auto-scale output numbers to UNITs; see UNIT below\n\
"), stdout);
      fputs (_("\
      --to-unit=N      the output unit size (instead of the default 1)\n\
"), stdout);
      fputs (_("\
  -z, --zero-terminated    line delimiter is NUL, not newline\n\
"), stdout);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);

      fputs (_("\
\n\
UNIT options:\n"), stdout);
      fputs (_("\
  none       no auto-scaling is done; suffixes will trigger an error\n\
"), stdout);
      fputs (_("\
  auto       accept optional single/two letter suffix:\n\
               1K = 1000, 1k = 1000,\n\
               1Ki = 1024,\n\
               1M = 1000000,\n\
               1Mi = 1048576,\n"), stdout);
      fputs (_("\
  si         accept optional single letter suffix:\n\
               1k = 1000, 1K = 1000,\n\
               1M = 1000000,\n\
               ...\n"), stdout);
      fputs (_("\
  iec        accept optional single letter suffix:\n\
               1K = 1024, 1k = 1024,\n\
               1M = 1048576,\n\
               ...\n"), stdout);
      fputs (_("\
  iec-i      accept optional two-letter suffix:\n\
               1Ki = 1024, 1ki = 1024,\n\
               1Mi = 1048576,\n\
               ...\n"), stdout);

      fputs (_("\n\
FIELDS supports cut(1) style field ranges:\n\
  N    N'th field, counted from 1\n\
  N-   from N'th field, to end of line\n\
  N-M  from N'th to M'th field (inclusive)\n\
  -M   from first to M'th field (inclusive)\n\
  -    all fields\n\
Multiple fields/ranges can be separated with commas\n\
"), stdout);

      fputs (_("\n\
FORMAT must be suitable for printing one floating-point argument '%f'.\n\
Optional quote (%'f) will enable --grouping (if supported by current locale).\n\
Optional width value (%10f) will pad output. Optional zero (%010f) width\n\
will zero pad the number. Optional negative values (%-10f) will left align.\n\
Optional precision (%.1f) will override the input determined precision.\n\
"), stdout);

      printf (_("\n\
Exit status is 0 if all input numbers were successfully converted.\n\
By default, %s will stop at the first conversion error with exit status 2.\n\
With --invalid='fail' a warning is printed for each conversion error\n\
and the exit status is 2.  With --invalid='warn' each conversion error is\n\
diagnosed, but the exit status is 0.  With --invalid='ignore' conversion\n\
errors are not diagnosed and the exit status is 0.\n\
"), program_name);

      printf (_("\n\
Examples:\n\
  $ %s --to=si 1000\n\
            -> \"1.0k\"\n\
  $ %s --to=iec 2048\n\
           -> \"2.0K\"\n\
  $ %s --to=iec-i 4096\n\
           -> \"4.0Ki\"\n\
  $ echo 1K | %s --from=si\n\
           -> \"1000\"\n\
  $ echo 1K | %s --from=iec\n\
           -> \"1024\"\n\
  $ df -B1 | %s --header --field 2-4 --to=si\n\
  $ ls -l  | %s --header --field 5 --to=iec\n\
  $ ls -lh | %s --header --field 5 --from=iec --padding=10\n\
  $ ls -lh | %s --header --field 5 --from=iec --format %%10f\n"),
              program_name, program_name, program_name,
              program_name, program_name, program_name,
              program_name, program_name, program_name);
      emit_ancillary_info (PROGRAM_NAME);
    }
  exit (status);
}

/* Given 'fmt' (a printf(3) compatible format string), extracts the following:
    1. padding (e.g. %20f)
    2. alignment (e.g. %-20f)
    3. grouping (e.g. %'f)

   Only a limited subset of printf(3) syntax is supported.

   TODO:
     support %e %g etc. rather than just %f

   NOTES:
   1. This function sets the global variables:
       padding_width, grouping,
       format_str_prefix, format_str_suffix
   2. The function aborts on any errors.  */
static void
parse_format_string (char const *fmt)
{
  size_t i;
  size_t prefix_len = 0;
  size_t suffix_pos;
  char *endptr = nullptr;
  bool zero_padding = false;

  for (i = 0; !(fmt[i] == '%' && fmt[i + 1] != '%'); i += (fmt[i] == '%') + 1)
    {
      if (!fmt[i])
        error (EXIT_FAILURE, 0,
               _("format %s has no %% directive"), quote (fmt));
      prefix_len++;
    }

  i++;
  while (true)
    {
      size_t skip = strspn (fmt + i, " ");
      i += skip;
      if (fmt[i] == '\'')
        {
          grouping = 1;
          i++;
        }
      else if (fmt[i] == '0')
        {
          zero_padding = true;
          i++;
        }
      else if (! skip)
        break;
    }

  intmax_t pad = strtoimax (fmt + i, &endptr, 10);

  if (pad != 0)
    {
      if (debug && padding_width && !(zero_padding && pad > 0))
        error (0, 0, _("--format padding overriding --padding"));

      /* Set padding width and alignment.  On overflow, set widths to
         large values that cause later code to avoid undefined behavior
         and fail at a reasonable point.  */
      if (pad < 0)
        padding_width = pad;
      else
        {
          if (zero_padding)
            zero_padding_width = MIN (pad, INT_MAX);
          else
            padding_width = pad;
        }
    }
  i = endptr - fmt;

  if (fmt[i] == '\0')
    error (EXIT_FAILURE, 0, _("format %s ends in %%"), quote (fmt));

  if (fmt[i] == '.')
    {
      i++;
      errno = 0;
      user_precision = strtol (fmt + i, &endptr, 10);
      if (errno == ERANGE || user_precision < 0 || SIZE_MAX < user_precision
          || isblank (fmt[i]) || fmt[i] == '+')
        {
          /* Note we disallow negative user_precision to be
             consistent with printf(1).  POSIX states that
             negative precision is only supported (and ignored)
             when used with '.*f'.  glibc at least will malform
             output when passed a direct negative precision.  */
          error (EXIT_FAILURE, 0,
                 _("invalid precision in format %s"), quote (fmt));
        }
      i = endptr - fmt;
    }

  if (fmt[i] != 'f')
    error (EXIT_FAILURE, 0, _("invalid format %s,"
                              " directive must be %%[0]['][-][N][.][N]f"),
         quote (fmt));
  i++;
  suffix_pos = i;

  for (; fmt[i] != '\0'; i += (fmt[i] == '%') + 1)
    if (fmt[i] == '%' && fmt[i + 1] != '%')
      error (EXIT_FAILURE, 0, _("format %s has too many %% directives"),
             quote (fmt));

  if (prefix_len)
    format_str_prefix = ximemdup0 (fmt, prefix_len);
  if (fmt[suffix_pos] != '\0')
    format_str_suffix = xstrdup (fmt + suffix_pos);

  devmsg ("format String:\n  input: %s\n  grouping: %s\n"
                   "  padding width: %jd\n"
                   "  prefix: %s\n  suffix: %s\n",
          quote_n (0, fmt), (grouping) ? "yes" : "no",
          padding_width,
          quote_n (1, format_str_prefix ? format_str_prefix : ""),
          quote_n (2, format_str_suffix ? format_str_suffix : ""));
}

/* Parse a numeric value (with optional suffix) from a string.
   Returns a long double value, with input precision.

   If there's an error converting the string to value - exits with
   an error.

   If there are any trailing characters after the number
   (besides a valid suffix) - exits with an error.  */
static enum simple_strtod_error
parse_human_number (char const *str, long double /*output */ *value,
                    size_t *precision)
{
  char *ptr = nullptr;

  enum simple_strtod_error e =
    simple_strtod_human (str, &ptr, value, precision, scale_from);
  if (e != SSE_OK && e != SSE_OK_PRECISION_LOSS)
    {
      simple_strtod_fatal (e, str);
      return e;
    }

  if (ptr && *ptr != '\0')
    {
      if (inval_style != inval_ignore)
        error (conv_exit_code, 0, _("invalid suffix in input %s: %s"),
               quote_n (0, str), quote_n (1, ptr));
      e = SSE_INVALID_SUFFIX;
    }
  return e;
}


/* Print the given VAL, using the requested representation.
   The number is printed to STDOUT, with padding and alignment.  */
static bool
prepare_padded_number (const long double val, size_t precision,
                       intmax_t *padding)
{
  /* Generate Output. */
  size_t precision_used = user_precision == -1 ? precision : user_precision;

  /* Can't reliably print too-large values without auto-scaling. */
  int x;
  expld (val, 10, &x);

  if (scale_to == scale_none
      && x + precision_used > MAX_UNSCALED_DIGITS)
    {
      if (inval_style != inval_ignore)
        {
          if (precision_used)
            error (conv_exit_code, 0,
                   _("value/precision too large to be printed: '%Lg/%zu'"
                     " (consider using --to)"), val, precision_used);
          else
            error (conv_exit_code, 0,
                   _("value too large to be printed: '%Lg'"
                     " (consider using --to)"), val);
        }
      return false;
    }

  if (x > MAX_ACCEPTABLE_DIGITS - 1)
    {
      if (inval_style != inval_ignore)
        error (conv_exit_code, 0, _("value too large to be printed: '%Lg'"
                                    " (cannot handle values > 999Q)"), val);
      return false;
    }

  while (true)
    {
      int numlen = double_to_human (val, precision_used,
                                    padding_buffer, padding_buffer_size,
                                    scale_to, grouping, round_style);
      ptrdiff_t growth;
      if (numlen < 0 || ckd_sub (&growth, numlen, padding_buffer_size - 1))
        error (EXIT_FAILURE, 0,
               _("failed to prepare value '%Lf' for printing"), val);
      if (growth <= 0)
        break;
      padding_buffer = xpalloc (padding_buffer, &padding_buffer_size,
                                growth, -1, 1);
    }

  devmsg ("formatting output:\n  value: %Lf\n  humanized: %s\n",
          val, quote (padding_buffer));

  intmax_t pad = 0;
  if (padding_width)
    {
      int buf_width = mbswidth (padding_buffer,
                                MBSW_REJECT_INVALID | MBSW_REJECT_UNPRINTABLE);
      if (0 <= buf_width)
        {
          if (padding_width < 0)
            {
              if (padding_width < -buf_width)
                pad = padding_width + buf_width;
            }
          else
            {
              if (buf_width < padding_width)
                pad = padding_width - buf_width;
            }
        }
    }

  *padding = pad;
  return true;
}

static void
print_padded_number (intmax_t padding)
{
  if (format_str_prefix)
    fputs (format_str_prefix, stdout);

  for (intmax_t p = padding; 0 < p; p--)
    putchar (' ');

  fputs (padding_buffer, stdout);

  for (intmax_t p = padding; p < 0; p++)
    putchar (' ');

  if (format_str_suffix)
    fputs (format_str_suffix, stdout);
}

/* Converts the TEXT number string to the requested representation,
   and handles automatic suffix addition.  */
static int
process_suffixed_number (char *text, long double *result,
                         size_t *precision, long int field)
{
  if (suffix && strlen (text) > strlen (suffix))
    {
      char *possible_suffix = text + strlen (text) - strlen (suffix);

      if (streq (suffix, possible_suffix))
        {
          /* trim suffix, ONLY if it's at the end of the text.  */
          *possible_suffix = '\0';
          devmsg ("trimming suffix %s\n", quote (suffix));
        }
      else
        devmsg ("no valid suffix found\n");
    }

  /* Skip white space - always.  */
  char *p = text;
  while (*p && isblank (to_uchar (*p)))
    ++p;

  /* setup auto-padding.  */
  if (auto_padding)
    {
      padding_width = text < p || 1 < field ? strlen (text) : 0;
      devmsg ("setting Auto-Padding to %jd characters\n", padding_width);
    }

  long double val = 0;
  enum simple_strtod_error e = parse_human_number (p, &val, precision);
  if (e == SSE_OK_PRECISION_LOSS && debug)
    error (0, 0, _("large input value %s: possible precision loss"),
           quote (p));

  if (from_unit_size != 1 || to_unit_size != 1)
    val = (val * from_unit_size) / to_unit_size;

  *result = val;

  return (e == SSE_OK || e == SSE_OK_PRECISION_LOSS);
}

static bool
newline_or_blank (mcel_t g)
{
  return g.ch == '\n' || c32isblank (g.ch);
}

/* Return a pointer to the beginning of the next field in line.
   The line pointer is moved to the end of the next field. */
static char*
next_field (char **line)
{
  char *field_start = *line;
  char *field_end   = field_start;

  if (delimiter != DELIMITER_DEFAULT)
    {
      if (*field_start != delimiter)
        {
          while (*field_end && *field_end != delimiter)
            ++field_end;
        }
      /* else empty field */
    }
  else
    {
      /* keep any space prefix in the returned field */
      field_end = skip_str_matching (field_end, newline_or_blank, true);
      field_end = skip_str_matching (field_end, newline_or_blank, false);
    }

  *line = field_end;
  return field_start;
}

ATTRIBUTE_PURE
static bool
include_field (uintmax_t field)
{
  struct field_range_pair *p = frp;
  if (!p)
    return field == 1;

  while (p->lo != UINTMAX_MAX)
    {
      if (p->lo <= field && p->hi >= field)
        return true;
      ++p;
    }
  return false;
}

/* Convert and output the given field. If it is not included in the set
   of fields to process just output the original */
static bool
process_field (char *text, uintmax_t field)
{
  long double val = 0;
  size_t precision = 0;
  bool valid_number = true;

  if (include_field (field))
    {
      valid_number =
        process_suffixed_number (text, &val, &precision, field);

      intmax_t padding;
      if (valid_number)
        valid_number = prepare_padded_number (val, precision, &padding);

      if (valid_number)
        print_padded_number (padding);
      else
        fputs (text, stdout);
    }
  else
    fputs (text, stdout);

  return valid_number;
}

/* Convert number in a given line of text.
   NEWLINE specifies whether to output a '\n' for this "line".  */
static int
process_line (char *line, bool newline)
{
  char *next;
  uintmax_t field = 0;
  bool valid_number = true;

  while (true) {
    ++field;
    next = next_field (&line);

    if (*line != '\0')
      {
        /* nul terminate the current field string and process */
        *line = '\0';

        if (! process_field (next, field))
          valid_number = false;

        fputc ((delimiter == DELIMITER_DEFAULT) ?
               ' ' : delimiter, stdout);
        ++line;
      }
    else
      {
        /* end of the line, process the last field and finish */
        if (! process_field (next, field))
          valid_number = false;

        break;
      }
  }

  if (newline)
    putchar (line_delim);

  return valid_number;
}

int
main (int argc, char **argv)
{
  int valid_numbers = 1;
  bool locale_ok;

  initialize_main (&argc, &argv);
  set_program_name (argv[0]);
  locale_ok = !!setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

#if HAVE_FPSETPREC
  /* Enabled extended precision if needed.  */
  fpsetprec (FP_PE);
#endif

  decimal_point = nl_langinfo (RADIXCHAR);
  if (decimal_point == nullptr || strlen (decimal_point) == 0)
    decimal_point = ".";
  decimal_point_length = strlen (decimal_point);

  atexit (close_stdout);

  while (true)
    {
      int c = getopt_long (argc, argv, "d:z", longopts, nullptr);

      if (c == -1)
        break;

      switch (c)
        {
        case FROM_OPTION:
          scale_from = XARGMATCH ("--from", optarg,
                                  scale_from_args, scale_from_types);
          break;

        case FROM_UNIT_OPTION:
          from_unit_size = unit_to_umax (optarg);
          break;

        case TO_OPTION:
          scale_to =
            XARGMATCH ("--to", optarg, scale_to_args, scale_to_types);
          break;

        case TO_UNIT_OPTION:
          to_unit_size = unit_to_umax (optarg);
          break;

        case ROUND_OPTION:
          round_style = XARGMATCH ("--round", optarg, round_args, round_types);
          break;

        case GROUPING_OPTION:
          grouping = 1;
          break;

        case PADDING_OPTION:
          if (((xstrtoimax (optarg, nullptr, 10, &padding_width, "")
                & ~LONGINT_OVERFLOW)
               != LONGINT_OK)
              || padding_width == 0)
            error (EXIT_FAILURE, 0, _("invalid padding value %s"),
                   quote (optarg));
          /* TODO: We probably want to apply a specific --padding
             to --header lines too.  */
          break;

        case FIELD_OPTION:
          if (n_frp)
            error (EXIT_FAILURE, 0, _("multiple field specifications"));
          set_fields (optarg, SETFLD_ALLOW_DASH);
          break;

        case 'd':
          /* Interpret -d '' to mean 'use the NUL byte as the delimiter.'  */
          if (optarg[0] != '\0' && optarg[1] != '\0')
            error (EXIT_FAILURE, 0,
                   _("the delimiter must be a single character"));
          delimiter = optarg[0];
          break;

        case 'z':
          line_delim = '\0';
          break;

        case SUFFIX_OPTION:
          suffix = optarg;
          break;

        case DEBUG_OPTION:
          debug = true;
          break;

        case DEV_DEBUG_OPTION:
          dev_debug = true;
          debug = true;
          break;

        case HEADER_OPTION:
          if (optarg)
            {
              if (xstrtoumax (optarg, nullptr, 10, &header, "") != LONGINT_OK
                  || header == 0)
                error (EXIT_FAILURE, 0, _("invalid header value %s"),
                       quote (optarg));
            }
          else
            {
              header = 1;
            }
          break;

        case FORMAT_OPTION:
          format_str = optarg;
          break;

        case INVALID_OPTION:
          inval_style = XARGMATCH ("--invalid", optarg,
                                   inval_args, inval_types);
          break;

          case_GETOPT_HELP_CHAR;
          case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);

        default:
          usage (EXIT_FAILURE);
        }
    }

  if (format_str != nullptr && grouping)
    error (EXIT_FAILURE, 0, _("--grouping cannot be combined with --format"));

  if (debug && ! locale_ok)
    error (0, 0, _("failed to set locale"));

  /* Warn about no-op.  */
  if (debug && scale_from == scale_none && scale_to == scale_none
      && !grouping && (padding_width == 0) && (format_str == nullptr))
    error (0, 0, _("no conversion option specified"));

  if (format_str)
    parse_format_string (format_str);

  if (grouping)
    {
      if (scale_to != scale_none)
        error (EXIT_FAILURE, 0, _("grouping cannot be combined with --to"));
      if (debug && (strlen (nl_langinfo (THOUSEP)) == 0))
        error (0, 0, _("grouping has no effect in this locale"));
    }

  auto_padding = (padding_width == 0 && delimiter == DELIMITER_DEFAULT);

  if (inval_style != inval_abort)
    conv_exit_code = 0;

  if (argc > optind)
    {
      if (debug && header)
        error (0, 0, _("--header ignored with command-line input"));

      for (; optind < argc; optind++)
        valid_numbers &= process_line (argv[optind], true);
    }
  else
    {
      char *line = nullptr;
      size_t line_allocated = 0;
      ssize_t len;

      while (header-- && getdelim (&line, &line_allocated,
                                   line_delim, stdin) > 0)
        fputs (line, stdout);

      while ((len = getdelim (&line, &line_allocated,
                              line_delim, stdin)) > 0)
        {
          bool newline = line[len - 1] == line_delim;
          if (newline)
            line[len - 1] = '\0';
          valid_numbers &= process_line (line, newline);
        }

      if (ferror (stdin))
        error (EXIT_FAILURE, errno, _("error reading input"));
    }

  if (debug && !valid_numbers)
    error (0, 0, _("failed to convert some of the input numbers"));

  int exit_status = EXIT_SUCCESS;
  if (!valid_numbers
      && inval_style != inval_warn && inval_style != inval_ignore)
    exit_status = EXIT_CONVERSION_WARNINGS;

  main_exit (exit_status);
}
