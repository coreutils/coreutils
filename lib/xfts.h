#include "fts_.h"

FTS *
xfts_open (char * const *, int options,
	   int (*) (const FTSENT **, const FTSENT **));
