## Process this file with automake to produce Makefile.in -*-Makefile-*-.

AUTOMAKE_OPTIONS = 1.2 gnits

TESTS = rt-1 time-1 symlink-slash follow-slink
EXTRA_DIST = $(TESTS)
TESTS_ENVIRONMENT = \
  top_srcdir=$(top_srcdir) \
  srcdir=$(srcdir) \
  PATH=`pwd`/../../src:$$PATH \
  PROG=ls
