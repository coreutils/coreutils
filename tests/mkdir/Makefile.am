## Process this file with automake to produce Makefile.in -*-Makefile-*-.
TESTS = \
  p-1 \
  p-2 \
  p-3 \
  p-slashdot \
  p-thru-slink \
  p-v \
  parents \
  perm \
  special-1 \
  t-slash \
  writable-under-readonly

EXTRA_DIST = $(TESTS)
TESTS_ENVIRONMENT = \
  CU_TEST_NAME=`basename $(abs_srcdir)`,$$tst \
  PATH="$(VG_PATH_PREFIX)`pwd`/../../src$(PATH_SEPARATOR)$$PATH"

include $(top_srcdir)/tests/check.mk
