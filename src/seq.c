/* seq - print sequence of numbers to standard output.
   Copyright (C) 1994-2025 Free Software Foundation, Inc.

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

/* Written by Ulrich Drepper.  */

#include <config.h>
#include <ctype.h>
#include <getopt.h>
#include <stdio.h>
#include <sys/types.h>

#include "system.h"
#include "c-ctype.h"
#include "cl-strtod.h"
#include "full-write.h"
#include "quote.h"
#include "xstrtod.h"

/* Roll our own isfinite/isnan rather than using <math.h>, so that we don't
   have to worry about linking -lm just for isfinite.  */
#ifndef isfinite
# define isfinite(x) ((x) * 0 == 0)
#endif
#ifndef isnan
# define isnan(x) ((x) != (x))
#endif

/* Limit below which seq_fast has more throughput.
   Determined with: seq 0 200 inf | pv > /dev/null  */
#define SEQ_FAST_STEP_LIMIT 200  /* Keep in sync with texinfo description.  */
#define SEQ_FAST_STEP_LIMIT_DIGITS 3

/* The official name of this program (e.g., no 'g' prefix).  */
#define PROGRAM_NAME "seq"

#define AUTHORS proper_name ("Ulrich Drepper")

/* True if the locale settings were honored.  */
static bool locale_ok;

/* If true print all number with equal width.  */
static bool equal_width;

/* The string used to separate two numbers.  */
static char const *separator;

/* The string output after all numbers have been output.
   Usually "\n" or "\0".  */
static char const terminator[] = "\n";

static struct option const long_options[] =
{
  { "equal-width", no_argument, nullptr, 'w'},
  { "format", required_argument, nullptr, 'f'},
  { "separator", required_argument, nullptr, 's'},
  {GETOPT_HELP_OPTION_DECL},
  {GETOPT_VERSION_OPTION_DECL},
  { nullptr, 0, nullptr, 0}
};

void
usage (int status)
{
  if (status != EXIT_SUCCESS)
    emit_try_help ();
  else
    {
      printf (_("\
Usage: %s [OPTION]... LAST\n\
  or:  %s [OPTION]... FIRST LAST\n\
  or:  %s [OPTION]... FIRST INCREMENT LAST\n\
"), program_name, program_name, program_name);
      fputs (_("\
Print numbers from FIRST to LAST, in steps of INCREMENT.\n\
"), stdout);

      emit_mandatory_arg_note ();

      fputs (_("\
  -f, --format=FORMAT      use printf style floating-point FORMAT\n\
  -s, --separator=STRING   use STRING to separate numbers (default: \\n)\n\
  -w, --equal-width        equalize width by padding with leading zeroes\n\
"), stdout);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      fputs (_("\
\n\
If FIRST or INCREMENT is omitted, it defaults to 1.  That is, an\n\
omitted INCREMENT defaults to 1 even when LAST is smaller than FIRST.\n\
The sequence of numbers ends when the sum of the current number and\n\
INCREMENT would become greater than LAST.\n\
FIRST, INCREMENT, and LAST are interpreted as floating point values.\n\
INCREMENT is usually positive if FIRST is smaller than LAST, and\n\
INCREMENT is usually negative if FIRST is greater than LAST.\n\
INCREMENT must not be 0; none of FIRST, INCREMENT and LAST may be NaN.\n\
"), stdout);
      fputs (_("\
FORMAT must be suitable for printing one argument of type 'double';\n\
it defaults to %.PRECf if FIRST, INCREMENT, and LAST are all fixed point\n\
decimal numbers with maximum precision PREC, and to %g otherwise.\n\
"), stdout);
      emit_ancillary_info (PROGRAM_NAME);
    }
  exit (status);
}

/* A command-line operand.  */
struct operand
{
  /* Its value, converted to 'long double'.  */
  long double value;

  /* Its print width, if it were printed out in a form similar to its
     input form.  An input like "-.1" is treated like "-0.1", and an
     input like "1." is treated like "1", but otherwise widths are
     left alone.  */
  size_t width;

  /* Number of digits after the decimal point, or INT_MAX if the
     number can't easily be expressed as a fixed-point number.  */
  int precision;
};
typedef struct operand operand;

/* Description of what a number-generating format will generate.  */
struct layout
{
  /* Number of bytes before and after the number.  */
  size_t prefix_len;
  size_t suffix_len;
};

/* Read a long double value from the command line.
   Return if the string is correct else signal error.  */

static operand
scan_arg (char const *arg)
{
  operand ret;

  if (! xstrtold (arg, nullptr, &ret.value, cl_strtold))
    {
      error (0, 0, _("invalid floating point argument: %s"), quote (arg));
      usage (EXIT_FAILURE);
    }

  if (isnan (ret.value))
    {
      error (0, 0, _("invalid %s argument: %s"), quote_n (0, "not-a-number"),
             quote_n (1, arg));
      usage (EXIT_FAILURE);
    }

  /* We don't output spaces or '+' so don't include in width */
  while (isspace (to_uchar (*arg)) || *arg == '+')
    arg++;

  /* Default to auto width and precision.  */
  ret.width = 0;
  ret.precision = INT_MAX;

  /* Use no precision (and possibly fast generation) for integers.  */
  char const *decimal_point = strchr (arg, '.');
  if (! decimal_point && ! strchr (arg, 'p') /* not a hex float */)
    ret.precision = 0;

  /* auto set width and precision for decimal inputs.  */
  if (! arg[strcspn (arg, "xX")] && isfinite (ret.value))
    {
      size_t fraction_len = 0;
      ret.width = strlen (arg);

      if (decimal_point)
        {
          fraction_len = strcspn (decimal_point + 1, "eE");
          if (fraction_len <= INT_MAX)
            ret.precision = fraction_len;
          ret.width += (fraction_len == 0                       /* #.  -> #   */
                        ? -1
                        : (decimal_point == arg                 /* .#  -> 0.# */
                           || !c_isdigit (decimal_point[-1]))); /* -.# -> 0.# */
        }
      char const *e = strchr (arg, 'e');
      if (! e)
        e = strchr (arg, 'E');
      if (e)
        {
          long exponent = MAX (strtol (e + 1, nullptr, 10), -LONG_MAX);
          ret.precision += exponent < 0 ? -exponent
                                        : - MIN (ret.precision, exponent);
          /* Don't account for e.... in the width since this is not output.  */
          ret.width -= strlen (arg) - (e - arg);
          /* Adjust the width as per the exponent.  */
          if (exponent < 0)
            {
              if (decimal_point)
                {
                  if (e == decimal_point + 1) /* undo #. -> # above  */
                    ret.width++;
                }
              else
                ret.width++;
              exponent = -exponent;
            }
          else
            {
              if (decimal_point && ret.precision == 0 && fraction_len)
                ret.width--; /* discount space for '.'  */
              exponent -= MIN (fraction_len, exponent);
            }
          ret.width += exponent;
        }
    }

  return ret;
}

/* If FORMAT is a valid printf format for a double argument, return
   its long double equivalent, allocated from dynamic storage, and
   store into *LAYOUT a description of the output layout; otherwise,
   report an error and exit.  */

static char const *
long_double_format (char const *fmt, struct layout *layout)
{
  size_t i;
  size_t prefix_len = 0;
  size_t suffix_len = 0;
  size_t length_modifier_offset;
  bool has_L;

  for (i = 0; ! (fmt[i] == '%' && fmt[i + 1] != '%'); i += (fmt[i] == '%') + 1)
    {
      if (!fmt[i])
        error (EXIT_FAILURE, 0,
               _("format %s has no %% directive"), quote (fmt));
      prefix_len++;
    }

  i++;
  i += strspn (fmt + i, "-+#0 '");
  i += strspn (fmt + i, "0123456789");
  if (fmt[i] == '.')
    {
      i++;
      i += strspn (fmt + i, "0123456789");
    }

  length_modifier_offset = i;
  has_L = (fmt[i] == 'L');
  i += has_L;
  if (fmt[i] == '\0')
    error (EXIT_FAILURE, 0, _("format %s ends in %%"), quote (fmt));
  if (! strchr ("efgaEFGA", fmt[i]))
    error (EXIT_FAILURE, 0,
           _("format %s has unknown %%%c directive"), quote (fmt), fmt[i]);

  for (i++; ; i += (fmt[i] == '%') + 1)
    if (fmt[i] == '%' && fmt[i + 1] != '%')
      error (EXIT_FAILURE, 0, _("format %s has too many %% directives"),
             quote (fmt));
    else if (fmt[i])
      suffix_len++;
    else
      {
        size_t format_size = i + 1;
        char *ldfmt = xmalloc (format_size + 1);
        memcpy (ldfmt, fmt, length_modifier_offset);
        ldfmt[length_modifier_offset] = 'L';
        strcpy (ldfmt + length_modifier_offset + 1,
                fmt + length_modifier_offset + has_L);
        layout->prefix_len = prefix_len;
        layout->suffix_len = suffix_len;
        return ldfmt;
      }
}

/* Actually print the sequence of numbers in the specified range, with the
   given or default stepping and format.  */

static void
print_numbers (char const *fmt, struct layout layout,
               long double first, long double step, long double last)
{
  bool out_of_range = (step < 0 ? first < last : last < first);

  if (! out_of_range)
    {
      long double x = first;
      long double i;

      for (i = 1; ; i++)
        {
          long double x0 = x;
          if (printf (fmt, x) < 0)
            write_error ();
          if (out_of_range)
            break;

          /* Mathematically equivalent to 'x += step;', and typically
             less subject to rounding error.  */
          x = first + i * step;

          out_of_range = (step < 0 ? x < last : last < x);

          if (out_of_range)
            {
              /* If the number just past LAST prints as a value equal
                 to LAST, and prints differently from the previous
                 number, then print the number.  This avoids problems
                 with rounding.  For example, with the x86 it causes
                 "seq 0 0.000001 0.000003" to print 0.000003 instead
                 of stopping at 0.000002.  */

              bool print_extra_number = false;
              long double x_val;
              char *x_str;
              int x_strlen;
              if (locale_ok)
                setlocale (LC_NUMERIC, "C");
              x_strlen = asprintf (&x_str, fmt, x);
              if (locale_ok)
                setlocale (LC_NUMERIC, "");
              if (x_strlen < 0)
                xalloc_die ();
              x_str[x_strlen - layout.suffix_len] = '\0';

              if (xstrtold (x_str + layout.prefix_len, nullptr,
                            &x_val, cl_strtold)
                  && x_val == last)
                {
                  char *x0_str = nullptr;
                  int x0_strlen = asprintf (&x0_str, fmt, x0);
                  if (x0_strlen < 0)
                    xalloc_die ();
                  x0_str[x0_strlen - layout.suffix_len] = '\0';
                  print_extra_number = !streq (x0_str, x_str);
                  free (x0_str);
                }

              free (x_str);
              if (! print_extra_number)
                break;
            }

          if (fputs (separator, stdout) == EOF)
            write_error ();
        }

      if (fputs (terminator, stdout) == EOF)
        write_error ();
    }
}

/* Return the default format given FIRST, STEP, and LAST.  */
static char const *
get_default_format (operand first, operand step, operand last)
{
  static char format_buf[sizeof "%0.Lf" + 2 * INT_STRLEN_BOUND (int)];

  int prec = MAX (first.precision, step.precision);

  if (prec != INT_MAX && last.precision != INT_MAX)
    {
      if (equal_width)
        {
          /* increase first_width by any increased precision in step */
          size_t first_width = first.width + (prec - first.precision);
          /* adjust last_width to use precision from first/step */
          size_t last_width = last.width + (prec - last.precision);
          if (last.precision && prec == 0)
            last_width--;  /* don't include space for '.' */
          if (last.precision == 0 && prec)
            last_width++;  /* include space for '.' */
          if (first.precision == 0 && prec)
            first_width++;  /* include space for '.' */
          size_t width = MAX (first_width, last_width);
          if (width <= INT_MAX)
            {
              int w = width;
              sprintf (format_buf, "%%0%d.%dLf", w, prec);
              return format_buf;
            }
        }
      else
        {
          sprintf (format_buf, "%%.%dLf", prec);
          return format_buf;
        }
    }

  return "%Lg";
}

/* The nonempty char array P represents a valid non-negative decimal integer.
   ENDP points just after the last char in P.
   Adjust the array to describe the next-larger integer and return
   whether this grows the array by one on the left.  */
static bool
incr_grows (char *p, char *endp)
{
  do
    {
      endp--;
      if (*endp < '9')
        {
          (*endp)++;
          return false;
        }
      *endp = '0';
    }
  while (p < endp);

  p[-1] = '1';
  return true;
}

/* Compare A and B with lengths A_LEN and B_LEN.  A and B are integers
   represented by nonempty arrays of digits without redundant leading '0'.
   Return negative if A < B, 0 if A = B, positive if A > B.  */
static int
cmp (char const *a, idx_t a_len, char const *b, idx_t b_len)
{
  return a_len == b_len ? memcmp (a, b, a_len) : _GL_CMP (a_len, b_len);
}

/* Trim leading 0's from S, but if S is all 0's, leave one.
   Return a pointer to the trimmed string.  */
ATTRIBUTE_PURE
static char const *
trim_leading_zeros (char const *s)
{
  char const *p = s;
  while (*s == '0')
    ++s;

  /* If there were only 0's, back up, to leave one.  */
  if (!*s && s != p)
    --s;
  return s;
}

/* Print all whole numbers from A to B, inclusive -- to stdout, each
   followed by a newline.  Then exit.  */
static void
seq_fast (char const *a, char const *b, uintmax_t step)
{
  /* Skip past any redundant leading '0's.  Without this, our naive cmp
     function would declare 000 to be larger than 99.  */
  a = trim_leading_zeros (a);
  b = trim_leading_zeros (b);

  idx_t p_len = strlen (a);
  idx_t b_len = strlen (b);
  bool inf = b_len == 3 && memeq (b, "inf", 4);

  /* Allow for at least 31 digits without realloc.
     1 more than p_len is needed for the inf case.  */
  enum { INITIAL_ALLOC_DIGITS = 31 };
  idx_t inc_size = MAX (MAX (p_len + 1, b_len), INITIAL_ALLOC_DIGITS);
  /* Ensure we only increase by at most 1 digit at buffer boundaries.  */
  static_assert (SEQ_FAST_STEP_LIMIT_DIGITS < INITIAL_ALLOC_DIGITS - 1);

  /* Copy A (sans NUL) to end of new buffer.  */
  char *p0 = xmalloc (inc_size);
  char *endp = p0 + inc_size;
  char *p = memcpy (endp - p_len, a, p_len);

  /* Reduce number of write calls which is seen to
     give a speed-up of more than 2x over naive stdio code
     when printing the first 10^9 integers.  */
  char buf[BUFSIZ];
  char *buf_end = buf + sizeof buf;
  char *bufp = buf;

  while (inf || cmp (p, endp - p, b, b_len) <= 0)
    {
      /* Append number, flushing output buffer while the number's
         digits do not fit with room for a separator or terminator.  */
      char *pp = p;
      while (buf_end - bufp <= endp - pp)
        {
          memcpy (bufp, pp, buf_end - bufp);
          pp += buf_end - bufp;
          if (full_write (STDOUT_FILENO, buf, sizeof buf) != sizeof buf)
            write_error ();
          bufp = buf;
        }

      /* The rest of the number, followed by a separator or terminator,
         will fit.  Tentatively append a separator.  */
      bufp = mempcpy (bufp, pp, endp - pp);
      *bufp++ = *separator;

      /* Grow number buffer if needed for the inf case.  */
      if (p == p0)
        {
          char *new_p0 = xpalloc (nullptr, &inc_size, 1, -1, 1);
          idx_t saved_p_len = endp - p;
          endp = new_p0 + inc_size;
          p = memcpy (endp - saved_p_len, p0, saved_p_len);
          free (p0);
          p0 = new_p0;
        }

      /* Compute next number.  */
      for (uintmax_t n_incr = step; n_incr; n_incr--)
        p -= incr_grows (p, endp);
    }

  /* Write the remaining buffered output with a terminator instead of
     a separator.  */
  idx_t remaining = bufp - buf;
  if (remaining)
    {
      bufp[-1] = *terminator;
      if (full_write (STDOUT_FILENO, buf, remaining) != remaining)
        write_error ();
    }

  exit (EXIT_SUCCESS);
}

/* Return true if S consists of at least one digit and no non-digits.  */
ATTRIBUTE_PURE
static bool
all_digits_p (char const *s)
{
  size_t n = strlen (s);
  return c_isdigit (s[0]) && n == strspn (s, "0123456789");
}

int
main (int argc, char **argv)
{
  int optc;
  operand first = { 1, 1, 0 };
  operand step = { 1, 1, 0 };
  operand last;
  struct layout layout = { 0, 0 };

  /* The printf(3) format used for output.  */
  char const *format_str = nullptr;

  initialize_main (&argc, &argv);
  set_program_name (argv[0]);
  locale_ok = !!setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  equal_width = false;
  separator = "\n";

  /* We have to handle negative numbers in the command line but this
     conflicts with the command line arguments.  So explicitly check first
     whether the next argument looks like a negative number.  */
  while (optind < argc)
    {
      if (argv[optind][0] == '-'
          && ((optc = argv[optind][1]) == '.' || c_isdigit (optc)))
        {
          /* means negative number */
          break;
        }

      optc = getopt_long (argc, argv, "+f:s:w", long_options, nullptr);
      if (optc == -1)
        break;

      switch (optc)
        {
        case 'f':
          format_str = optarg;
          break;

        case 's':
          separator = optarg;
          break;

        case 'w':
          equal_width = true;
          break;

        case_GETOPT_HELP_CHAR;

        case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);

        default:
          usage (EXIT_FAILURE);
        }
    }

  int n_args = argc - optind;
  if (n_args < 1)
    {
      error (0, 0, _("missing operand"));
      usage (EXIT_FAILURE);
    }

  if (3 < n_args)
    {
      error (0, 0, _("extra operand %s"), quote (argv[optind + 3]));
      usage (EXIT_FAILURE);
    }

  if (format_str)
    format_str = long_double_format (format_str, &layout);

  if (format_str != nullptr && equal_width)
    {
      error (0, 0, _("format string may not be specified"
                     " when printing equal width strings"));
      usage (EXIT_FAILURE);
    }

  char const *user_start = n_args == 1 ? "1" : argv[optind];

  /* If the following hold:
     - no format string, [FIXME: relax this, eventually]
     - integer start (or no start)
     - integer end
     - integer increment <= SEQ_FAST_STEP_LIMIT
     then use the much more efficient integer-only code,
     operating on arbitrarily large numbers.  */
  bool fast_step_ok = false;
  if (n_args != 3
      || (all_digits_p (argv[optind + 1])
          && xstrtold (argv[optind + 1], nullptr, &step.value, cl_strtold)
          && 0 < step.value && step.value <= SEQ_FAST_STEP_LIMIT))
    fast_step_ok = true;

  if (all_digits_p (argv[optind])
      && (n_args == 1 || all_digits_p (argv[optind + 1]))
      && (n_args < 3 || (fast_step_ok
                         && all_digits_p (argv[optind + 2])))
      && !equal_width && !format_str && strlen (separator) == 1)
    {
      char const *s1 = user_start;
      char const *s2 = argv[optind + (n_args - 1)];
      seq_fast (s1, s2, step.value);
    }

  last = scan_arg (argv[optind++]);

  if (optind < argc)
    {
      first = last;
      last = scan_arg (argv[optind++]);

      if (optind < argc)
        {
          step = last;
          if (step.value == 0)
            {
              error (0, 0, _("invalid Zero increment value: %s"),
                     quote (argv[optind - 1]));
              usage (EXIT_FAILURE);
            }

          last = scan_arg (argv[optind++]);
        }
    }

  /* Try the fast method again, for integers of the form 1e1 etc.,
     or "inf" end value.  */
  if (first.precision == 0 && step.precision == 0 && last.precision == 0
      && isfinite (first.value) && 0 <= first.value && 0 <= last.value
      && 0 < step.value && step.value <= SEQ_FAST_STEP_LIMIT
      && !equal_width && !format_str && strlen (separator) == 1)
    {
      char *s1;
      char *s2;
      if (all_digits_p (user_start))
        s1 = xstrdup (user_start);
      else if (asprintf (&s1, "%0.Lf", first.value) < 0)
        xalloc_die ();
      if (! isfinite (last.value))
        s2 = xstrdup ("inf"); /* Ensure "inf" is used.  */
      else if (asprintf (&s2, "%0.Lf", last.value) < 0)
        xalloc_die ();

      if (*s1 != '-' && *s2 != '-')
        seq_fast (s1, s2, step.value);

      free (s1);
      free (s2);
      /* Upon any failure, let the more general code deal with it.  */
    }

  if (format_str == nullptr)
    format_str = get_default_format (first, step, last);

  print_numbers (format_str, layout, first.value, step.value, last.value);

  main_exit (EXIT_SUCCESS);
}
