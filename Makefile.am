## Process this file with automake to produce Makefile.in -*-Makefile-*-

SUBDIRS = lib src doc man m4 po tests
EXTRA_DIST = Makefile.cfg Makefile.maint GNUmakefile \
  .kludge-stamp .prev-version THANKS-to-translators THANKStt.in \
  announce-gen \
  old/fileutils/ChangeLog \
  old/fileutils/ChangeLog-1997 \
  old/sh-utils/ChangeLog \
  old/sh-utils/ChangeLog.0 \
  old/textutils/ChangeLog \
  old/fileutils/NEWS \
  old/sh-utils/NEWS \
  old/textutils/NEWS

install-root:
	cd src && $(MAKE) $@

ACLOCAL_AMFLAGS = -I m4

# Some tests always need root privileges, others need them only sometimes.
check-root:
	cd tests && $(MAKE) $@

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
.kludge-stamp: $(srcdir)/src/Makefile.in
	perl -pi -e '$(rm_subst)' $(srcdir)/src/Makefile.in
	touch $@

THANKS-to-translators: po/LINGUAS THANKStt.in
	(								\
	  cat $(srcdir)/THANKStt.in;					\
	  for lang in `cat po/LINGUAS`; do				\
	    echo http://www.iro.umontreal.ca/contrib/po/HTML/team-$$lang.html; \
	  done;								\
	) > $@-tmp && mv $@-tmp $@
