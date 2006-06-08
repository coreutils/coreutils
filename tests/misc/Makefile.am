## Process this file with automake to produce Makefile.in -*-Makefile-*-.

EXTRA_DIST = $(TESTS)

TESTS_ENVIRONMENT = \
  top_srcdir=$(top_srcdir) \
  srcdir=$(srcdir) \
  PACKAGE_VERSION=$(PACKAGE_VERSION) \
  PERL="$(PERL)" \
  BUILD_SRC_DIR="`pwd`/../../src" \
  PATH="`pwd`/../../src$(PATH_SEPARATOR)$$PATH" \
  PROG=`../../src/basename -- "$$tst"`

# Do not choose a name that is a shell keyword like 'if', or a
# commonly-used utility like 'cat' or 'test', as the name of a test.
# Otherwise, VPATH builds will fail on hosts like Solaris, since they
# will expand 'if test ...' to 'if .../test ...', and the '.../test'
# will execute the test script rather than the standard utility.

TESTS = \
  cat-proc \
  base64 \
  basename \
  close-stdout \
  csplit \
  date \
  date-sec \
  dirname \
  expand \
  false-status \
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
  pwd-long \
  sha224sum \
  sha256sum \
  sha384sum \
  sha512sum \
  sort-merge \
  sort-rand \
  split-a \
  split-fail \
  split-l \
  stat-fmt \
  stat-printf \
  tac-continue \
  test-diag \
  tty-eof
