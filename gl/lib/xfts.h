#include "fts_.h"

FTS *
xfts_open (char * const *, int options,
           int (*) (const FTSENT **, const FTSENT **))
  _GL_ATTRIBUTE_MALLOC _GL_ATTRIBUTE_DEALLOC (fts_close, 1)
  _GL_ATTRIBUTE_NONNULL ((1)) _GL_ATTRIBUTE_RETURNS_NONNULL;

bool
cycle_warning_required (FTS const *fts, FTSENT const *ent)
  _GL_ATTRIBUTE_NONNULL () _GL_ATTRIBUTE_PURE;
