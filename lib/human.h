#ifndef HUMAN_H_
# define HUMAN_H_ 1

/* A conservative bound on the maximum length of a human-readable string.
   The output can be the product of the largest uintmax_t and the largest int,
   so add their sizes before converting to a bound on digits.  */
#define LONGEST_HUMAN_READABLE ((sizeof (uintmax_t) + sizeof (int)) * CHAR_BIT / 3)

#ifndef __P
# if defined (__GNUC__) || (defined (__STDC__) && __STDC__)
#  define __P(args) args
# else
#  define __P(args) ()
# endif  /* GCC.  */
#endif  /* Not __P.  */

char *human_readable __P ((uintmax_t, char *, int, int, int));

#endif /* HUMAN_H_ */
