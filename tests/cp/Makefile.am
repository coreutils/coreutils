## Process this file with automake to produce Makefile.in -*-Makefile-*-.
AUTOMAKE_OPTIONS = 1.1 gnits

TESTS = \
  preserve-2 r-vs-symlink link-preserve \
  backup-1 no-deref-link1 no-deref-link2 no-deref-link3 backup-is-src \
  same-file cp-mv-backup symlink-slash slink-2-slink fail-perm dir-slash \
  perm cp-HL special-bits link dir-rm-dest cp-parents deref-slink \
  dir-vs-file into-self
EXTRA_DIST = $(TESTS)
TESTS_ENVIRONMENT = \
  MAKE=$(MAKE) \
  PATH="`pwd`/../../src@PATH_SEPARATOR@$$PATH"
