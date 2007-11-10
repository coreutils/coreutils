# Make coreutils.					-*-Makefile-*-

# Copyright (C) 1990, 1993-2007 Free Software Foundation, Inc.

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

SUBDIRS = lib src doc man po tests gnulib-tests
EXTRA_DIST = Makefile.cfg Makefile.maint GNUmakefile \
  .prev-version THANKS-to-translators THANKStt.in \
  .vg-suppressions \
  .x-po-check \
  .x-sc_GPL_version \
  .x-sc_file_system .x-sc_obsolete_symbols \
  .x-sc_prohibit_atoi_atof \
  .x-sc_prohibit_strcmp \
  .x-sc_require_config_h \
  .x-sc_space_tab .x-sc_sun_os_names \
  .x-sc_trailing_blank \
  .x-sc_unmarked_diagnostics \
  .x-sc_useless_cpp_parens \
  ChangeLog-2005 \
  bootstrap \
  bootstrap.conf \
  build-aux/cvsu \
  build-aux/git-version-gen \
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

# Just prior to distribution, ...
# transform the automake-generated rule that runs `rm -f rm'.
# On some systems, that command would fail with a diagnostic like
# `rm: cannot unlink `rm': Text file busy' when `.' appears so early
# in the shell's search path that running `rm' would run the `rm'
# executable in the current directory.
# Similarly, adjust the clean-binPROGRAMS rule.
rm_subst = \
  s!(rm -f (rm\b|\$$\(bin_PROGRAMS\)$$))!$$1 > /dev/null 2>&1 || /bin/$$1!

# Arrange so that .version appears only in distribution tarballs,
# never in a checked-out repository.
# The perl substitution is to change some key uses of "rm" to "/bin/rm".
# See the rm_subst comment for details.
dist-hook:
	echo $(VERSION) > $(distdir)/.version
	perl -pi -e '$(rm_subst)' $(distdir)/src/Makefile.in

distcheck-hook:
	$(MAKE) my-distcheck

MAINTAINERCLEANFILES = THANKS-to-translators
THANKS-to-translators: po/LINGUAS THANKStt.in
	(								\
	  cat $(srcdir)/THANKStt.in;					\
	  for lang in `cat $(srcdir)/po/LINGUAS`; do				\
	    echo http://www.iro.umontreal.ca/contrib/po/HTML/team-$$lang.html; \
	  done;								\
	) > $@-tmp && mv $@-tmp $@
