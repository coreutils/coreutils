#ifndef HUMAN_H_
# define HUMAN_H_ 1

/* Before including this file, you need something like the following:

	#if HAVE_CONFIG_H
	# include <config.h>
	#endif

	#if HAVE_STDBOOL_H
	# include <stdbool.h>
	#else
	typedef enum {false = 0, true = 1} bool;
	#endif

	#if HAVE_INTTYPES_H
	# include <inttypes.h>
	#else
	# if HAVE_STDINT_H
	#  include <stdint.h>
	# endif
	#endif

	#include <limits.h>

   so that the proper identifiers are all declared.  */

/* A conservative bound on the maximum length of a human-readable string.
   The output can be the square of the largest uintmax_t, so double
   its size before converting to a bound.
   302 / 1000 is ceil (log10 (2.0)).  Add 1 for integer division truncation.
   Also, the output can have a thousands separator between every digit,
   so multiply by MB_LEN_MAX + 1 and then subtract MB_LEN_MAX.
   Finally, append 3, the maximum length of a suffix.  */
# define LONGEST_HUMAN_READABLE \
  ((2 * sizeof (uintmax_t) * CHAR_BIT * 302 / 1000 + 1) * (MB_LEN_MAX + 1) \
   - MB_LEN_MAX + 3)

/* Options for human_readable.  */
enum
{
  /* Unless otherwise specified these options may be ORed together.  */

  /* The following three options are mutually exclusive.  */
  /* Round to plus infinity (default).  */
  human_ceiling = 0,
  /* Round to nearest, ties to even.  */
  human_round_to_nearest = 1,
  /* Round to minus infinity.  */
  human_floor = 2,

  /* Group digits together, e.g. `1,000,000'.  This uses the
     locale-defined grouping; the traditional C locale does not group,
     so this has effect only if some other locale is in use.  */
  human_group_digits = 4,

  /* When autoscaling, suppress ".0" at end.  */
  human_suppress_point_zero = 8,

  /* Scale output and use SI-style units, ignoring the output block size.  */
  human_autoscale = 16,

  /* Prefer base 1024 to base 1000.  */
  human_base_1024 = 32,

  /* Append SI prefix, e.g. "k" or "M".  */
  human_SI = 64,

  /* Append "B" (if base 1000) or "iB" (if base 1024) to SI prefix.  */
  human_B = 128
};

char *human_readable (uintmax_t, char *, int, uintmax_t, uintmax_t);

int human_options (char const *, bool, uintmax_t *);

#endif /* HUMAN_H_ */
