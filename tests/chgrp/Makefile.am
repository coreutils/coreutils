## Process this file with automake to produce Makefile.in -*-Makefile-*-.
AUTOMAKE_OPTIONS = 1.4 gnits

TESTS = no-x posix-H basic deref recurse
EXTRA_DIST = $(TESTS)
TESTS_ENVIRONMENT = \
  host_triplet=$(host_triplet) \
  PATH="$(VG_PATH_PREFIX)`pwd`/../../src$(PATH_SEPARATOR)$$PATH"
