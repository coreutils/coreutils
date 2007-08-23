#include <sys/types.h>
#include <sys/stat.h>
#include <stdbool.h>

#include "hash.h"

extern void record_file (Hash_table *ht, char const *file,
			 struct stat const *stats)
  __attribute__((nonnull(2, 3)));

extern bool seen_file (Hash_table const *ht, char const *file,
		       struct stat const *stats);
