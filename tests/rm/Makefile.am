## Process this file with automake to produce Makefile.in -*-Makefile-*-.
AUTOMAKE_OPTIONS = 1.1 gnits

TESTS = \
  fail-2eperm \
  cycle i-no-r fail-eperm \
  dangling-symlink rm1 rm2 rm3 rm4 rm5 \
  unread2 r-1 r-2 r-3 i-1 ir-1 f-1 sunos-1 deep-1 hash \
  isatty # unreadable empty-name
EXTRA_DIST = $(TESTS)
TESTS_ENVIRONMENT = \
  RM=../../src/rm \
  MKDIR=../../src/mkdir \
  top_srcdir=$(top_srcdir) \
  srcdir=$(srcdir) \
  PERL="@PERL@" \
  PATH="`pwd`/../../src@PATH_SEPARATOR@$$PATH" \
  PROG=rm
