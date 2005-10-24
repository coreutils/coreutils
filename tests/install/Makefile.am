## Process this file with automake to produce Makefile.in -*-Makefile-*-.
AUTOMAKE_OPTIONS = 1.3 gnits

TESTS = trap basic-1 create-leading d-slashdot
EXTRA_DIST = $(TESTS)
TESTS_ENVIRONMENT = \
  EXEEXT='$(EXEEXT)' \
  PATH="`pwd`/../../src$(PATH_SEPARATOR)$$PATH"
