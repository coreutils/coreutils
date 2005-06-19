## Process this file with automake to produce Makefile.in -*-Makefile-*-
info_TEXINFOS = coreutils.texi

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

constants.texi: $(top_srcdir)/src/tail.c
	LC_ALL=C \
	sed -n -e 's/^#define \(DEFAULT_MAX[_A-Z]*\) \(.*\)/@set \1 \2/p' \
	  $(top_srcdir)/src/tail.c > t-$@
	mv t-$@ $@

MAINTAINERCLEANFILES = constants.texi

$(DVIS): $(EXTRA_DIST)
$(INFO_DEPS): $(EXTRA_DIST)

# Extended regular expressions to match word starts and ends.
_W = (^|[^A-Za-z0-9_])
W_ = ([^A-Za-z0-9_]|$$)

# List words/regexps here that should not appear in the texinfo documentation.
# E.g., use @sc{nul}, not `NUL'
# and use `time zone', not `timezone'.
check-texinfo:
	fail=0; \
	grep timezone $(srcdir)/*.texi && fail=1; \
	$(EGREP) '$(_W)IO$(W_)' $(srcdir)/*.texi && fail=1; \
	grep non-zero $(srcdir)/*.texi && fail=1; \
	grep '@url{' $(srcdir)/*.texi && fail=1; \
	$(EGREP) '$(_W)NUL$(W_)' $(srcdir)/*.texi && fail=1; \
	grep '\$$@"' $(srcdir)/*.texi && fail=1; \
	grep -n '[^[:punct:]]@footnote' $(srcdir)/*.texi && fail=1; \
	grep -n filename $(srcdir)/*.texi|grep -vE 'setfilename|{filename}' \
	  && fail=1; \
	$(PERL) -e 1 2> /dev/null && { $(PERL) -ne \
	  '/\bPOSIX\b/ && !/\@acronym{POSIX}/ && !/^\* / || /{posix}/ and print,exit 1' \
	  $(srcdir)/*.texi 2> /dev/null || fail=1; }; \
	grep -iwE 'builtins?' $(srcdir)/*.texi && fail=1; \
	grep -iwE 'path(name)?s?' $(srcdir)/*.texi \
	  | grep -vE '@vindex PATH$$|@env{PATH}' && fail=1; \
	exit $$fail

check: check-texinfo
