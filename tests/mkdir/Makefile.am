## Process this file with automake to produce Makefile.in -*-Makefile-*-.
AUTOMAKE_OPTIONS = 1.1 gnits

TESTS = p-1 p-2 special-1 perm parents t-slash
EXTRA_DIST = $(TESTS)
TESTS_ENVIRONMENT = \
  srcdir=$(srcdir) \
  PATH="`pwd`/../../src@PATH_SEPARATOR@$$PATH"
