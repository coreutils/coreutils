## Process this file with automake to produce Makefile.in -*-Makefile-*-
info_TEXINFOS = fileutils.texi

EXTRA_DIST = perm.texi getdate.texi

# Tell makeinfo to put everything in a single info file: fileutils.info.
# Otherwise, it would also generate files with names like fileutils.info-[123],
# and those names all map to one 14-byte name (fileutils.info-) on some crufty
# old systems.
MAKEINFO = makeinfo --no-split
