#ifndef ROOT_DEV_INO_H
# define ROOT_DEV_INO_H 1

# include "dev-ino.h"

struct dev_ino *
get_root_dev_ino (struct dev_ino *root_d_i);

/* These macros are common to the programs that support the
   --preserve-root and --no-preserve-root options.  */

# define ROOT_DEV_INO_CHECK(Root_dev_ino, Dir_statbuf) \
    (Root_dev_ino && SAME_INODE (*Dir_statbuf, *Root_dev_ino))

# define ROOT_DEV_INO_WARN(Dirname)					\
  do									\
    {									\
      if (STREQ (Dirname, "/"))						\
	error (0, 0, _("it is dangerous to operate recursively on %s"),	\
	       quote (Dirname));					\
      else								\
	error (0, 0,							\
	       _("it is dangerous to operate recursively on %s (same as %s)"), \
	       quote_n (0, Dirname), quote_n (1, "/"));			\
      error (0, 0, _("use --no-preserve-root to override this failsafe")); \
    }									\
  while (0)

#endif
