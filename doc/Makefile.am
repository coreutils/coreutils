# Make coreutils documentation.				-*-Makefile-*-

# Copyright (C) 1995, 1996, 1997, 1998, 2001, 2002, 2003, 2004, 2005,
# 2006 Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
# 02110-1301, USA.

info_TEXINFOS = coreutils.texi

EXTRA_DIST = perm.texi getdate.texi constants.texi fdl.texi

# The following is necessary if the package name is 8 characters or longer.
# If the info documentation would be split into 10 or more separate files,
# then this is necessary even if the package name is 7 characters long.
#
# Tell makeinfo to put everything in a single info file: <package>.info.
# Otherwise, it would also generate files with names like <package>.info-[123],
# and those names all map to one 14-byte name (<package>.info-) on some crufty
# old systems.
AM_MAKEINFOFLAGS = --no-split

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
# Use `time zone', not `timezone'.
# Use `zeros', not `zeroes' (nothing wrong with `zeroes'. just be consistent).
check-texinfo:
	fail=0; \
	grep timezone $(srcdir)/*.texi && fail=1; \
	$(EGREP) '$(_W)IO$(W_)' $(srcdir)/*.texi && fail=1; \
	grep non-zero $(srcdir)/*.texi && fail=1; \
	grep '@url{' $(srcdir)/*.texi && fail=1; \
	$(EGREP) '$(_W)NUL$(W_)' $(srcdir)/*.texi && fail=1; \
	grep '\$$@"' $(srcdir)/*.texi && fail=1; \
	grep -n '[^[:punct:]]@footnote' $(srcdir)/*.texi && fail=1; \
	grep -n filename $(srcdir)/*.texi|$(EGREP) -v 'setfilename|[{]filename[}]' \
	  && fail=1; \
	$(PERL) -e 1 2> /dev/null && { $(PERL) -ne \
	  '/\bPOSIX\b/ && !/\@acronym{POSIX}/ && !/^\* / || /{posix}/ and print,exit 1' \
	  $(srcdir)/*.texi 2> /dev/null || fail=1; }; \
	$(EGREP) -i '$(_W)zeroes$(W_)' $(srcdir)/*.texi && fail=1; \
	$(EGREP) -i '$(_W)builtins?$(W_)' $(srcdir)/*.texi && fail=1; \
	$(EGREP) -i '$(_W)path(name)?s?$(W_)' $(srcdir)/*.texi \
	  | $(EGREP) -v '@vindex PATH$$|@env[{]PATH[}]' && fail=1; \
	exit $$fail

check: check-texinfo
