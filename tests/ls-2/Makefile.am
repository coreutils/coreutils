## Process this file with automake to produce Makefile.in -*-Makefile-*-.

EXTRA_DIST = $(TESTS) T.pm

TESTS_ENVIRONMENT = \
  top_srcdir=$(top_srcdir) \
  srcdir=$(srcdir) \
  PERL="@PERL@" \
  PATH=../../src:$$PATH \
  LS=ls

TESTS = run-test
