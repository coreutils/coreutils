## Process this file with automake to produce Makefile.in -*-Makefile-*-.
AUTOMAKE_OPTIONS = 1.4 gnits

TESTS = \
  basic \
  deref \
  separator
EXTRA_DIST = $(TESTS)
TESTS_ENVIRONMENT = \
  PATH="$(VG_PATH_PREFIX)`pwd`/../../src$(PATH_SEPARATOR)$$PATH"
