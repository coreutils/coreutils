#ifndef HUMAN_H_
# define HUMAN_H_ 1

/* A conservative bound on the maximum length of a human-readable string.
   The output can be the product of the largest uintmax_t and the largest int,
   so add their sizes before converting to a bound on digits.  */
# define LONGEST_HUMAN_READABLE ((sizeof (uintmax_t) + sizeof (int)) \
				 * CHAR_BIT / 3)

# ifndef PARAMS
#  if defined PROTOTYPES || (defined __STDC__ && __STDC__)
#   define PARAMS(Args) Args
#  else
#   define PARAMS(Args) ()
#  endif
# endif

char *human_readable PARAMS ((uintmax_t, char *, int, int));

void human_block_size PARAMS ((char const *, int, int *));

#endif /* HUMAN_H_ */
