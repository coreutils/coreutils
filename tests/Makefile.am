## Process this file with automake to produce Makefile.in -*-Makefile-*-.

# Sort in traditional ASCII order, regardless of the current locale;
# otherwise we may get into trouble with distinct strings that the
# current locale considers to be equal.
ASSORT = LC_ALL=C sort

EXTRA_DIST =		\
  Coreutils.pm		\
  CuTmpdir.pm		\
  Makefile.am.in	\
  README		\
  check.mk		\
  envvar-check		\
  expensive		\
  group-names		\
  input-tty		\
  lang-default		\
  mk-script		\
  other-fs-tmpdir	\
  require-perl		\
  rwx-to-mode		\
  sample-test		\
  setgid-check		\
  sparse-file		\
  test-lib.sh		\
  umask-check

# Regarding ordering in SUBDIRS, place early in the list the tools that
# are most commonly used in test scripts.  Every test script uses rm
# and chmod, so they have to be very early.
# Ordering within misc/ should handle the rest.

## N O T E :: Please do not add new tests/ directories.
## There are too many already.  Put new tests in misc/.
SUBDIRS = \
  rm \
  chmod \
  misc \
  chgrp chown cp cut dd du head \
  install join ln ls mkdir mv pr readlink rmdir \
  sort tac tail tail-2 test touch tr \
  uniq wc
## N O T E :: Please do not add new directories.

all_t = t1 t2 t3 t4 t5 t6 t7 t8 t9 ta tb tc td
.PHONY: check-root $(all_t)
check-root: $(all_t)

t1:
	cd chown && $(MAKE) check TESTS=basic
t2:
	cd cp    && $(MAKE) check TESTS=special-bits
t3:
	cd rm    && $(MAKE) check TESTS=no-give-up
t4:
	cd rm    && $(MAKE) check TESTS=fail-2eperm
t5:
	cd tail-2 && $(MAKE) check TESTS=append-only
t6:
	cd rm    && $(MAKE) check TESTS=one-file-system
t7:
	cd ls    && $(MAKE) check TESTS=nameless-uid
t8:
	cd misc  && $(MAKE) check TESTS=chcon
t9:
	cd cp    && $(MAKE) check TESTS=cp-a-selinux
ta:
	cd mkdir && $(MAKE) check TESTS=writable-under-readonly
tb:
	cd mv    && $(MAKE) check TESTS=sticky-to-xpart
tc:
	cd cp    && $(MAKE) check TESTS=preserve-gid
td:
	cd touch && $(MAKE) check TESTS=now-owned-by-other

check-recursive: root-hint

# Advertise `check-root' target.
.PHONY: root-hint
root-hint:
	@echo '***********************************************************'
	@echo "NOTICE: Some tests may be run only as root."
	@echo "  Do \`make check-root' as \`root' to run these tests."
	@echo '***********************************************************'
