## Process this file with automake to produce Makefile.in -*-Makefile-*-.
AUTOMAKE_OPTIONS = 1.3 gnits

TESTS = mv-special-1 into-self backup-is-src
EXTRA_DIST = $(TESTS) setup
TESTS_ENVIRONMENT = \
  DF=../../src/df \
  LS=../../src/ls \
  MKDIR=../../src/mkdir \
  MKNOD=../../src/mknod \
  MV=../../src/mv \
  RM=../../src/rm \
  TOUCH=../../src/touch
