# Make coreutils.					-*-Makefile-*-

# Copyright (C) 1990, 1993-2007 Free Software Foundation, Inc.

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

SUBDIRS = lib src doc man po tests
EXTRA_DIST = Makefile.cfg Makefile.maint GNUmakefile \
  .gitignore \
  .hgignore \
  .kludge-stamp .prev-version THANKS-to-translators THANKStt.in \
  .vg-suppressions \
  .x-po-check \
  .x-sc_file_system .x-sc_obsolete_symbols \
  .x-sc_prohibit_atoi_atof \
  .x-sc_require_config_h \
  .x-sc_space_tab .x-sc_sun_os_names \
  .x-sc_trailing_blank \
  .x-sc_unmarked_diagnostics \
  .x-sc_useless_cpp_parens \
  ChangeLog-2005 \
  bootstrap \
  bootstrap.conf \
  build-aux/cvsu \
  build-aux/vc-list-files \
  gl/modules/getloadavg.diff \
  m4/ChangeLog \
  old/fileutils/ChangeLog \
  old/fileutils/ChangeLog-1997 \
  old/fileutils/NEWS \
  old/sh-utils/ChangeLog \
  old/sh-utils/ChangeLog.0 \
  old/sh-utils/NEWS \
  old/textutils/ChangeLog \
  old/textutils/NEWS

install-root:
	cd src && $(MAKE) $@

ACLOCAL_AMFLAGS = -I m4

# Some tests always need root privileges, others need them only sometimes.
check-root:
	cd tests && $(MAKE) $@

dist-hook:
	test -x $(srcdir)/build-aux/install-sh

distcheck-hook:
	$(MAKE) my-distcheck

# Just prior to distribution, ...
# transform the automake-generated rule that runs `rm -f rm'.
# On some systems, that command would fail with a diagnostic like
# `rm: cannot unlink `rm': Text file busy' when `.' appears so early
# in the shell's search path that running `rm' would run the `rm'
# executable in the current directory.
# Similarly, adjust the clean-binPROGRAMS rule.

rm_subst = \
  s!(rm -f (rm|\$$\(bin_PROGRAMS\)))$$!$$1 > /dev/null 2>&1 || /bin/$$1!
MAINTAINERCLEANFILES = .kludge-stamp
.kludge-stamp: $(srcdir)/src/Makefile.in
	perl -pi -e '$(rm_subst)' $(srcdir)/src/Makefile.in
	touch $@


MAINTAINERCLEANFILES += THANKS-to-translators
THANKS-to-translators: po/LINGUAS THANKStt.in
	(								\
	  cat $(srcdir)/THANKStt.in;					\
	  for lang in `cat $(srcdir)/po/LINGUAS`; do				\
	    echo http://www.iro.umontreal.ca/contrib/po/HTML/team-$$lang.html; \
	  done;								\
	) > $@-tmp && mv $@-tmp $@
