## Process this file with automake to produce Makefile.in -*-Makefile-*-.
AUTOMAKE_OPTIONS = 1.3 gnits

TESTS = empty-file dir-1 dangling-symlink no-rights
EXTRA_DIST = $(TESTS)
TESTS_ENVIRONMENT = \
  PATH=`pwd`/../../src:$$PATH
