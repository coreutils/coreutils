## Process this file with automake to produce Makefile.in -*-Makefile-*-.
AUTOMAKE_OPTIONS = 1.1 gnits

TESTS = \
  backup-1 no-deref-link1 no-deref-link2 no-deref-link3 backup-is-src \
  same-file cp-mv-backup symlink-slash slink-2-slink fail-perm dir-slash \
  perm
EXTRA_DIST = $(TESTS)
TESTS_ENVIRONMENT = \
  PATH=`pwd`/../../src:$$PATH
