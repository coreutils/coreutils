#ifndef CYCLE_CHECK_H
# define CYCLE_CHECK_H 1

# if HAVE_INTTYPES_H
#  include <inttypes.h>
# endif
# if HAVE_STDINT_H
#  include <stdint.h>
# endif
# include <stdbool.h>
# include "dev-ino.h"

struct cycle_check_state
{
  struct dev_ino dev_ino;
  uintmax_t chdir_counter;
  int magic;
};

void cycle_check_init (struct cycle_check_state *state);
bool cycle_check (struct cycle_check_state *state, struct stat const *sb);

#endif
