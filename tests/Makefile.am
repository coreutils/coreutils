## Process this file with automake to produce Makefile.in -*-Makefile-*-.

TESTS = help-version
TESTS_ENVIRONMENT = \
  all_programs="$(bin_SCRIPTS) $(bin_PROGRAMS)"

EXTRA_DIST = \
  $(TESTS) .env-warn Fetish.pm Makefile.am.in README envvar-check \
  expensive group-names input-tty lang-default mk-script priv-check \
  rwx-to-mode sample-test setgid-check

SUBDIRS = \
  basename chgrp chmod chown cp cut date dd dircolors du expr factor \
  fmt head install join ln ls ls-2 md5sum misc mkdir mv od pr rm rmdir \
  seq sha1sum shred sort stty sum tac tail tail-2 test touch tr tsort \
  unexpand uniq wc

check-root:
	cd chown && $(MAKE) check TESTS=basic
	cd cp    && $(MAKE) check TESTS=special-bits
	cd mv    && $(MAKE) check TESTS=mv-special-1
	cd rm    && $(MAKE) check TESTS=fail-2eperm

check-recursive: evar-check root-hint

# Warn when `make check' is run with POSIXLY_CORRECT or CDPATH set.
.PHONY: evar-check
evar-check:
	../src/printenv POSIXLY_CORRECT >/dev/null \
	  && sed s/%%/POSIXLY_CORRECT/ $(srcdir)/.env-warn || :
	test "$${CDPATH+set}" = set \
	  && sed s/%%/CDPATH/ $(srcdir)/.env-warn || :

# Advertise `check-root' target.
root-hint:
	@echo '********************************************'
	@echo "NOTICE: Some tests will or might need to be"
	@echo "        run as root. Do \`make check-root'"
	@echo "        as \`root' to run just these tests."
	@echo '********************************************'
