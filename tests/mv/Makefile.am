## Process this file with automake to produce Makefile.in -*-Makefile-*-.
AUTOMAKE_OPTIONS = 1.3 gnits

TESTS = \
  hard-3 \
  hard-2 \
  perm-1 \
  i-link-no \
  part-fail \
  dup-source childproof i-4 update i-2 mv-special-1 \
  into-self into-self-2 into-self-3 into-self-4 \
  backup-is-src \
  i-1 hard-link-1 force partition-perm to-symlink dir-file diag \
  part-symlink part-rename trailing-slash

EXTRA_DIST = $(TESTS) setup
TESTS_ENVIRONMENT = \
  PERL="@PERL@" \
  PATH="`pwd`/../../src@PATH_SEPARATOR@$$PATH" \
  PROG=mv
