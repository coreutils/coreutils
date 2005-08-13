## Process this file with automake to produce Makefile.in -*-Makefile-*-.

EXTRA_DIST = $(TESTS)

TESTS_ENVIRONMENT = \
  top_srcdir=$(top_srcdir) \
  srcdir=$(srcdir) \
  PACKAGE_VERSION=$(PACKAGE_VERSION) \
  PERL="$(PERL)" \
  BUILD_SRC_DIR="`pwd`/../../src" \
  PATH="`pwd`/../../src$(PATH_SEPARATOR)$$PATH" \
  PROG=$$tst

TESTS = \
  date \
  tac-continue \
  close-stdout \
  pwd \
  date-sec \
  paste-no-nl \
  stat-fmt \
  expand \
  fold \
  nohup \
  head-elide-tail \
  split-fail \
  false \
  tty-eof \
  printf-hex \
  nl split-l printf split-a head-pos sort head-c csplit nice pathchk1
