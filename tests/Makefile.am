## Process this file with automake to produce Makefile.in -*-Makefile-*-.

## Copyright (C) 2007-2012 Free Software Foundation, Inc.

## This program is free software: you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation, either version 3 of the License, or
## (at your option) any later version.

## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.

## You should have received a copy of the GNU General Public License
## along with this program.  If not, see <http://www.gnu.org/licenses/>.

# Ensure that all version-controlled executable files are listed in TESTS.
# Collect test names from the line matching /^TESTS = \\$$/ to the following
# one that does not end in '\'.
_v = TESTS
_w = root_tests
vc_exe_in_TESTS: Makefile
	$(AM_V_GEN)if test -d $(top_srcdir)/.git && test $(srcdir) = .; then \
	 {								\
	  for list in $(_v) $(_w); do					\
	    sed -n "/^$$list =[	 ]*\\\\$$/,/[^\]$$/p" Makefile.am |	\
	    sed -n 's/^  *\([^$$	 ]\{1,\}\).*/\1/p';		\
	  done;								\
	  for f in `cd $(top_srcdir) &&					\
	   build-aux/vc-list-files $(subdir) | sed 's!^$(subdir)/!!'`; do \
	    test -f "$$f" && test -x "$$f" && echo "$$f";		\
	  done;								\
	 } | sort | uniq -u | grep . && exit 1 ||:;			\
	else :; fi

check: vc_exe_in_TESTS
.PHONY: vc_exe_in_TESTS

CLEANFILES =
CLEANFILES += .built-programs
check_DATA = .built-programs
.built-programs: $(top_srcdir)/src/Makefile.am
	$(AM_V_GEN)(cd $(top_builddir)/src				\
            && MAKEFLAGS= $(MAKE) -s built_programs.list)		\
          > $@-t && mv $@-t $@

# '$f' is set by the Automake-generated test harness to the path of the
# current test script stripped of VPATH components, and is used by the
# shell-or-perl script to determine the name of the temporary files to be
# used.  Note that $f is a shell variable, not a make macro, so the use of
# '$$f' below is correct, and not a typo.
LOG_COMPILER = \
  $(SHELL) $(srcdir)/shell-or-perl \
  --test-name "$$f" --srcdir '$(srcdir)' \
  --shell '$(SHELL)' --perl '$(PERL)' --

# Note that the first lines are statements.  They ensure that environment
# variables that can perturb tests are unset or set to expected values.
# The rest are envvar settings that propagate build-related Makefile
# variables to test scripts.
TESTS_ENVIRONMENT =				\
  . $(srcdir)/lang-default;			\
  tmp__=$${TMPDIR-/tmp};			\
  test -d "$$tmp__" && test -w "$$tmp__" || tmp__=.;	\
  . $(srcdir)/envvar-check;			\
  TMPDIR=$$tmp__; export TMPDIR;		\
  export					\
  VERSION='$(VERSION)'				\
  LOCALE_FR='$(LOCALE_FR)'			\
  LOCALE_FR_UTF8='$(LOCALE_FR_UTF8)'		\
  abs_top_builddir='$(abs_top_builddir)'	\
  abs_top_srcdir='$(abs_top_srcdir)'		\
  abs_srcdir='$(abs_srcdir)'			\
  built_programs="`cat .built-programs`"	\
  host_os=$(host_os)				\
  host_triplet='$(host_triplet)'		\
  srcdir='$(srcdir)'				\
  top_srcdir='$(top_srcdir)'			\
  CONFIG_HEADER='$(abs_top_builddir)/$(CONFIG_INCLUDE)' \
  CU_TEST_NAME=`basename '$(abs_srcdir)'`,`echo $$tst|sed 's,^\./,,;s,/,-,g'` \
  CC='$(CC)'					\
  AWK='$(AWK)'					\
  EGREP='$(EGREP)'				\
  EXEEXT='$(EXEEXT)'				\
  MAKE=$(MAKE)					\
  PACKAGE_BUGREPORT='$(PACKAGE_BUGREPORT)'	\
  PACKAGE_VERSION=$(PACKAGE_VERSION)		\
  PERL='$(PERL)'				\
  PREFERABLY_POSIX_SHELL='$(PREFERABLY_POSIX_SHELL)' \
  ; test -d /usr/xpg4/bin && PATH='/usr/xpg4/bin$(PATH_SEPARATOR)'"$$PATH"; \
  PATH='$(abs_top_builddir)/src$(PATH_SEPARATOR)'"$$PATH" \
  ; 9>&2

# On failure, display the global testsuite log on stdout.
VERBOSE = yes

# Sort in traditional ASCII order, regardless of the current locale;
# otherwise we may get into trouble with distinct strings that the
# current locale considers to be equal.
ASSORT = LC_ALL=C sort

EXTRA_DIST =		\
  Coreutils.pm		\
  CuSkip.pm		\
  CuTmpdir.pm		\
  check.mk		\
  d_type-check		\
  envvar-check		\
  filefrag-extent-compare \
  fiemap-capable	\
  init.cfg		\
  init.sh		\
  lang-default		\
  other-fs-tmpdir	\
  sample-test		\
  shell-or-perl		\
  $(pr_data)

root_tests =					\
  chown/basic					\
  cp/cp-a-selinux				\
  cp/preserve-gid				\
  cp/special-bits				\
  cp/cp-mv-enotsup-xattr			\
  cp/capability					\
  cp/sparse-fiemap				\
  dd/skip-seek-past-dev				\
  df/problematic-chars				\
  du/bind-mount-dir-cycle			\
  install/install-C-root			\
  ls/capability					\
  ls/nameless-uid				\
  misc/chcon					\
  misc/chroot-credentials			\
  misc/id-setgid				\
  misc/selinux					\
  misc/truncate-owned-by-other			\
  mkdir/writable-under-readonly			\
  mv/sticky-to-xpart				\
  rm/fail-2eperm				\
  rm/no-give-up					\
  rm/one-file-system				\
  rm/read-only					\
  tail-2/append-only				\
  touch/now-owned-by-other

.PHONY: check-root
check-root:
	$(MAKE) check TESTS='$(root_tests)'

check-recursive: root-hint

# Advertise 'check-root' target.
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

# Notes on the ordering of these tests:
# Place early in the list tests of the tools that
# are most commonly used in test scripts themselves.
# E.g., nearly every test script uses rm and chmod.
# help-version comes early because it's a basic sanity test.
# Put seq early, since lots of other tests use it.
# Put tests that sleep early, but not all together, so in parallel builds
# they share time with tests that burn CPU, not with others that sleep.
# Put head-elide-tail early, because it's long-running.

TESTS =						\
  misc/help-version				\
  tail-2/inotify-race				\
  misc/invalid-opt				\
  rm/ext3-perf					\
  rm/cycle					\
  cp/link-heap					\
  misc/tty-eof					\
  tail-2/inotify-hash-abuse			\
  tail-2/inotify-hash-abuse2			\
  tail-2/F-vs-missing				\
  tail-2/F-vs-rename				\
  tail-2/inotify-rotate				\
  chmod/no-x					\
  chgrp/basic					\
  rm/dangling-symlink				\
  misc/ls-time					\
  rm/d-1					\
  rm/d-2					\
  rm/d-3					\
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
  rm/one-file-system2				\
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
  rm/many-dir-entries-vs-OOM			\
  chgrp/default-no-deref			\
  chgrp/deref					\
  chgrp/no-x					\
  chgrp/posix-H					\
  chgrp/recurse					\
  fmt/base					\
  fmt/long-line					\
  fmt/goal-option				\
  misc/env					\
  misc/ptx					\
  misc/test					\
  misc/seq					\
  misc/seq-long-double				\
  misc/head					\
  misc/head-elide-tail				\
  tail-2/tail-n0f				\
  misc/ls-misc					\
  misc/date					\
  misc/date-next-dow				\
  misc/ptx-overrun				\
  misc/xstrtol					\
  tail-2/pid					\
  misc/od					\
  misc/od-float					\
  misc/mktemp					\
  misc/arch					\
  misc/pr					\
  misc/join					\
  pr/pr-tests					\
  misc/pwd-option				\
  misc/chcon-fail				\
  misc/cut					\
  misc/wc					\
  misc/wc-files0-from				\
  misc/wc-files0				\
  misc/wc-parallel				\
  misc/cat-proc					\
  misc/cat-buf					\
  misc/base64					\
  misc/basename					\
  misc/close-stdout				\
  misc/chroot-fail				\
  misc/comm					\
  misc/csplit					\
  misc/csplit-1000				\
  misc/csplit-heap				\
  misc/date-sec					\
  misc/dircolors				\
  misc/dirname					\
  misc/env-null					\
  misc/expand					\
  misc/expr					\
  misc/factor					\
  misc/false-status				\
  misc/fold					\
  misc/groups-dash				\
  misc/groups-version				\
  misc/head-c					\
  misc/head-pos					\
  misc/id-context				\
  misc/id-groups				\
  misc/id-setgid				\
  misc/md5sum					\
  misc/md5sum-bsd				\
  misc/md5sum-newline				\
  misc/md5sum-parallel				\
  misc/mknod					\
  misc/nice					\
  misc/nice-fail				\
  misc/nl					\
  misc/nohup					\
  misc/nproc-avail				\
  misc/nproc-positive				\
  misc/od-N					\
  misc/od-multiple-t				\
  misc/od-x8					\
  misc/paste					\
  misc/pathchk1					\
  misc/printenv					\
  misc/printf					\
  misc/printf-cov				\
  misc/printf-hex				\
  misc/printf-surprise				\
  misc/pwd-long					\
  misc/readlink-fp-loop				\
  misc/readlink-root				\
  misc/realpath					\
  misc/runcon-no-reorder			\
  misc/sha1sum					\
  misc/sha1sum-vec				\
  misc/sha224sum				\
  misc/sha256sum				\
  misc/sha384sum				\
  misc/sha512sum				\
  misc/shred-exact				\
  misc/shred-passes				\
  misc/shred-remove				\
  misc/shuf					\
  misc/sort					\
  misc/sort-benchmark-random			\
  misc/sort-compress				\
  misc/sort-compress-hang			\
  misc/sort-compress-proc			\
  misc/sort-continue				\
  misc/sort-debug-keys				\
  misc/sort-debug-warn				\
  misc/sort-discrim				\
  misc/sort-files0-from				\
  misc/sort-float				\
  misc/sort-merge				\
  misc/sort-merge-fdlimit			\
  misc/sort-month				\
  misc/sort-exit-early				\
  misc/sort-rand				\
  misc/sort-spinlock-abuse			\
  misc/sort-stale-thread-mem			\
  misc/sort-unique				\
  misc/sort-unique-segv				\
  misc/sort-version				\
  misc/sort-NaN-infloop				\
  misc/sort-u-FMR				\
  split/filter					\
  split/suffix-auto-length			\
  split/suffix-length				\
  split/additional-suffix			\
  split/b-chunk					\
  split/fail					\
  split/lines					\
  split/l-chunk					\
  split/r-chunk					\
  split/numeric					\
  split/guard-input				\
  misc/stat-birthtime				\
  misc/stat-fmt					\
  misc/stat-hyphen				\
  misc/stat-mount				\
  misc/stat-nanoseconds				\
  misc/stat-printf				\
  misc/stat-slash				\
  misc/stdbuf					\
  misc/stty					\
  misc/stty-invalid				\
  misc/stty-pairs				\
  misc/stty-row-col				\
  misc/sum					\
  misc/sum-sysv					\
  misc/tac					\
  misc/tac-continue				\
  misc/tac-2-nonseekable			\
  misc/tail					\
  misc/tee					\
  misc/tee-dash					\
  misc/test-diag				\
  misc/timeout					\
  misc/timeout-group				\
  misc/timeout-parameters			\
  misc/tr					\
  misc/tr-case-class				\
  misc/truncate-dangling-symlink		\
  misc/truncate-dir-fail			\
  misc/truncate-fail-diag			\
  misc/truncate-fifo				\
  misc/truncate-no-create-missing		\
  misc/truncate-overflow			\
  misc/truncate-parameters			\
  misc/truncate-relative			\
  misc/tsort					\
  misc/unexpand					\
  misc/uniq					\
  misc/uniq-perf				\
  misc/xattr					\
  tail-2/wait					\
  chmod/c-option				\
  chmod/equal-x					\
  chmod/equals					\
  chmod/inaccessible				\
  chmod/octal					\
  chmod/setgid					\
  chmod/silent					\
  chmod/thru-dangling				\
  chmod/umask-x					\
  chmod/usage					\
  chown/deref					\
  chown/preserve-root				\
  chown/separator				\
  cp/abuse					\
  cp/acl					\
  cp/attr-existing				\
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
  cp/existing-perm-dir				\
  cp/existing-perm-race				\
  cp/fail-perm					\
  cp/fiemap-empty                               \
  cp/fiemap-perf                                \
  cp/fiemap-2                                   \
  cp/file-perm-race				\
  cp/into-self					\
  cp/link					\
  cp/link-no-deref				\
  cp/link-preserve				\
  cp/link-symlink				\
  cp/nfs-removal-race				\
  cp/no-deref-link1				\
  cp/no-deref-link2				\
  cp/no-deref-link3				\
  cp/parent-perm				\
  cp/parent-perm-race				\
  cp/perm					\
  cp/preserve-2					\
  cp/preserve-link				\
  cp/preserve-slink-time			\
  cp/proc-short-read				\
  cp/proc-zero-len				\
  cp/r-vs-symlink				\
  cp/reflink-auto				\
  cp/reflink-perm				\
  cp/same-file					\
  cp/slink-2-slink				\
  cp/sparse					\
  cp/sparse-to-pipe				\
  cp/special-f					\
  cp/src-base-dot				\
  cp/symlink-slash				\
  cp/thru-dangling				\
  df/header					\
  df/df-P					\
  df/unreadable					\
  df/total-unprocessed				\
  df/no-mtab-status				\
  dd/direct					\
  dd/misc					\
  dd/nocache					\
  dd/not-rewound				\
  dd/reblock					\
  dd/skip-seek					\
  dd/skip-seek2					\
  dd/bytes					\
  dd/skip-seek-past-file			\
  dd/sparse					\
  dd/stderr					\
  dd/unblock					\
  dd/unblock-sync				\
  df/total-verify				\
  du/2g						\
  du/8gb					\
  du/basic					\
  du/bigtime					\
  du/deref					\
  du/deref-args					\
  du/exclude					\
  du/fd-leak					\
  du/files0-from				\
  du/files0-from-dir				\
  du/hard-link					\
  du/inacc-dest					\
  du/inacc-dir					\
  du/inaccessible-cwd				\
  du/long-from-unreadable			\
  du/long-sloop					\
  du/max-depth					\
  du/move-dir-while-traversing			\
  du/no-deref					\
  du/no-x					\
  du/one-file-system				\
  du/restore-wd					\
  du/slash					\
  du/slink					\
  du/trailing-slash				\
  du/two-args					\
  id/gnu-zero-uids				\
  id/no-context					\
  install/basic-1				\
  install/create-leading			\
  install/d-slashdot				\
  install/install-C				\
  install/install-C-selinux			\
  install/strip-program				\
  install/trap					\
  ln/backup-1					\
  ln/hard-backup				\
  ln/hard-to-sym				\
  ln/misc					\
  ln/relative					\
  ln/sf-1					\
  ln/slash-decorated-nonexistent-dest		\
  ln/target-1					\
  ls/abmon-align				\
  ls/block-size					\
  ls/color-clear-to-eol				\
  ls/color-dtype-dir				\
  ls/color-norm					\
  ls/dangle					\
  ls/dired					\
  ls/file-type					\
  ls/follow-slink				\
  ls/getxattr-speedup				\
  ls/infloop					\
  ls/inode					\
  ls/m-option					\
  ls/multihardlink				\
  ls/no-arg					\
  ls/no-cap					\
  ls/proc-selinux-segfault			\
  ls/readdir-mountpoint-inode			\
  ls/recursive					\
  ls/root-rel-symlink-color			\
  ls/rt-1					\
  ls/slink-acl					\
  ls/stat-dtype					\
  ls/stat-failed				\
  ls/stat-free-color				\
  ls/stat-free-symlinks				\
  ls/stat-vs-dirent				\
  ls/symlink-slash				\
  ls/time-style-diag				\
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
  mv/mv-n					\
  mv/mv-special-1				\
  mv/no-target-dir				\
  mv/part-fail					\
  mv/part-hardlink				\
  mv/part-rename				\
  mv/part-symlink				\
  mv/partition-perm				\
  mv/perm-1					\
  mv/symlink-onto-hardlink			\
  mv/symlink-onto-hardlink-to-self		\
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
  tail-2/flush-initial				\
  tail-2/follow-name				\
  tail-2/follow-stdin				\
  tail-2/pipe-f					\
  tail-2/pipe-f2				\
  tail-2/proc-ksyms				\
  tail-2/start-middle				\
  touch/60-seconds				\
  touch/dangling-symlink			\
  touch/dir-1					\
  touch/fail-diag				\
  touch/fifo					\
  touch/no-create-missing			\
  touch/no-dereference				\
  touch/no-rights				\
  touch/not-owner				\
  touch/obsolescent				\
  touch/read-only				\
  touch/relative				\
  touch/trailing-slash				\
  $(root_tests)

pr_data =					\
  pr/0F						\
  pr/0FF					\
  pr/0FFnt					\
  pr/0FFt					\
  pr/0FnFnt					\
  pr/0FnFt					\
  pr/0Fnt					\
  pr/0Ft					\
  pr/2-S_f-t_notab				\
  pr/2-Sf-t_notab				\
  pr/2f-t_notab					\
  pr/2s_f-t_notab				\
  pr/2s_w60f-t_nota				\
  pr/2sf-t_notab				\
  pr/2sw60f-t_notab				\
  pr/2w60f-t_notab				\
  pr/3-0F					\
  pr/3-5l24f-t					\
  pr/3-FF					\
  pr/3a2l17-FF					\
  pr/3a3f-0F					\
  pr/3a3l15-t					\
  pr/3a3l15f-t					\
  pr/3b2l17-FF					\
  pr/3b3f-0F					\
  pr/3b3f-0FF					\
  pr/3b3f-FF					\
  pr/3b3l15-t					\
  pr/3b3l15f-t					\
  pr/3f-0F					\
  pr/3f-FF					\
  pr/3l24-t					\
  pr/3l24f-t					\
  pr/3ml24-FF					\
  pr/3ml24-t					\
  pr/3ml24-t-FF					\
  pr/3ml24f-t					\
  pr/4-7l24-FF					\
  pr/4l24-FF					\
  pr/FF						\
  pr/FFn					\
  pr/FFtn					\
  pr/FnFn					\
  pr/Ja3l24f-lm					\
  pr/Jb3l24f-lm					\
  pr/Jml24f-lm-lo				\
  pr/W-72l24f-ll				\
  pr/W20l24f-ll					\
  pr/W26l24f-ll					\
  pr/W27l24f-ll					\
  pr/W28l24f-ll					\
  pr/W35Ja3l24f-lm				\
  pr/W35Jb3l24f-lm				\
  pr/W35Jml24f-lmlo				\
  pr/W35a3l24f-lm				\
  pr/W35b3l24f-lm				\
  pr/W35ml24f-lm-lo				\
  pr/W72Jl24f-ll				\
  pr/a2l15-FF					\
  pr/a2l17-FF					\
  pr/a3-0F					\
  pr/a3f-0F					\
  pr/a3f-0FF					\
  pr/a3f-FF					\
  pr/a3l15-t					\
  pr/a3l15f-t					\
  pr/a3l24f-lm					\
  pr/b2l15-FF					\
  pr/b2l17-FF					\
  pr/b3-0F					\
  pr/b3f-0F					\
  pr/b3f-0FF					\
  pr/b3f-FF					\
  pr/b3l15-t					\
  pr/b3l15f-t					\
  pr/b3l24f-lm					\
  pr/l24-FF					\
  pr/l24-t					\
  pr/l24f-t					\
  pr/loli					\
  pr/ml20-FF-t					\
  pr/ml24-FF					\
  pr/ml24-t					\
  pr/ml24-t-FF					\
  pr/ml24f-0F					\
  pr/ml24f-lm-lo				\
  pr/ml24f-t					\
  pr/ml24f-t-0F					\
  pr/n+2-5l24f-0FF				\
  pr/n+2l24f-0FF				\
  pr/n+2l24f-bl					\
  pr/n+3-7l24-FF				\
  pr/n+3l24f-0FF				\
  pr/n+3l24f-bl					\
  pr/n+3ml20f-bl-FF				\
  pr/n+3ml24f-bl-tn				\
  pr/n+3ml24f-tn-bl				\
  pr/n+4-8a2l17-FF				\
  pr/n+4b2l17f-0FF				\
  pr/n+5-8b3l17f-FF				\
  pr/n+5a3l13f-0FF				\
  pr/n+6a2l17-FF				\
  pr/n+6b3l13f-FF				\
  pr/n+7l24-FF					\
  pr/n+8l20-FF					\
  pr/nJml24f-lmlmlo				\
  pr/nJml24f-lmlolm				\
  pr/nN1+3l24f-bl				\
  pr/nN15l24f-bl				\
  pr/nSml20-bl-FF				\
  pr/nSml20-t-t-FF				\
  pr/nSml20-t-tFFFF				\
  pr/nSml24-bl-FF				\
  pr/nSml24-t-t-FF				\
  pr/nSml24-t-tFFFF				\
  pr/nl24f-bl					\
  pr/o3Jml24f-lm-lo				\
  pr/o3a3Sl24f-tn				\
  pr/o3a3Snl24f-tn				\
  pr/o3a3l24f-tn				\
  pr/o3b3Sl24f-tn				\
  pr/o3b3Snl24f-tn				\
  pr/o3b3l24f-tn				\
  pr/o3mSl24f-bl-tn				\
  pr/o3mSnl24fbltn				\
  pr/o3ml24f-bl-tn				\
  pr/t-0FF					\
  pr/t-FF					\
  pr/t-bl					\
  pr/t-t					\
  pr/tFFn					\
  pr/tFFt					\
  pr/tFFt-bl					\
  pr/tFFt-ll					\
  pr/tFFt-lm					\
  pr/tFnFt					\
  pr/t_notab					\
  pr/t_tab					\
  pr/t_tab_					\
  pr/ta3-0FF					\
  pr/ta3-FF					\
  pr/tb3-0FF					\
  pr/tb3-FF					\
  pr/tn						\
  pr/tn2e5o3-t_tab				\
  pr/tn2e8-t_tab				\
  pr/tn2e8o3-t_tab				\
  pr/tn_2e8-t_tab				\
  pr/tn_2e8S-t_tab				\
  pr/tne8-t_tab					\
  pr/tne8o3-t_tab				\
  pr/tt-0FF					\
  pr/tt-FF					\
  pr/tt-bl					\
  pr/tt-t					\
  pr/tta3-0FF					\
  pr/tta3-FF					\
  pr/ttb3-0FF					\
  pr/ttb3-FF					\
  pr/w72l24f-ll
