## Process this file with automake to produce Makefile.in -*-Makefile-*-.
TESTS = row-col-1 basic-1
EXTRA_DIST = $(TESTS)
TESTS_ENVIRONMENT = \
  CU_TEST_NAME=`basename $(abs_srcdir)`,$$tst \
  PATH="$(VG_PATH_PREFIX)`pwd`/../../src$(PATH_SEPARATOR)$$PATH"
