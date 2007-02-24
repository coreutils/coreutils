## Process this file with automake to produce Makefile.in -*-Makefile-*-.
TESTS = fail-perm ignore t-slash

EXTRA_DIST = $(TESTS)
TESTS_ENVIRONMENT = \
  top_srcdir=$(top_srcdir) \
  srcdir=$(srcdir) \
  PERL="$(PERL)" \
  CU_TEST_NAME=`basename $(abs_srcdir)`,$$tst \
  PATH="$(VG_PATH_PREFIX)`pwd`/../../src$(PATH_SEPARATOR)$$PATH" \
  PROG=rmdir
