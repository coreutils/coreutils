## Process this file with automake to produce Makefile.in -*-Makefile-*-
info_TEXINFOS = fileutils.texi

EXTRA_DIST = perm.texi getdate.texi

# Tell makeinfo to put everything in a single info file: sh-utils.info.
# Otherwise, it would also generate files with names like sh-utils.info-[123],
# and those names all map to one 14-byte name (sh-utils.info-) on some crufty
# old systems.
MAKEINFO = makeinfo --no-split
