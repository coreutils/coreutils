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
  group-names		\
  input-tty		\
  lang-default		\
  mk-script		\
  other-fs-tmpdir	\
  require-perl		\
  sample-test		\
  setgid-check		\
  sparse-file		\
  test-lib.sh

## N O T E :: Do not add more names to this list.
## N O T E :: Even these are expected to go away.
SUBDIRS = cut head join pr sort tac tail test tr uniq

root_tests =					\
  chown/basic					\
  cp/cp-a-selinux				\
  cp/preserve-gid				\
  cp/special-bits				\
  ls/nameless-uid				\
  misc/chcon					\
  mkdir/writable-under-readonly			\
  mv/sticky-to-xpart				\
  rm/fail-2eperm				\
  rm/no-give-up					\
  rm/one-file-system				\
  tail-2/append-only				\
  touch/now-owned-by-other

.PHONY: check-root
check-root:
	$(MAKE) check TESTS='$(root_tests)' SUBDIRS=

check-recursive: root-hint

# Advertise `check-root' target.
.PHONY: root-hint
root-hint:
	@echo '***********************************************************'
	@echo "NOTICE: Some tests may be run only as root."
	@echo "  See the 'Running tests as root' section in README."
	@echo '***********************************************************'

EXTRA_DIST += $(TESTS)

# Do not choose a name that is a shell keyword like 'if', or a
# commonly-used utility like 'cat' or 'test', as the name of a test.
# Otherwise, VPATH builds will fail on hosts like Solaris, since they
# will expand 'if test ...' to 'if .../test ...', and the '.../test'
# will execute the test script rather than the standard utility.

# Regarding ordering in SUBDIRS, place early in the list the tools that
# are most commonly used in test scripts.  Nearly every test script uses
# rm and chmod, so they appear early in the list.

# Notes on the ordering of these tests:
# help-version comes early because it's a basic sanity test.
# Put seq early, since lots of other tests use it.
# Put tests that sleep early, but not all together, so in parallel builds
# they share time with tests that burn CPU, not with others that sleep.
# Put head-elide-tail early, because it's long-running.

TESTS =						\
  misc/help-version				\
  rm/cycle					\
  chmod/no-x					\
  chgrp/basic					\
  rm/dangling-symlink				\
  misc/ls-time					\
  rm/deep-1					\
  rm/deep-2					\
  rm/dir-no-w					\
  rm/dir-nonrecur				\
  rm/dot-rel					\
  rm/isatty					\
  rm/empty-inacc				\
  rm/empty-name					\
  rm/f-1					\
  rm/fail-eacces				\
  rm/fail-eperm					\
  tail-2/assert					\
  rm/hash					\
  rm/i-1					\
  rm/i-never					\
  rm/i-no-r					\
  tail-2/infloop-1				\
  rm/ignorable					\
  rm/inaccessible				\
  rm/interactive-always				\
  rm/interactive-once				\
  rm/ir-1					\
  rm/r-1					\
  rm/r-2					\
  rm/r-3					\
  rm/r-4					\
  rm/readdir-bug				\
  rm/rm1					\
  touch/empty-file				\
  rm/rm2					\
  rm/rm3					\
  rm/rm4					\
  rm/rm5					\
  rm/sunos-1					\
  rm/unread2					\
  rm/unread3					\
  rm/unreadable					\
  rm/v-slash					\
  chgrp/default-no-deref			\
  chgrp/deref					\
  chgrp/no-x					\
  chgrp/posix-H					\
  chgrp/recurse					\
  misc/seq					\
  misc/head-elide-tail				\
  tail-2/tail-n0f				\
  misc/ls-misc					\
  misc/date					\
  misc/ptx-overrun				\
  misc/xstrtol					\
  misc/od					\
  misc/mktemp					\
  misc/arch					\
  misc/pr					\
  misc/df-P					\
  misc/pwd-unreadable-parent			\
  misc/chcon-fail				\
  misc/selinux					\
  misc/cut					\
  misc/wc					\
  misc/wc-files0-from				\
  misc/wc-files0				\
  misc/cat-proc					\
  misc/base64					\
  misc/basename					\
  misc/close-stdout				\
  misc/csplit					\
  misc/date-sec					\
  misc/dircolors				\
  misc/df					\
  misc/dirname					\
  misc/expand					\
  misc/expr					\
  misc/factor					\
  misc/false-status				\
  misc/fmt					\
  misc/fmt-long-line				\
  misc/fold					\
  misc/groups-dash				\
  misc/groups-version				\
  misc/head-c					\
  misc/head-pos					\
  misc/id-context				\
  misc/md5sum					\
  misc/md5sum-newline				\
  misc/mknod					\
  misc/nice					\
  misc/nl					\
  misc/nohup					\
  misc/od-N					\
  misc/od-x8					\
  misc/paste					\
  misc/pathchk1					\
  misc/printf					\
  misc/printf-hex				\
  misc/printf-surprise				\
  misc/pwd-long					\
  misc/readlink-fp-loop				\
  misc/runcon-no-reorder			\
  misc/sha1sum					\
  misc/sha1sum-vec				\
  misc/sha224sum				\
  misc/sha256sum				\
  misc/sha384sum				\
  misc/sha512sum				\
  misc/shred-exact				\
  misc/shred-remove				\
  misc/shuf					\
  misc/sort-compress				\
  misc/sort-merge				\
  misc/sort-rand				\
  misc/split-a					\
  misc/split-fail				\
  misc/split-l					\
  misc/stat-fmt					\
  misc/stat-printf				\
  misc/stty					\
  misc/stty-invalid				\
  misc/stty-row-col				\
  misc/sum					\
  misc/sum-sysv					\
  misc/tac					\
  misc/tac-continue				\
  misc/tee					\
  misc/tee-dash					\
  misc/test-diag				\
  misc/tsort					\
  misc/tty-eof					\
  misc/unexpand					\
  misc/uniq					\
  chmod/c-option				\
  chmod/equal-x					\
  chmod/equals					\
  chmod/inaccessible				\
  chmod/octal					\
  chmod/setgid					\
  chmod/thru-dangling				\
  chmod/umask-x					\
  chmod/usage					\
  chown/deref					\
  chown/preserve-root				\
  chown/separator				\
  cp/abuse					\
  cp/acl					\
  cp/backup-1					\
  cp/backup-dir					\
  cp/backup-is-src				\
  cp/cp-HL					\
  cp/cp-deref					\
  cp/cp-i					\
  cp/cp-mv-backup				\
  cp/cp-parents					\
  cp/deref-slink				\
  cp/dir-rm-dest				\
  cp/dir-slash					\
  cp/dir-vs-file				\
  cp/existing-perm-race				\
  cp/fail-perm					\
  cp/file-perm-race				\
  cp/into-self					\
  cp/link					\
  cp/link-no-deref				\
  cp/link-preserve				\
  cp/no-deref-link1				\
  cp/no-deref-link2				\
  cp/no-deref-link3				\
  cp/parent-perm				\
  cp/parent-perm-race				\
  cp/perm					\
  cp/preserve-2					\
  cp/proc-zero-len				\
  cp/r-vs-symlink				\
  cp/same-file					\
  cp/slink-2-slink				\
  cp/sparse					\
  cp/special-f					\
  cp/src-base-dot				\
  cp/symlink-slash				\
  cp/thru-dangling				\
  dd/misc					\
  dd/not-rewound				\
  dd/skip-seek					\
  dd/skip-seek2					\
  dd/unblock-sync				\
  du/2g						\
  du/8gb					\
  du/basic					\
  du/deref					\
  du/deref-args					\
  du/exclude					\
  du/fd-leak					\
  du/files0-from				\
  du/hard-link					\
  du/inacc-dest					\
  du/inacc-dir					\
  du/inaccessible-cwd				\
  du/long-from-unreadable			\
  du/long-sloop					\
  du/no-deref					\
  du/no-x					\
  du/one-file-system				\
  du/restore-wd					\
  du/slash					\
  du/slink					\
  du/trailing-slash				\
  du/two-args					\
  install/basic-1				\
  install/create-leading			\
  install/d-slashdot				\
  install/trap					\
  ln/backup-1					\
  ln/hard-backup				\
  ln/misc					\
  ln/sf-1					\
  ln/target-1					\
  ls/color-dtype-dir				\
  ls/dangle					\
  ls/dired					\
  ls/file-type					\
  ls/follow-slink				\
  ls/infloop					\
  ls/inode					\
  ls/m-option					\
  ls/no-arg					\
  ls/proc-selinux-segfault			\
  ls/recursive					\
  ls/rt-1					\
  ls/stat-dtype					\
  ls/stat-failed				\
  ls/stat-free-symlinks				\
  ls/stat-vs-dirent				\
  ls/symlink-slash				\
  ls/x-option					\
  mkdir/p-1					\
  mkdir/p-2					\
  mkdir/p-3					\
  mkdir/p-slashdot				\
  mkdir/p-thru-slink				\
  mkdir/p-v					\
  mkdir/parents					\
  mkdir/perm					\
  mkdir/selinux					\
  mkdir/special-1				\
  mkdir/t-slash					\
  mv/acl					\
  mv/atomic					\
  mv/atomic2					\
  mv/backup-dir					\
  mv/backup-is-src				\
  mv/childproof					\
  mv/diag					\
  mv/dir-file					\
  mv/dir2dir					\
  mv/dup-source					\
  mv/force					\
  mv/hard-2					\
  mv/hard-3					\
  mv/hard-4					\
  mv/hard-link-1				\
  mv/hard-verbose				\
  mv/i-1					\
  mv/i-2					\
  mv/i-3					\
  mv/i-4					\
  mv/i-5					\
  mv/i-link-no					\
  mv/into-self					\
  mv/into-self-2				\
  mv/into-self-3				\
  mv/into-self-4				\
  mv/leak-fd					\
  mv/mv-special-1				\
  mv/no-target-dir				\
  mv/part-fail					\
  mv/part-hardlink				\
  mv/part-rename				\
  mv/part-symlink				\
  mv/partition-perm				\
  mv/perm-1					\
  mv/reply-no					\
  mv/to-symlink					\
  mv/trailing-slash				\
  mv/update					\
  readlink/can-e				\
  readlink/can-f				\
  readlink/can-m				\
  readlink/rl-1					\
  rmdir/fail-perm				\
  rmdir/ignore					\
  rmdir/t-slash					\
  tail-2/assert-2				\
  tail-2/big-4gb				\
  tail-2/proc-ksyms				\
  tail-2/start-middle				\
  touch/dangling-symlink			\
  touch/dir-1					\
  touch/fail-diag				\
  touch/fifo					\
  touch/no-create-missing			\
  touch/no-rights				\
  touch/not-owner				\
  touch/obsolescent				\
  touch/read-only				\
  touch/relative				\
  $(root_tests)

include $(top_srcdir)/tests/check.mk
