## Process this file with automake to produce Makefile.in -*-Makefile-*-.
TESTS = \
  p-thru-slink \
  p-3 p-1 p-2 p-v special-1 perm parents t-slash p-slashdot
EXTRA_DIST = $(TESTS)
TESTS_ENVIRONMENT = \
  srcdir=$(srcdir) \
  CU_TEST_NAME=`basename $(abs_srcdir)`,$$tst \
  PATH="$(VG_PATH_PREFIX)`pwd`/../../src$(PATH_SEPARATOR)$$PATH"
