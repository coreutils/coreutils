## Process this file with automake to produce Makefile.in -*-Makefile-*-.

AUTOMAKE_OPTIONS = 1.2 gnits

TESTS = \
  inode dangle file-type recursive dired infloop \
  rt-1 time-1 symlink-slash follow-slink no-arg

EXTRA_DIST = $(TESTS)
TESTS_ENVIRONMENT = \
  top_srcdir=$(top_srcdir) \
  srcdir=$(srcdir) \
  PATH="`pwd`/../../src@PATH_SEPARATOR@$$PATH" \
  PROG=ls
