## Process this file with automake to produce Makefile.in -*-Makefile-*-.

# Sort in traditional ASCII order, regardless of the current locale;
# otherwise we may get into trouble with distinct strings that the
# current locale considers to be equal.
ASSORT = LC_ALL=C sort

all_programs = (cd ../src && MAKEFLAGS= $(MAKE) -s all_programs.list)

TESTS = help-version
TESTS_ENVIRONMENT = \
  all_programs="`$(all_programs)`" \
  PACKAGE_BUGREPORT=$(PACKAGE_BUGREPORT) \
  CU_TEST_NAME=`basename $(abs_srcdir)`,$$tst \
  PATH="$(VG_PATH_PREFIX)`pwd`/../src$(PATH_SEPARATOR)$$PATH"

EXTRA_DIST = \
  $(TESTS) Coreutils.pm Makefile.am.in README acl envvar-check \
  expensive group-names input-tty lang-default mk-script \
  other-fs-tmpdir priv-check \
  rwx-to-mode sample-test setgid-check sparse-file \
  umask-check very-expensive

## N O T E :: Please do not add new tests/ directories.
## There are too many already.  Put new tests in misc/.
SUBDIRS = \
  chgrp chmod chown cp cut dd dircolors du expr factor fmt head \
  install join ln ls ls-2 md5sum misc mkdir mv od pr readlink rm rmdir \
  seq sha1sum shred sort stty sum tac tail tail-2 tee test touch tr \
  tsort unexpand uniq wc
## N O T E :: Please do not add new directories.

all_t = t1 t2 t3 t4 t5 t6 t7
.PHONY: check-root $(all_t)
check-root: $(all_t)

t1:
	cd chown && $(MAKE) check TESTS=basic
t2:
	cd cp    && $(MAKE) check TESTS=special-bits
t3:
	cd rm    && $(MAKE) check TESTS=no-give-up
t4:
	cd rm    && $(MAKE) check TESTS=fail-2eperm
t5:
	cd tail-2 && $(MAKE) check TESTS=append-only
t6:
	cd rm    && $(MAKE) check TESTS=one-file-system
t7:
	cd ls    && $(MAKE) check TESTS=nameless-uid

check-recursive: root-hint

# Advertise `check-root' target.
.PHONY: root-hint
root-hint:
	@echo '***********************************************************'
	@echo "NOTICE: Some tests may be run only as root."
	@echo "  Do \`make check-root' as \`root' to run these tests."
	@echo '***********************************************************'
