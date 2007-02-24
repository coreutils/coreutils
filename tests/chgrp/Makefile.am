## Process this file with automake to produce Makefile.in -*-Makefile-*-.
TESTS = \
  default-no-deref \
  basic \
  deref \
  no-x \
  posix-H \
  recurse

EXTRA_DIST = $(TESTS)
TESTS_ENVIRONMENT = \
  host_triplet=$(host_triplet) \
  CU_TEST_NAME=`basename $(abs_srcdir)`,$$tst \
  PATH="$(VG_PATH_PREFIX)`pwd`/../../src$(PATH_SEPARATOR)$$PATH"
