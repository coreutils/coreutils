## Process this file with automake to produce Makefile.in -*-Makefile-*-
info_TEXINFOS = omni-utils.texi

EXTRA_DIST = perm.texi getdate.texi constants.texi doclicense.texi

# The following is necessary if the package name is 8 characters or longer.
# If the info documentation would be split into 10 or more separate files,
# then this is necessary even if the package name is 7 characters long.
#
# Tell makeinfo to put everything in a single info file: <package>.info.
# Otherwise, it would also generate files with names like <package>.info-[123],
# and those names all map to one 14-byte name (<package>.info-) on some crufty
# old systems.
MAKEINFO = makeinfo --no-split

# List words/regexps here that should not appear in the texinfo documentation.
check-texinfo:
	grep timezone $(srcdir)/*.texi && exit 1 || :
	grep -w POSIX $(srcdir)/*.texi && exit 1 || :

check: check-texinfo
