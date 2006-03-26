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
  base64 \
  basename \
  close-stdout \
  csplit \
  date \
  date-sec \
  dirname \
  expand \
  false \
  fold \
  head-c \
  head-elide-tail \
  head-pos \
  mknod \
  nice \
  nl \
  nohup \
  paste-no-nl \
  pathchk1 \
  printf \
  printf-hex \
  pwd \
  sha224sum \
  sha256sum \
  sha384sum \
  sha512sum \
  sort \
  sort-rand \
  split-a \
  split-fail \
  split-l \
  stat-fmt \
  stat-printf \
  tac-continue \
  test \
  tty-eof
