# Make coreutils.					-*-Makefile-*-

# Copyright (C) 1990-2013 Free Software Foundation, Inc.

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

ALL_RECURSIVE_TARGETS =

SUBDIRS = po . gnulib-tests

changelog_etc =				\
  ChangeLog-2005			\
  ChangeLog-2006			\
  ChangeLog-2007			\
  ChangeLog-2008			\
  build-aux/ChangeLog-2007		\
  doc/ChangeLog-2007			\
  lib/ChangeLog-2007			\
  m4/ChangeLog-2007			\
  old/fileutils/ChangeLog		\
  old/fileutils/ChangeLog-1997		\
  old/fileutils/NEWS			\
  old/sh-utils/ChangeLog		\
  old/sh-utils/ChangeLog.0		\
  old/sh-utils/NEWS			\
  old/textutils/ChangeLog		\
  old/textutils/NEWS			\
  po/ChangeLog-2007

EXTRA_DIST =				\
  $(changelog_etc)			\
  .mailmap				\
  .prev-version				\
  .version				\
  .vg-suppressions			\
  THANKS.in				\
  THANKS-to-translators			\
  THANKStt.in				\
  bootstrap				\
  bootstrap.conf			\
  build-aux/gen-lists-of-programs.sh	\
  cfg.mk				\
  dist-check.mk				\
  maint.mk				\
  tests/GNUmakefile			\
  thanks-gen

gen_progs_lists = $(top_srcdir)/build-aux/gen-lists-of-programs.sh

# Keep these in sync with bootstrap.conf:bootstrap_post_import_hook().
# Use '$(top_srcdir)/m4' and '$(srcdir)/src' for the benefit of non-GNU
# makes: it is with those directories that 'cu-progs.m4' and 'cu-progs.mk'
# appear in our dependencies.
$(top_srcdir)/m4/cu-progs.m4: $(gen_progs_lists)
	$(AM_V_GEN)rm -f $@ $@-t \
	  && $(SHELL) $(gen_progs_lists) --autoconf >$@-t \
	  && chmod a-w $@-t && mv -f $@-t $@
$(srcdir)/src/cu-progs.mk: $(gen_progs_lists)
	$(AM_V_GEN)rm -f $@ $@-t \
	  && $(SHELL) $(gen_progs_lists) --automake >$@-t \
	  && chmod a-w $@-t && mv -f $@-t $@

ACLOCAL_AMFLAGS = -I m4

# Shortcut targets to make it easier to run (very) expensive tests.
check-expensive:
	$(MAKE) check RUN_EXPENSIVE_TESTS=yes
check-very-expensive:
	$(MAKE) check-expensive RUN_VERY_EXPENSIVE_TESTS=yes

# Just prior to distribution, ...
# transform the automake-generated rule that runs 'rm -f rm'.
# On some systems, that command would fail with a diagnostic like
# "rm: cannot unlink 'rm': Text file busy" when '.' appears so early
# in the shell's search path that running 'rm' would run the 'rm'
# executable in the current directory.
# Similarly, adjust the clean-binPROGRAMS rule.
rm_subst = \
  s!(rm -f (rm\b|\$$\(bin_PROGRAMS\)$$))!$$1 > /dev/null 2>&1 || /bin/$$1!

BUILT_SOURCES = .version
.version:
	$(AM_V_GEN)echo $(VERSION) > $@-t && mv $@-t $@

# Arrange so that .tarball-version appears only in the distribution
# tarball, and never in a checked-out repository.
# The perl substitution is to change some key uses of "rm" to "/bin/rm".
# See the rm_subst comment for details.
dist-hook: gen-ChangeLog
	$(AM_V_GEN)echo $(VERSION) > $(distdir)/.tarball-version
	$(AM_V_at)perl -pi -e '$(rm_subst)' $(distdir)/Makefile.in

gen_start_date = 2008-02-08
.PHONY: gen-ChangeLog
gen-ChangeLog:
	$(AM_V_GEN)if test -d .git; then				\
	  $(top_srcdir)/build-aux/gitlog-to-changelog			\
	    --amend=$(srcdir)/build-aux/git-log-fix			\
	    --since=$(gen_start_date) > $(distdir)/cl-t;		\
	  rm -f $(distdir)/ChangeLog;					\
	  mv $(distdir)/cl-t $(distdir)/ChangeLog;			\
	fi

ALL_RECURSIVE_TARGETS += distcheck-hook
distcheck-hook: check-ls-dircolors
	$(MAKE) my-distcheck
	$(MAKE) taint-distcheck

DISTCLEANFILES = VERSION
MAINTAINERCLEANFILES = THANKS-to-translators
THANKS-to-translators: po/LINGUAS THANKStt.in
	$(AM_V_GEN)(							\
	  cat $(srcdir)/THANKStt.in;					\
	  for lang in `cat $(srcdir)/po/LINGUAS`; do			\
	    echo http://translationproject.org/team/$$lang.html;	\
	  done;								\
	) > $@-tmp && mv $@-tmp $@

# Ensure that the sets of two-letter codes in ls.c and dircolors.c
# remain in sync.
.PHONY: check-ls-dircolors
check-ls-dircolors:
	$(AM_V_GEN)dc=$$(sed -n '/static.*ls_codes\[/,/};'/p	\
	    $(srcdir)/src/dircolors.c				\
	  |sed -n '/^  *"/p'|tr , '\n'|sed 's/^  *//'		\
	  |sed -n 's/^"\(..\)"/\1/p'|sort -u);			\
	ls=$$(sed -n '/static.*indicator_name\[/,/};'/\p	\
	    $(srcdir)/src/ls.c					\
	  |sed -n '/^  *"/p'|tr , '\n'|sed 's/^  *//'		\
	  |sed -n 's/^"\(..\)"/\1/p'|sort -u);			\
	test "$$dc" = "$$ls"

# Sort in traditional ASCII order, regardless of the current locale;
# otherwise we may get into trouble with distinct strings that the
# current locale considers to be equal.
ASSORT = LC_ALL=C sort

# Extract all lines up to the first one starting with "##".
prologue = perl -ne '/^\#\#/ and exit; print' $(srcdir)/THANKS.in

THANKS: THANKS.in Makefile.am .mailmap thanks-gen .version
	$(AM_V_GEN)rm -f $@-t $@;					\
	{								\
	  $(prologue); echo;						\
	  { perl -ne '/^$$/.../^$$/ and print' $(srcdir)/THANKS.in	\
	      | grep -v '^$$' | perl -pe 's/  +/\0/';			\
	    git log --pretty=format:'%aN%x00%aE'			\
	      | $(ASSORT) -u;						\
	  } | $(srcdir)/thanks-gen					\
	    | LC_ALL=en_US.UTF-8 sort -f;				\
	  echo;								\
	  printf ';; %s\n' 'Local Variables:' 'coding: utf-8' End:;	\
	} > $@-t && chmod a-w $@-t && mv $@-t $@

# Some of our git hook scripts are supposed to be identical to git's samples.
# See if they are still in sync.
.PHONY: check-git-hook-script-sync
check-git-hook-script-sync:
	@fail=0;							\
	t=$$(mktemp -d)							\
	  && cd $$t && git init -q && cd .git/hooks			\
	  && for i in pre-commit pre-applypatch applypatch-msg; do	\
	       diff $(abs_top_srcdir)/scripts/git-hooks/$$i $$i.sample	\
		 || fail=1;						\
	     done;							\
	rm -rf $$t;							\
	test $$fail = 0

noinst_LIBRARIES =
MOSTLYCLEANFILES =
CLEANFILES =
MOSTLYCLEANDIRS =

AM_CPPFLAGS = -Ilib -I$(top_srcdir)/lib -Isrc -I$(top_srcdir)/src

include $(top_srcdir)/lib/local.mk
include $(top_srcdir)/src/local.mk
include $(top_srcdir)/doc/local.mk
include $(top_srcdir)/man/local.mk
include $(top_srcdir)/tests/local.mk
