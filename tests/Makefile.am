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

# Indirections required so that we'll still be able to know the
# complete list of our tests even if the user override TESTS from the
# command line (which he's allowed to do by the test harness API).
TESTS = $(all_tests)
root_tests = $(all_root_tests)

# Ensure that all version-controlled executable files are listed
# in $(all_tests).
vc_exe_in_TESTS:
	$(AM_V_GEN)test -d $(top_srcdir)/.git || exit 1;		\
	 {								\
	  for t in $(all_tests); do echo $$t; done; 			\
	  cd $(top_srcdir);						\
	  for f in `$(SHELL) build-aux/vc-list-files $(subdir)		\
	              | sed 's!^$(subdir)/!!'`; do			\
	    test -f "tests/$$f" && test -x "tests/$$f" && echo "$$f";	\
	  done;								\
	 } | sort | uniq -u | grep . && exit 1; :

check-local: vc_exe_in_TESTS
.PHONY: vc_exe_in_TESTS

CLEANFILES =
CLEANFILES += .built-programs
check_DATA = .built-programs
.built-programs: $(top_srcdir)/Makefile.am
	$(AM_V_GEN)(cd $(top_builddir)					\
            && MAKEFLAGS= $(MAKE) -s built_programs.list)		\
          | sed s,^src/,, > $@-t && mv $@-t $@

TEST_EXTENSIONS = .sh .pl .xpl

if HAVE_PERL
TESTSUITE_PERL = $(PERL)
else
TESTSUITE_PERL = $(SHELL) $(srcdir)/no-perl
endif

# Options passed to the perl invocations running the perl test scripts.
TESTSUITE_PERL_OPTIONS = -w -I$(srcdir) -MCoreutils -MCuSkip
# '$f' is set by the Automake-generated test harness to the path of the
# current test script stripped of VPATH components, and is used by the
# CuTmpdir module to determine the name of the temporary files to be
# used.  Note that $f is a shell variable, not a make macro, so the use
# of '$$f' below is correct, and not a typo.
TESTSUITE_PERL_OPTIONS += -M"CuTmpdir qw($$f)"

SH_LOG_COMPILER = $(SHELL)
PL_LOG_COMPILER = $(TESTSUITE_PERL) $(TESTSUITE_PERL_OPTIONS)
# Perl scripts that must be run in tainted mode.
XPL_LOG_COMPILER = $(TESTSUITE_PERL) -T $(TESTSUITE_PERL_OPTIONS)

# Note that the first lines are statements.  They ensure that environment
# variables that can perturb tests are unset or set to expected values.
# The rest are envvar settings that propagate build-related Makefile
# variables to test scripts.
AM_TESTS_ENVIRONMENT =				\
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
  d_type-check		\
  envvar-check		\
  filefrag-extent-compare \
  fiemap-capable	\
  init.cfg		\
  init.sh		\
  lang-default		\
  no-perl		\
  other-fs-tmpdir	\
  sample-test		\
  $(pr_data)

all_root_tests =				\
  chown/basic.sh				\
  cp/cp-a-selinux.sh				\
  cp/preserve-gid.sh				\
  cp/special-bits.sh				\
  cp/cp-mv-enotsup-xattr.sh			\
  cp/capability.sh				\
  cp/sparse-fiemap.sh				\
  dd/skip-seek-past-dev.sh			\
  df/problematic-chars.sh			\
  du/bind-mount-dir-cycle.sh			\
  install/install-C-root.sh			\
  ls/capability.sh				\
  ls/nameless-uid.sh				\
  misc/chcon.sh					\
  misc/chroot-credentials.sh			\
  misc/id-setgid.sh				\
  misc/selinux.sh				\
  misc/truncate-owned-by-other.sh		\
  mkdir/writable-under-readonly.sh		\
  mv/sticky-to-xpart.sh				\
  rm/fail-2eperm.sh				\
  rm/no-give-up.sh				\
  rm/one-file-system.sh				\
  rm/read-only.sh				\
  tail-2/append-only.sh				\
  touch/now-owned-by-other.sh

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

EXTRA_DIST += $(all_tests)

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

all_tests =					\
  misc/help-version.sh				\
  tail-2/inotify-race.sh			\
  misc/invalid-opt.pl				\
  rm/ext3-perf.sh				\
  rm/cycle.sh					\
  cp/link-heap.sh				\
  misc/tty-eof.pl				\
  tail-2/inotify-hash-abuse.sh			\
  tail-2/inotify-hash-abuse2.sh			\
  tail-2/F-vs-missing.sh			\
  tail-2/F-vs-rename.sh				\
  tail-2/inotify-rotate.sh			\
  chmod/no-x.sh					\
  chgrp/basic.sh				\
  rm/dangling-symlink.sh			\
  misc/ls-time.sh				\
  rm/d-1.sh					\
  rm/d-2.sh					\
  rm/d-3.sh					\
  rm/deep-1.sh					\
  rm/deep-2.sh					\
  rm/dir-no-w.sh				\
  rm/dir-nonrecur.sh				\
  rm/dot-rel.sh					\
  rm/isatty.sh					\
  rm/empty-inacc.sh				\
  rm/empty-name.pl				\
  rm/f-1.sh					\
  rm/fail-eacces.sh				\
  rm/fail-eperm.xpl				\
  tail-2/assert.sh				\
  rm/hash.sh					\
  rm/i-1.sh					\
  rm/i-never.sh					\
  rm/i-no-r.sh					\
  tail-2/infloop-1.sh				\
  rm/ignorable.sh				\
  rm/inaccessible.sh				\
  rm/interactive-always.sh			\
  rm/interactive-once.sh			\
  rm/ir-1.sh					\
  rm/one-file-system2.sh			\
  rm/r-1.sh					\
  rm/r-2.sh					\
  rm/r-3.sh					\
  rm/r-4.sh					\
  rm/readdir-bug.sh				\
  rm/rm1.sh					\
  touch/empty-file.sh				\
  rm/rm2.sh					\
  rm/rm3.sh					\
  rm/rm4.sh					\
  rm/rm5.sh					\
  rm/sunos-1.sh					\
  rm/unread2.sh					\
  rm/unread3.sh					\
  rm/unreadable.pl				\
  rm/v-slash.sh					\
  rm/many-dir-entries-vs-OOM.sh			\
  chgrp/default-no-deref.sh			\
  chgrp/deref.sh				\
  chgrp/no-x.sh					\
  chgrp/posix-H.sh				\
  chgrp/recurse.sh				\
  fmt/base.pl					\
  fmt/long-line.sh				\
  fmt/goal-option.sh				\
  misc/env.sh					\
  misc/ptx.pl					\
  misc/test.pl					\
  misc/seq.pl					\
  misc/seq-long-double.sh			\
  misc/head.pl					\
  misc/head-elide-tail.pl			\
  tail-2/tail-n0f.sh				\
  misc/ls-misc.pl				\
  misc/date.pl					\
  misc/date-next-dow.pl				\
  misc/ptx-overrun.sh				\
  misc/xstrtol.pl				\
  tail-2/pid.sh					\
  misc/od.pl					\
  misc/od-float.sh				\
  misc/mktemp.pl				\
  misc/arch.sh					\
  misc/pr.pl					\
  misc/join.pl					\
  pr/pr-tests.pl				\
  misc/pwd-option.sh				\
  misc/chcon-fail.sh				\
  misc/cut.pl					\
  misc/wc.pl					\
  misc/wc-files0-from.pl			\
  misc/wc-files0.sh				\
  misc/wc-parallel.sh				\
  misc/cat-proc.sh				\
  misc/cat-buf.sh				\
  misc/base64.pl				\
  misc/basename.pl				\
  misc/close-stdout.sh				\
  misc/chroot-fail.sh				\
  misc/comm.pl					\
  misc/csplit.sh				\
  misc/csplit-1000.sh				\
  misc/csplit-heap.sh				\
  misc/date-sec.sh				\
  misc/dircolors.pl				\
  misc/dirname.pl				\
  misc/env-null.sh				\
  misc/expand.pl				\
  misc/expr.pl					\
  misc/factor.pl				\
  misc/false-status.sh				\
  misc/fold.pl					\
  misc/groups-dash.sh				\
  misc/groups-version.sh			\
  misc/head-c.sh				\
  misc/head-pos.sh				\
  misc/id-context.sh				\
  misc/id-groups.sh				\
  misc/id-setgid.sh				\
  misc/md5sum.pl				\
  misc/md5sum-bsd.sh				\
  misc/md5sum-newline.pl			\
  misc/md5sum-parallel.sh			\
  misc/mknod.sh					\
  misc/nice.sh					\
  misc/nice-fail.sh				\
  misc/nl.sh					\
  misc/nohup.sh					\
  misc/nproc-avail.sh				\
  misc/nproc-positive.sh			\
  misc/od-N.sh					\
  misc/od-multiple-t.sh				\
  misc/od-x8.sh					\
  misc/paste.pl					\
  misc/pathchk1.sh				\
  misc/printenv.sh				\
  misc/printf.sh				\
  misc/printf-cov.pl				\
  misc/printf-hex.sh				\
  misc/printf-surprise.sh			\
  misc/pwd-long.sh				\
  misc/readlink-fp-loop.sh			\
  misc/readlink-root.sh				\
  misc/realpath.sh				\
  misc/runcon-no-reorder.sh			\
  misc/sha1sum.pl				\
  misc/sha1sum-vec.pl				\
  misc/sha224sum.pl				\
  misc/sha256sum.pl				\
  misc/sha384sum.pl				\
  misc/sha512sum.pl				\
  misc/shred-exact.sh				\
  misc/shred-passes.sh				\
  misc/shred-remove.sh				\
  misc/shuf.sh					\
  misc/sort.pl					\
  misc/sort-benchmark-random.sh			\
  misc/sort-compress.sh				\
  misc/sort-compress-hang.sh			\
  misc/sort-compress-proc.sh			\
  misc/sort-continue.sh				\
  misc/sort-debug-keys.sh			\
  misc/sort-debug-warn.sh			\
  misc/sort-discrim.sh				\
  misc/sort-files0-from.pl			\
  misc/sort-float.sh				\
  misc/sort-merge.pl				\
  misc/sort-merge-fdlimit.sh			\
  misc/sort-month.sh				\
  misc/sort-exit-early.sh			\
  misc/sort-rand.sh				\
  misc/sort-spinlock-abuse.sh			\
  misc/sort-stale-thread-mem.sh			\
  misc/sort-unique.sh				\
  misc/sort-unique-segv.sh			\
  misc/sort-version.sh				\
  misc/sort-NaN-infloop.sh			\
  misc/sort-u-FMR.sh				\
  split/filter.sh				\
  split/suffix-auto-length.sh			\
  split/suffix-length.sh			\
  split/additional-suffix.sh			\
  split/b-chunk.sh				\
  split/fail.sh					\
  split/lines.sh				\
  split/l-chunk.sh				\
  split/r-chunk.sh				\
  split/numeric.sh				\
  split/guard-input.sh				\
  misc/stat-birthtime.sh			\
  misc/stat-fmt.sh				\
  misc/stat-hyphen.sh				\
  misc/stat-mount.sh				\
  misc/stat-nanoseconds.sh			\
  misc/stat-printf.pl				\
  misc/stat-slash.sh				\
  misc/stdbuf.sh				\
  misc/stty.sh					\
  misc/stty-invalid.sh				\
  misc/stty-pairs.sh				\
  misc/stty-row-col.sh				\
  misc/sum.pl					\
  misc/sum-sysv.sh				\
  misc/tac.pl					\
  misc/tac-continue.sh				\
  misc/tac-2-nonseekable.sh			\
  misc/tail.pl					\
  misc/tee.sh					\
  misc/tee-dash.sh				\
  misc/test-diag.pl				\
  misc/timeout.sh				\
  misc/timeout-group.sh				\
  misc/timeout-parameters.sh			\
  misc/tr.pl					\
  misc/tr-case-class.sh				\
  misc/truncate-dangling-symlink.sh		\
  misc/truncate-dir-fail.sh			\
  misc/truncate-fail-diag.sh			\
  misc/truncate-fifo.sh				\
  misc/truncate-no-create-missing.sh		\
  misc/truncate-overflow.sh			\
  misc/truncate-parameters.sh			\
  misc/truncate-relative.sh			\
  misc/tsort.pl					\
  misc/unexpand.pl				\
  misc/uniq.pl					\
  misc/uniq-perf.sh				\
  misc/xattr.sh					\
  tail-2/wait.sh				\
  chmod/c-option.sh				\
  chmod/equal-x.sh				\
  chmod/equals.sh				\
  chmod/inaccessible.sh				\
  chmod/octal.sh				\
  chmod/setgid.sh				\
  chmod/silent.sh				\
  chmod/thru-dangling.sh			\
  chmod/umask-x.sh				\
  chmod/usage.sh				\
  chown/deref.sh				\
  chown/preserve-root.sh			\
  chown/separator.sh				\
  cp/abuse.sh					\
  cp/acl.sh					\
  cp/attr-existing.sh				\
  cp/backup-1.sh				\
  cp/backup-dir.sh				\
  cp/backup-is-src.sh				\
  cp/cp-HL.sh					\
  cp/cp-deref.sh				\
  cp/cp-i.sh					\
  cp/cp-mv-backup.sh				\
  cp/cp-parents.sh				\
  cp/deref-slink.sh				\
  cp/dir-rm-dest.sh				\
  cp/dir-slash.sh				\
  cp/dir-vs-file.sh				\
  cp/existing-perm-dir.sh			\
  cp/existing-perm-race.sh			\
  cp/fail-perm.sh				\
  cp/fiemap-empty.sh				\
  cp/fiemap-perf.sh				\
  cp/fiemap-2.sh				\
  cp/file-perm-race.sh				\
  cp/into-self.sh				\
  cp/link.sh					\
  cp/link-no-deref.sh				\
  cp/link-preserve.sh				\
  cp/link-symlink.sh				\
  cp/nfs-removal-race.sh			\
  cp/no-deref-link1.sh				\
  cp/no-deref-link2.sh				\
  cp/no-deref-link3.sh				\
  cp/parent-perm.sh				\
  cp/parent-perm-race.sh			\
  cp/perm.sh					\
  cp/preserve-2.sh				\
  cp/preserve-link.sh				\
  cp/preserve-slink-time.sh			\
  cp/proc-short-read.sh				\
  cp/proc-zero-len.sh				\
  cp/r-vs-symlink.sh				\
  cp/reflink-auto.sh				\
  cp/reflink-perm.sh				\
  cp/same-file.sh				\
  cp/slink-2-slink.sh				\
  cp/sparse.sh					\
  cp/sparse-to-pipe.sh				\
  cp/special-f.sh				\
  cp/src-base-dot.sh				\
  cp/symlink-slash.sh				\
  cp/thru-dangling.sh				\
  df/header.sh					\
  df/df-P.sh					\
  df/unreadable.sh				\
  df/total-unprocessed.sh			\
  df/no-mtab-status.sh				\
  dd/direct.sh					\
  dd/misc.sh					\
  dd/nocache.sh					\
  dd/not-rewound.sh				\
  dd/reblock.sh					\
  dd/skip-seek.pl				\
  dd/skip-seek2.sh				\
  dd/bytes.sh					\
  dd/skip-seek-past-file.sh			\
  dd/sparse.sh					\
  dd/stderr.sh					\
  dd/unblock.pl					\
  dd/unblock-sync.sh				\
  df/total-verify.sh				\
  du/2g.sh					\
  du/8gb.sh					\
  du/basic.sh					\
  du/bigtime.sh					\
  du/deref.sh					\
  du/deref-args.sh				\
  du/exclude.sh					\
  du/fd-leak.sh					\
  du/files0-from.pl				\
  du/files0-from-dir.sh				\
  du/hard-link.sh				\
  du/inacc-dest.sh				\
  du/inacc-dir.sh				\
  du/inaccessible-cwd.sh			\
  du/long-from-unreadable.sh			\
  du/long-sloop.sh				\
  du/max-depth.sh				\
  du/move-dir-while-traversing.sh		\
  du/no-deref.sh				\
  du/no-x.sh					\
  du/one-file-system.sh				\
  du/restore-wd.sh				\
  du/slash.sh					\
  du/slink.sh					\
  du/trailing-slash.sh				\
  du/two-args.sh				\
  id/gnu-zero-uids.sh				\
  id/no-context.sh				\
  install/basic-1.sh				\
  install/create-leading.sh			\
  install/d-slashdot.sh				\
  install/install-C.sh				\
  install/install-C-selinux.sh			\
  install/strip-program.sh			\
  install/trap.sh				\
  ln/backup-1.sh				\
  ln/hard-backup.sh				\
  ln/hard-to-sym.sh				\
  ln/misc.sh					\
  ln/relative.sh				\
  ln/sf-1.sh					\
  ln/slash-decorated-nonexistent-dest.sh	\
  ln/target-1.sh				\
  ls/abmon-align.sh				\
  ls/block-size.sh				\
  ls/color-clear-to-eol.sh			\
  ls/color-dtype-dir.sh				\
  ls/color-norm.sh				\
  ls/dangle.sh					\
  ls/dired.sh					\
  ls/file-type.sh				\
  ls/follow-slink.sh				\
  ls/getxattr-speedup.sh			\
  ls/infloop.sh					\
  ls/inode.sh					\
  ls/m-option.sh				\
  ls/multihardlink.sh				\
  ls/no-arg.sh					\
  ls/no-cap.sh					\
  ls/proc-selinux-segfault.sh			\
  ls/readdir-mountpoint-inode.sh		\
  ls/recursive.sh				\
  ls/root-rel-symlink-color.sh			\
  ls/rt-1.sh					\
  ls/slink-acl.sh				\
  ls/stat-dtype.sh				\
  ls/stat-failed.sh				\
  ls/stat-free-color.sh				\
  ls/stat-free-symlinks.sh			\
  ls/stat-vs-dirent.sh				\
  ls/symlink-slash.sh				\
  ls/time-style-diag.sh				\
  ls/x-option.sh				\
  mkdir/p-1.sh					\
  mkdir/p-2.sh					\
  mkdir/p-3.sh					\
  mkdir/p-slashdot.sh				\
  mkdir/p-thru-slink.sh				\
  mkdir/p-v.sh					\
  mkdir/parents.sh				\
  mkdir/perm.sh					\
  mkdir/selinux.sh				\
  mkdir/special-1.sh				\
  mkdir/t-slash.sh				\
  mv/acl.sh					\
  mv/atomic.sh					\
  mv/atomic2.sh					\
  mv/backup-dir.sh				\
  mv/backup-is-src.sh				\
  mv/childproof.sh				\
  mv/diag.sh					\
  mv/dir-file.sh				\
  mv/dir2dir.sh					\
  mv/dup-source.sh				\
  mv/force.sh					\
  mv/hard-2.sh					\
  mv/hard-3.sh					\
  mv/hard-4.sh					\
  mv/hard-link-1.sh				\
  mv/hard-verbose.sh				\
  mv/i-1.pl					\
  mv/i-2.sh					\
  mv/i-3.sh					\
  mv/i-4.sh					\
  mv/i-5.sh					\
  mv/i-link-no.sh				\
  mv/into-self.sh				\
  mv/into-self-2.sh				\
  mv/into-self-3.sh				\
  mv/into-self-4.sh				\
  mv/leak-fd.sh					\
  mv/mv-n.sh					\
  mv/mv-special-1.sh				\
  mv/no-target-dir.sh				\
  mv/part-fail.sh				\
  mv/part-hardlink.sh				\
  mv/part-rename.sh				\
  mv/part-symlink.sh				\
  mv/partition-perm.sh				\
  mv/perm-1.sh					\
  mv/symlink-onto-hardlink.sh			\
  mv/symlink-onto-hardlink-to-self.sh		\
  mv/to-symlink.sh				\
  mv/trailing-slash.sh				\
  mv/update.sh					\
  readlink/can-e.sh				\
  readlink/can-f.sh				\
  readlink/can-m.sh				\
  readlink/rl-1.sh				\
  rmdir/fail-perm.sh				\
  rmdir/ignore.sh				\
  rmdir/t-slash.sh				\
  tail-2/assert-2.sh				\
  tail-2/big-4gb.sh				\
  tail-2/flush-initial.sh			\
  tail-2/follow-name.sh				\
  tail-2/follow-stdin.sh			\
  tail-2/pipe-f.sh				\
  tail-2/pipe-f2.sh				\
  tail-2/proc-ksyms.sh				\
  tail-2/start-middle.sh			\
  touch/60-seconds.sh				\
  touch/dangling-symlink.sh			\
  touch/dir-1.sh				\
  touch/fail-diag.sh				\
  touch/fifo.sh					\
  touch/no-create-missing.sh			\
  touch/no-dereference.sh			\
  touch/no-rights.sh				\
  touch/not-owner.sh				\
  touch/obsolescent.sh				\
  touch/read-only.sh				\
  touch/relative.sh				\
  touch/trailing-slash.sh			\
  $(all_root_tests)

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
