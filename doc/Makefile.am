## Process this file with automake to produce Makefile.in
info_TEXINFOS = fileutils.texi

# FIXME: remove texinfo.tex when automake has been fixed to include it
# automatically
EXTRA_DIST = perm.texi getdate.texi texinfo.tex

# Tell makeinfo to put everything in a single info file: sh-utils.info.
# Otherwise, it would also generate files with names like sh-utils.info-[123],
# and those names all map to one 14-byte name (sh-utils.info-) on some crufty
# old systems.
MAKEINFO = makeinfo --no-split
