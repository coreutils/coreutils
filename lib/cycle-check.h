#ifndef CYCLE_CHECK_H
# define CYCLE_CHECK_H 1

/* Before including this file, you need something like the following:

	#if HAVE_CONFIG_H
	# include <config.h>
	#endif

	#include <sys/types.h>
	#include <sys/stat.h>

	#if HAVE_STDBOOL_H
	# include <stdbool.h>
	#else
	typedef enum {false = 0, true = 1} bool;
	#endif

   so that the proper identifiers are all declared.  */

# include "dev-ino.h"

struct cycle_check_state
{
  struct dev_ino dev_ino;
  size_t chdir_counter;
  long unsigned int magic;
};

void cycle_check_init (struct cycle_check_state *state);
bool cycle_check (struct cycle_check_state *state, struct stat const *sb);

#endif
