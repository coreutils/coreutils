## Process this file with automake to produce Makefile.in -*-Makefile-*-.
AUTOMAKE_OPTIONS = 1.1 gnits

TESTS = r-1 r-2 r-3 i-1 ir-1 f-1 sunos-1 deep-1 # unreadable
EXTRA_DIST = $(TESTS)
TESTS_ENVIRONMENT = \
  RM=../../src/rm \
  MKDIR=../../src/mkdir \
  top_srcdir=$(top_srcdir) \
  srcdir=$(srcdir) \
  PERL="@PERL@" \
  PATH=../../src:$$PATH \
  PROG=rm
