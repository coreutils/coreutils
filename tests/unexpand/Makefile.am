## Process this file with automake to produce Makefile.in -*-Makefile-*-.

EXTRA_DIST = $(TESTS)

TESTS_ENVIRONMENT = \
  top_srcdir=$(top_srcdir) \
  srcdir=$(srcdir) \
  PERL="@PERL@" \
  PATH="`pwd`/../../src@PATH_SEPARATOR@$$PATH" \
  PROG=unexpand

TESTS = basic-1
