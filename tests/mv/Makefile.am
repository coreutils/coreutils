## Process this file with automake to produce Makefile.in -*-Makefile-*-.
AUTOMAKE_OPTIONS = 1.3 gnits

TESTS = mv-special-1 into-self into-self-2 backup-is-src i-1
EXTRA_DIST = $(TESTS) setup
TESTS_ENVIRONMENT = \
  DF=../../src/df \
  LN=../../src/ln \
  LS=../../src/ls \
  MKDIR=../../src/mkdir \
  MKNOD=../../src/mknod \
  MV=../../src/mv \
  RM=../../src/rm \
  TOUCH=../../src/touch \
  PERL=@PERL@ \
  PATH=../../src:$$PATH \
  PROG=mv
