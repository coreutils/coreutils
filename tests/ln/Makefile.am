## Process this file with automake to produce Makefile.in -*-Makefile-*-.
TESTS = hard-backup target-1 sf-1 misc backup-1
EXTRA_DIST = $(TESTS)
TESTS_ENVIRONMENT = \
  CU_TEST_NAME=`basename $(abs_srcdir)`,$$tst \
  PATH="$(VG_PATH_PREFIX)`pwd`/../../src$(PATH_SEPARATOR)$$PATH"
