## Process this file with automake to produce Makefile.in -*-Makefile-*-.
AUTOMAKE_OPTIONS = 1.4 gnits

TESTS = \
  hard-link 8gb basic restore-wd \
  exclude no-x no-deref trailing-slash deref two-args slink
EXTRA_DIST = $(TESTS)
TESTS_ENVIRONMENT = \
  PATH="`pwd`/../../src@PATH_SEPARATOR@$$PATH"
