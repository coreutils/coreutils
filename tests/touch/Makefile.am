## Process this file with automake to produce Makefile.in -*-Makefile-*-.
AUTOMAKE_OPTIONS = 1.3 gnits

TESTS = \
  not-owner \
  no-create-missing \
  fail-diag dir-1 dangling-symlink empty-file fifo no-rights obsolescent
EXTRA_DIST = $(TESTS)
TESTS_ENVIRONMENT = \
  host_triplet=$(host_triplet) \
  PATH="`pwd`/../../src@PATH_SEPARATOR@$$PATH"
