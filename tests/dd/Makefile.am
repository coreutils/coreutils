## Process this file with automake to produce Makefile.in -*-Makefile-*-.

EXTRA_DIST = $(TESTS)

TESTS_ENVIRONMENT = \
  PERL="$(PERL)" \
  CU_TEST_NAME=`basename $(abs_srcdir)`,$$tst \
  PATH="$(VG_PATH_PREFIX)`pwd`/../../src$(PATH_SEPARATOR)$$PATH" \
  PROG=dd

TESTS = \
  misc not-rewound skip-seek skip-seek2 unblock-sync

include $(top_srcdir)/tests/check.mk
