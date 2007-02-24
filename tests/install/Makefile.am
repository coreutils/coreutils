## Process this file with automake to produce Makefile.in -*-Makefile-*-.
TESTS = trap basic-1 create-leading d-slashdot
EXTRA_DIST = $(TESTS)
TESTS_ENVIRONMENT = \
  EXEEXT='$(EXEEXT)' \
  CU_TEST_NAME=`basename $(abs_srcdir)`,$$tst \
  PATH="$(VG_PATH_PREFIX)`pwd`/../../src$(PATH_SEPARATOR)$$PATH"
