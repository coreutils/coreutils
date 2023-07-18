## Process this file with automake to produce Makefile.in -*-Makefile-*-.

## Copyright (C) 2007-2023 Free Software Foundation, Inc.

## This program is free software: you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation, either version 3 of the License, or
## (at your option) any later version.

## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.

## You should have received a copy of the GNU General Public License
## along with this program.  If not, see <https://www.gnu.org/licenses/>.

# Indirections required so that we'll still be able to know the
# complete list of our tests even if the user overrides TESTS
# from the command line (as permitted by the test harness API).
TESTS = $(all_tests) $(factor_tests)
root_tests = $(all_root_tests)

EXTRA_DIST += $(all_tests)

TEST_EXTENSIONS = .sh .pl .xpl

if HAVE_PERL
TESTSUITE_PERL = $(PERL)
else
TESTSUITE_PERL = $(SHELL) $(srcdir)/tests/no-perl
endif

# Options passed to the perl invocations running the perl test scripts.
TESTSUITE_PERL_OPTIONS = -w -I$(srcdir)/tests -MCuSkip -MCoreutils
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

# We don't want this to go in the top-level directory.
TEST_SUITE_LOG = tests/test-suite.log

# Note that the first lines are statements.  They ensure that environment
# variables that can perturb tests are unset or set to expected values.
# The rest are envvar settings that propagate build-related Makefile
# variables to test scripts.
TESTS_ENVIRONMENT =				\
  . $(srcdir)/tests/lang-default;		\
  tmp__=$${TMPDIR-/tmp};			\
  test -d "$$tmp__" && test -w "$$tmp__" || tmp__=.;	\
  . $(srcdir)/tests/envvar-check;		\
  TMPDIR=$$tmp__; export TMPDIR;		\
  export					\
  VERSION='$(VERSION)'				\
  LOCALE_FR='$(LOCALE_FR)'			\
  LOCALE_FR_UTF8='$(LOCALE_FR_UTF8)'		\
  abs_top_builddir='$(abs_top_builddir)'	\
  abs_top_srcdir='$(abs_top_srcdir)'		\
  abs_srcdir='$(abs_srcdir)'			\
  built_programs='$(built_programs) $(single_binary_progs)' \
  fail=0					\
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
  PACKAGE_VERSION=$(PACKAGE_VERSION)		\
  PERL='$(PERL)'				\
  SHELL='$(PREFERABLY_POSIX_SHELL)'		\
  ; test -d /usr/xpg4/bin && PATH='/usr/xpg4/bin$(PATH_SEPARATOR)'"$$PATH"; \
  PATH='$(abs_top_builddir)/src$(PATH_SEPARATOR)'"$$PATH" \
  ; 9>&2

# On failure, display the global testsuite log on stdout.
VERBOSE = yes

EXTRA_DIST +=			\
  init.cfg			\
  tests/Coreutils.pm		\
  tests/CuSkip.pm		\
  tests/CuTmpdir.pm		\
  tests/d_type-check		\
  tests/envvar-check		\
  tests/factor/run.sh		\
  tests/factor/create-test.sh	\
  tests/filefrag-extent-compare \
  tests/seek-data-capable	\
  tests/init.sh			\
  tests/lang-default		\
  tests/no-perl			\
  tests/other-fs-tmpdir		\
  tests/sample-test		\
  $(pr_data)

all_root_tests =				\
  tests/chown/basic.sh				\
  tests/cp/cp-a-selinux.sh			\
  tests/cp/preserve-gid.sh			\
  tests/cp/special-bits.sh			\
  tests/cp/cp-mv-enotsup-xattr.sh		\
  tests/cp/capability.sh			\
  tests/cp/sparse-extents-2.sh			\
  tests/cp/cross-dev-symlink.sh			\
  tests/dd/skip-seek-past-dev.sh		\
  tests/df/problematic-chars.sh			\
  tests/df/over-mount-device.sh			\
  tests/du/bind-mount-dir-cycle.sh		\
  tests/du/bind-mount-dir-cycle-v2.sh		\
  tests/id/setgid.sh				\
  tests/install/install-C-root.sh		\
  tests/ls/capability.sh			\
  tests/ls/nameless-uid.sh			\
  tests/chcon/chcon.sh				\
  tests/chroot/chroot-credentials.sh		\
  tests/misc/selinux.sh				\
  tests/truncate/truncate-owned-by-other.sh	\
  tests/mkdir/writable-under-readonly.sh	\
  tests/mkdir/smack-root.sh			\
  tests/mv/hardlink-case.sh			\
  tests/mv/sticky-to-xpart.sh			\
  tests/rm/fail-2eperm.sh			\
  tests/rm/no-give-up.sh			\
  tests/rm/one-file-system.sh			\
  tests/rm/read-only.sh				\
  tests/rm/empty-immutable-skip.sh		\
  tests/split/l-chunk-root.sh			\
  tests/tail/append-only.sh			\
  tests/tail/end-of-device.sh			\
  tests/touch/now-owned-by-other.sh

ALL_RECURSIVE_TARGETS += check-root
.PHONY: check-root
check-root:
	$(MAKE) check TESTS='$(root_tests)' SUBDIRS=.

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
  tests/help/help-version.sh			\
  tests/help/help-version-getopt.sh		\
  tests/tail/inotify-race.sh			\
  tests/tail/inotify-race2.sh			\
  tests/misc/invalid-opt.pl			\
  tests/rm/ext3-perf.sh				\
  tests/rm/cycle.sh				\
  tests/cp/link-heap.sh				\
  tests/cp/no-ctx.sh				\
  tests/tty/tty-eof.pl				\
  tests/misc/read-errors.sh			\
  tests/misc/write-errors.sh			\
  tests/tail/inotify-hash-abuse.sh		\
  tests/tail/inotify-hash-abuse2.sh		\
  tests/tail/F-vs-missing.sh			\
  tests/tail/F-vs-rename.sh			\
  tests/tail/F-headers.sh			\
  tests/tail/descriptor-vs-rename.sh		\
  tests/tail/inotify-rotate.sh			\
  tests/tail/inotify-rotate-resources.sh	\
  tests/tail/inotify-dir-recreate.sh		\
  tests/tail/inotify-only-regular.sh		\
  tests/chmod/no-x.sh				\
  tests/chgrp/basic.sh				\
  tests/rm/dangling-symlink.sh			\
  tests/ls/ls-time.sh				\
  tests/rm/d-1.sh				\
  tests/rm/d-2.sh				\
  tests/rm/d-3.sh				\
  tests/rm/deep-1.sh				\
  tests/rm/deep-2.sh				\
  tests/rm/dir-no-w.sh				\
  tests/rm/dir-nonrecur.sh			\
  tests/rm/dot-rel.sh				\
  tests/rm/isatty.sh				\
  tests/rm/empty-inacc.sh			\
  tests/rm/empty-name.pl			\
  tests/rm/f-1.sh				\
  tests/rm/fail-eacces.sh			\
  tests/rm/fail-eperm.xpl			\
  tests/tail/assert.sh				\
  tests/rm/hash.sh				\
  tests/rm/i-1.sh				\
  tests/rm/i-never.sh				\
  tests/rm/i-no-r.sh				\
  tests/rm/ignorable.sh				\
  tests/rm/inaccessible.sh			\
  tests/rm/interactive-always.sh		\
  tests/rm/interactive-once.sh			\
  tests/rm/ir-1.sh				\
  tests/rm/one-file-system2.sh			\
  tests/rm/r-1.sh				\
  tests/rm/r-2.sh				\
  tests/rm/r-3.sh				\
  tests/rm/r-4.sh				\
  tests/rm/r-root.sh				\
  tests/rm/readdir-bug.sh			\
  tests/rm/rm1.sh				\
  tests/touch/empty-file.sh			\
  tests/rm/rm2.sh				\
  tests/rm/rm3.sh				\
  tests/rm/rm4.sh				\
  tests/rm/rm5.sh				\
  tests/rm/sunos-1.sh				\
  tests/rm/unread2.sh				\
  tests/rm/unread3.sh				\
  tests/rm/unreadable.pl			\
  tests/rm/v-slash.sh				\
  tests/rm/many-dir-entries-vs-OOM.sh		\
  tests/rm/rm-readdir-fail.sh			\
  tests/chgrp/default-no-deref.sh		\
  tests/chgrp/deref.sh				\
  tests/chgrp/no-x.sh				\
  tests/chgrp/posix-H.sh			\
  tests/chgrp/recurse.sh			\
  tests/fmt/base.pl				\
  tests/fmt/goal-option.sh			\
  tests/fmt/long-line.sh			\
  tests/fmt/non-space.sh			\
  tests/misc/echo.sh				\
  tests/env/env.sh				\
  tests/env/env-signal-handler.sh		\
  tests/ptx/ptx.pl				\
  tests/test/test.pl				\
  tests/seq/seq.pl				\
  tests/seq/seq-epipe.sh			\
  tests/seq/seq-extra-number.sh			\
  tests/seq/seq-io-errors.sh			\
  tests/seq/seq-locale.sh			\
  tests/seq/seq-long-double.sh			\
  tests/seq/seq-precision.sh			\
  tests/head/head.pl				\
  tests/head/head-elide-tail.pl			\
  tests/tail/tail-n0f.sh			\
  tests/ls/ls-misc.pl				\
  tests/date/date.pl				\
  tests/date/date-next-dow.pl			\
  tests/ptx/ptx-overrun.sh			\
  tests/misc/xstrtol.pl				\
  tests/tail/overlay-headers.sh			\
  tests/tail/pid.sh				\
  tests/od/od.pl				\
  tests/od/od-endian.sh				\
  tests/od/od-float.sh				\
  tests/misc/mktemp.pl				\
  tests/misc/arch.sh				\
  tests/misc/join.pl				\
  tests/pr/pr-tests.pl				\
  tests/pwd/pwd-option.sh			\
  tests/chcon/chcon-fail.sh			\
  tests/misc/coreutils.sh			\
  tests/cut/cut.pl				\
  tests/cut/cut-huge-range.sh			\
  tests/wc/wc.pl				\
  tests/wc/wc-files0-from.pl			\
  tests/wc/wc-files0.sh				\
  tests/wc/wc-nbsp.sh				\
  tests/wc/wc-parallel.sh			\
  tests/wc/wc-proc.sh				\
  tests/wc/wc-total.sh				\
  tests/cat/cat-E.sh				\
  tests/cat/cat-proc.sh				\
  tests/cat/cat-buf.sh				\
  tests/cat/cat-self.sh				\
  tests/misc/base64.pl				\
  tests/misc/basename.pl			\
  tests/misc/basenc.pl				\
  tests/misc/close-stdout.sh			\
  tests/chroot/chroot-fail.sh			\
  tests/cksum/cksum.sh				\
  tests/cksum/cksum-a.sh			\
  tests/cksum/cksum-c.sh			\
  tests/cksum/cksum-base64.pl			\
  tests/cksum/cksum-raw.sh			\
  tests/misc/comm.pl				\
  tests/csplit/csplit.sh			\
  tests/csplit/csplit-1000.sh			\
  tests/csplit/csplit-heap.sh			\
  tests/csplit/csplit-io-err.sh			\
  tests/csplit/csplit-suppress-matched.pl	\
  tests/date/date-debug.sh			\
  tests/date/date-sec.sh			\
  tests/date/date-tz.sh				\
  tests/misc/dircolors.pl			\
  tests/misc/dirname.pl				\
  tests/env/env-null.sh				\
  tests/env/env-S.pl				\
  tests/env/env-S-script.sh			\
  tests/misc/expand.pl				\
  tests/expr/expr.pl				\
  tests/expr/expr-multibyte.pl			\
  tests/factor/factor.pl			\
  tests/factor/factor-parallel.sh		\
  tests/misc/false-status.sh			\
  tests/misc/fold.pl				\
  tests/groups/groups-dash.sh			\
  tests/groups/groups-process-all.sh		\
  tests/groups/groups-version.sh		\
  tests/head/head-c.sh				\
  tests/head/head-pos.sh			\
  tests/head/head-write-error.sh		\
  tests/misc/kill.sh				\
  tests/cksum/b2sum.sh				\
  tests/cksum/md5sum.pl				\
  tests/cksum/md5sum-bsd.sh			\
  tests/cksum/md5sum-newline.pl			\
  tests/cksum/md5sum-parallel.sh		\
  tests/misc/mknod.sh				\
  tests/nice/nice.sh				\
  tests/nice/nice-fail.sh			\
  tests/misc/nl.sh				\
  tests/misc/nohup.sh				\
  tests/nproc/nproc-avail.sh			\
  tests/nproc/nproc-positive.sh			\
  tests/nproc/nproc-override.sh			\
  tests/misc/numfmt.pl				\
  tests/od/od-N.sh				\
  tests/od/od-j.sh				\
  tests/od/od-multiple-t.sh			\
  tests/od/od-x8.sh				\
  tests/misc/paste.pl				\
  tests/misc/pathchk.sh				\
  tests/misc/printenv.sh			\
  tests/printf/printf.sh			\
  tests/printf/printf-cov.pl			\
  tests/printf/printf-hex.sh			\
  tests/printf/printf-mb.sh			\
  tests/printf/printf-surprise.sh		\
  tests/printf/printf-quote.sh			\
  tests/pwd/pwd-long.sh				\
  tests/readlink/readlink-fp-loop.sh		\
  tests/readlink/readlink-root.sh		\
  tests/misc/realpath.sh			\
  tests/runcon/runcon-compute.sh		\
  tests/runcon/runcon-no-reorder.sh		\
  tests/cksum/sha1sum.pl			\
  tests/cksum/sha1sum-vec.pl			\
  tests/cksum/sha224sum.pl			\
  tests/cksum/sha256sum.pl			\
  tests/cksum/sha384sum.pl			\
  tests/cksum/sha512sum.pl			\
  tests/shred/shred-exact.sh			\
  tests/shred/shred-passes.sh			\
  tests/shred/shred-remove.sh			\
  tests/shred/shred-size.sh			\
  tests/shuf/shuf.sh				\
  tests/shuf/shuf-reservoir.sh			\
  tests/misc/sleep.sh				\
  tests/cksum/sm3sum.pl				\
  tests/sort/sort.pl				\
  tests/sort/sort-benchmark-random.sh		\
  tests/sort/sort-compress.sh			\
  tests/sort/sort-compress-hang.sh		\
  tests/sort/sort-compress-proc.sh		\
  tests/sort/sort-continue.sh			\
  tests/sort/sort-debug-keys.sh			\
  tests/sort/sort-debug-warn.sh			\
  tests/sort/sort-discrim.sh			\
  tests/sort/sort-files0-from.pl		\
  tests/sort/sort-float.sh			\
  tests/sort/sort-h-thousands-sep.sh		\
  tests/sort/sort-merge.pl			\
  tests/sort/sort-merge-fdlimit.sh		\
  tests/sort/sort-month.sh			\
  tests/sort/sort-exit-early.sh			\
  tests/sort/sort-rand.sh			\
  tests/sort/sort-spinlock-abuse.sh		\
  tests/sort/sort-stale-thread-mem.sh		\
  tests/sort/sort-unique.sh			\
  tests/sort/sort-unique-segv.sh		\
  tests/sort/sort-version.sh			\
  tests/sort/sort-NaN-infloop.sh		\
  tests/sort/sort-u-FMR.sh			\
  tests/split/filter.sh				\
  tests/split/suffix-auto-length.sh		\
  tests/split/suffix-length.sh			\
  tests/split/additional-suffix.sh		\
  tests/split/b-chunk.sh			\
  tests/split/fail.sh				\
  tests/split/lines.sh				\
  tests/split/line-bytes.sh			\
  tests/split/l-chunk.sh			\
  tests/split/r-chunk.sh			\
  tests/split/record-sep.sh			\
  tests/split/numeric.sh			\
  tests/split/guard-input.sh			\
  tests/stat/stat-birthtime.sh			\
  tests/stat/stat-fmt.sh			\
  tests/stat/stat-hyphen.sh			\
  tests/stat/stat-mount.sh			\
  tests/stat/stat-nanoseconds.sh		\
  tests/stat/stat-printf.pl			\
  tests/stat/stat-slash.sh			\
  tests/misc/stdbuf.sh				\
  tests/stty/stty.sh				\
  tests/stty/stty-invalid.sh			\
  tests/stty/stty-pairs.sh			\
  tests/stty/stty-row-col.sh			\
  tests/cksum/sum.pl				\
  tests/cksum/sum-sysv.sh			\
  tests/misc/sync.sh				\
  tests/tac/tac.pl				\
  tests/tac/tac-continue.sh			\
  tests/tac/tac-2-nonseekable.sh		\
  tests/tail/tail.pl				\
  tests/misc/tee.sh				\
  tests/test/test-N.sh				\
  tests/test/test-diag.pl			\
  tests/misc/time-style.sh			\
  tests/timeout/timeout.sh			\
  tests/timeout/timeout-blocked.pl		\
  tests/timeout/timeout-group.sh		\
  tests/timeout/timeout-large-parameters.sh	\
  tests/timeout/timeout-parameters.sh		\
  tests/tr/tr.pl				\
  tests/tr/tr-case-class.sh			\
  tests/truncate/truncate-dangling-symlink.sh	\
  tests/truncate/truncate-dir-fail.sh		\
  tests/truncate/truncate-fail-diag.sh		\
  tests/truncate/truncate-fifo.sh		\
  tests/truncate/truncate-no-create-missing.sh	\
  tests/truncate/truncate-overflow.sh		\
  tests/truncate/truncate-parameters.sh		\
  tests/truncate/truncate-relative.sh		\
  tests/misc/tsort.pl				\
  tests/tty/tty.sh				\
  tests/misc/usage_vs_getopt.sh			\
  tests/misc/unexpand.pl			\
  tests/uniq/uniq.pl				\
  tests/uniq/uniq-perf.sh			\
  tests/uniq/uniq-collate.sh			\
  tests/misc/xattr.sh				\
  tests/misc/yes.sh				\
  tests/tail/wait.sh				\
  tests/tail/retry.sh				\
  tests/tail/symlink.sh				\
  tests/tail/tail-c.sh				\
  tests/tail/truncate.sh			\
  tests/chmod/c-option.sh			\
  tests/chmod/equal-x.sh			\
  tests/chmod/equals.sh				\
  tests/chmod/ignore-symlink.sh			\
  tests/chmod/inaccessible.sh			\
  tests/chmod/octal.sh				\
  tests/chmod/setgid.sh				\
  tests/chmod/silent.sh				\
  tests/chmod/thru-dangling.sh			\
  tests/chmod/umask-x.sh			\
  tests/chmod/usage.sh				\
  tests/chown/deref.sh				\
  tests/chown/preserve-root.sh			\
  tests/chown/separator.sh			\
  tests/cp/abuse.sh				\
  tests/cp/acl.sh				\
  tests/cp/attr-existing.sh			\
  tests/cp/backup-1.sh				\
  tests/cp/backup-dir.sh			\
  tests/cp/backup-is-src.sh			\
  tests/cp/cp-HL.sh				\
  tests/cp/cp-deref.sh				\
  tests/cp/cp-i.sh				\
  tests/cp/cp-mv-backup.sh			\
  tests/cp/cp-parents.sh			\
  tests/cp/debug.sh				\
  tests/cp/deref-slink.sh			\
  tests/cp/dir-rm-dest.sh			\
  tests/cp/dir-slash.sh				\
  tests/cp/dir-vs-file.sh			\
  tests/cp/existing-perm-dir.sh			\
  tests/cp/existing-perm-race.sh		\
  tests/cp/fail-perm.sh				\
  tests/cp/sparse-extents.sh			\
  tests/cp/copy-FMR.sh				\
  tests/cp/sparse-perf.sh			\
  tests/cp/sparse-2.sh				\
  tests/cp/file-perm-race.sh			\
  tests/cp/into-self.sh				\
  tests/cp/link.sh				\
  tests/cp/link-deref.sh			\
  tests/cp/link-no-deref.sh			\
  tests/cp/link-preserve.sh			\
  tests/cp/link-symlink.sh			\
  tests/cp/nfs-removal-race.sh			\
  tests/cp/no-deref-link1.sh			\
  tests/cp/no-deref-link2.sh			\
  tests/cp/no-deref-link3.sh			\
  tests/cp/parent-perm.sh			\
  tests/cp/parent-perm-race.sh			\
  tests/cp/perm.sh				\
  tests/cp/preserve-2.sh			\
  tests/cp/preserve-link.sh			\
  tests/cp/preserve-mode.sh			\
  tests/cp/preserve-slink-time.sh		\
  tests/cp/proc-short-read.sh			\
  tests/cp/proc-zero-len.sh			\
  tests/cp/r-vs-symlink.sh			\
  tests/cp/reflink-auto.sh			\
  tests/cp/reflink-perm.sh			\
  tests/cp/same-file.sh				\
  tests/cp/slink-2-slink.sh			\
  tests/cp/sparse.sh				\
  tests/cp/sparse-to-pipe.sh			\
  tests/cp/special-f.sh				\
  tests/cp/src-base-dot.sh			\
  tests/cp/symlink-slash.sh			\
  tests/cp/thru-dangling.sh			\
  tests/df/header.sh				\
  tests/df/df-P.sh				\
  tests/df/df-output.sh				\
  tests/df/df-symlink.sh			\
  tests/df/unreadable.sh			\
  tests/df/total-unprocessed.sh			\
  tests/df/no-mtab-status.sh			\
  tests/df/skip-duplicates.sh			\
  tests/df/skip-rootfs.sh			\
  tests/dd/ascii.sh				\
  tests/dd/direct.sh				\
  tests/dd/misc.sh				\
  tests/dd/no-allocate.sh			\
  tests/dd/nocache.sh				\
  tests/dd/nocache_eof.sh			\
  tests/dd/not-rewound.sh			\
  tests/dd/reblock.sh				\
  tests/dd/skip-seek.pl				\
  tests/dd/skip-seek2.sh			\
  tests/dd/bytes.sh				\
  tests/dd/skip-seek-past-file.sh		\
  tests/dd/sparse.sh				\
  tests/dd/stderr.sh				\
  tests/dd/unblock.pl				\
  tests/dd/unblock-sync.sh			\
  tests/dd/stats.sh				\
  tests/df/total-verify.sh			\
  tests/du/2g.sh				\
  tests/du/8gb.sh				\
  tests/du/apparent.sh				\
  tests/du/basic.sh				\
  tests/du/bigtime.sh				\
  tests/du/deref.sh				\
  tests/du/deref-args.sh			\
  tests/du/exclude.sh				\
  tests/du/fd-leak.sh				\
  tests/du/files0-from.pl			\
  tests/du/files0-from-dir.sh			\
  tests/du/hard-link.sh				\
  tests/du/inacc-dest.sh			\
  tests/du/inacc-dir.sh				\
  tests/du/inaccessible-cwd.sh			\
  tests/du/inodes.sh				\
  tests/du/long-from-unreadable.sh		\
  tests/du/long-sloop.sh			\
  tests/du/max-depth.sh				\
  tests/du/move-dir-while-traversing.sh		\
  tests/du/no-deref.sh				\
  tests/du/no-x.sh				\
  tests/du/one-file-system.sh			\
  tests/du/restore-wd.sh			\
  tests/du/slash.sh				\
  tests/du/threshold.sh				\
  tests/du/trailing-slash.sh			\
  tests/du/two-args.sh				\
  tests/id/gnu-zero-uids.sh			\
  tests/id/no-context.sh			\
  tests/id/context.sh				\
  tests/id/uid.sh				\
  tests/id/zero.sh				\
  tests/id/smack.sh				\
  tests/install/basic-1.sh			\
  tests/install/create-leading.sh		\
  tests/install/d-slashdot.sh			\
  tests/install/install-C.sh			\
  tests/install/install-C-selinux.sh		\
  tests/install/install-Z-selinux.sh		\
  tests/install/strip-program.sh		\
  tests/install/trap.sh				\
  tests/ln/backup-1.sh				\
  tests/ln/hard-backup.sh			\
  tests/ln/hard-to-sym.sh			\
  tests/ln/misc.sh				\
  tests/ln/relative.sh				\
  tests/ln/sf-1.sh				\
  tests/ln/slash-decorated-nonexistent-dest.sh	\
  tests/ln/target-1.sh				\
  tests/ls/a-option.sh				\
  tests/ls/abmon-align.sh			\
  tests/ls/birthtime.sh				\
  tests/ls/block-size.sh			\
  tests/ls/classify.sh				\
  tests/ls/color-clear-to-eol.sh		\
  tests/ls/color-dtype-dir.sh			\
  tests/ls/color-norm.sh			\
  tests/ls/color-term.sh			\
  tests/ls/color-ext.sh				\
  tests/ls/dangle.sh				\
  tests/ls/dired.sh				\
  tests/ls/file-type.sh				\
  tests/ls/follow-slink.sh			\
  tests/ls/getxattr-speedup.sh			\
  tests/ls/group-dirs.sh			\
  tests/ls/hex-option.sh			\
  tests/ls/hyperlink.sh				\
  tests/ls/infloop.sh				\
  tests/ls/inode.sh				\
  tests/ls/m-option.sh				\
  tests/ls/w-option.sh				\
  tests/ls/multihardlink.sh			\
  tests/ls/no-arg.sh				\
  tests/ls/no-cap.sh				\
  tests/ls/selinux-segfault.sh			\
  tests/ls/quote-align.sh			\
  tests/ls/readdir-mountpoint-inode.sh		\
  tests/ls/recursive.sh				\
  tests/ls/removed-directory.sh			\
  tests/ls/root-rel-symlink-color.sh		\
  tests/ls/rt-1.sh				\
  tests/ls/slink-acl.sh				\
  tests/ls/stat-dtype.sh			\
  tests/ls/stat-failed.sh			\
  tests/ls/stat-free-color.sh			\
  tests/ls/stat-free-symlinks.sh		\
  tests/ls/stat-vs-dirent.sh			\
  tests/ls/symlink-loop.sh			\
  tests/ls/symlink-quote.sh			\
  tests/ls/symlink-slash.sh			\
  tests/ls/time-style-diag.sh			\
  tests/ls/sort-width-option.sh			\
  tests/ls/x-option.sh				\
  tests/ls/zero-option.sh			\
  tests/mkdir/p-1.sh				\
  tests/mkdir/p-2.sh				\
  tests/mkdir/p-3.sh				\
  tests/mkdir/p-acl.sh				\
  tests/mkdir/p-slashdot.sh			\
  tests/mkdir/p-thru-slink.sh			\
  tests/mkdir/p-v.sh				\
  tests/mkdir/parents.sh			\
  tests/mkdir/perm.sh				\
  tests/mkdir/selinux.sh			\
  tests/mkdir/restorecon.sh			\
  tests/mkdir/special-1.sh			\
  tests/mkdir/t-slash.sh			\
  tests/mkdir/smack-no-root.sh			\
  tests/mv/acl.sh				\
  tests/mv/atomic.sh				\
  tests/mv/atomic2.sh				\
  tests/mv/backup-dir.sh			\
  tests/mv/backup-is-src.sh			\
  tests/mv/childproof.sh			\
  tests/mv/diag.sh				\
  tests/mv/dir-file.sh				\
  tests/mv/dir2dir.sh				\
  tests/mv/dup-source.sh			\
  tests/mv/force.sh				\
  tests/mv/hard-2.sh				\
  tests/mv/hard-3.sh				\
  tests/mv/hard-4.sh				\
  tests/mv/hard-link-1.sh			\
  tests/mv/i-1.pl				\
  tests/mv/i-2.sh				\
  tests/mv/i-3.sh				\
  tests/mv/i-4.sh				\
  tests/mv/i-5.sh				\
  tests/mv/i-link-no.sh				\
  tests/mv/into-self.sh				\
  tests/mv/into-self-2.sh			\
  tests/mv/into-self-3.sh			\
  tests/mv/into-self-4.sh			\
  tests/mv/leak-fd.sh				\
  tests/mv/mv-n.sh				\
  tests/mv/mv-special-1.sh			\
  tests/mv/no-copy.sh				\
  tests/mv/no-target-dir.sh			\
  tests/mv/part-fail.sh				\
  tests/mv/part-hardlink.sh			\
  tests/mv/part-rename.sh			\
  tests/mv/part-symlink.sh			\
  tests/mv/partition-perm.sh			\
  tests/mv/perm-1.sh				\
  tests/mv/symlink-onto-hardlink.sh		\
  tests/mv/symlink-onto-hardlink-to-self.sh	\
  tests/mv/to-symlink.sh			\
  tests/mv/trailing-slash.sh			\
  tests/mv/update.sh				\
  tests/readlink/can-e.sh			\
  tests/readlink/can-f.sh			\
  tests/readlink/can-m.sh			\
  tests/readlink/multi.sh			\
  tests/readlink/rl-1.sh			\
  tests/rmdir/fail-perm.sh			\
  tests/rmdir/ignore.sh				\
  tests/rmdir/symlink-errors.sh			\
  tests/rmdir/t-slash.sh			\
  tests/tail/assert-2.sh			\
  tests/tail/big-4gb.sh				\
  tests/tail/flush-initial.sh			\
  tests/tail/follow-name.sh			\
  tests/tail/follow-stdin.sh			\
  tests/tail/pipe-f.sh				\
  tests/tail/pipe-f2.sh				\
  tests/tail/proc-ksyms.sh			\
  tests/tail/start-middle.sh			\
  tests/touch/60-seconds.sh			\
  tests/touch/dangling-symlink.sh		\
  tests/touch/dir-1.sh				\
  tests/touch/fail-diag.sh			\
  tests/touch/fifo.sh				\
  tests/touch/no-create-missing.sh		\
  tests/touch/no-dereference.sh			\
  tests/touch/no-rights.sh			\
  tests/touch/not-owner.sh			\
  tests/touch/obsolescent.sh			\
  tests/touch/read-only.sh			\
  tests/touch/relative.sh			\
  tests/touch/trailing-slash.sh			\
  $(all_root_tests)

# See tests/factor/create-test.sh.
tf = tests/factor
factor_tests = \
  $(tf)/t00.sh $(tf)/t01.sh $(tf)/t02.sh $(tf)/t03.sh $(tf)/t04.sh \
  $(tf)/t05.sh $(tf)/t06.sh $(tf)/t07.sh $(tf)/t08.sh $(tf)/t09.sh \
  $(tf)/t10.sh $(tf)/t11.sh $(tf)/t12.sh $(tf)/t13.sh $(tf)/t14.sh \
  $(tf)/t15.sh $(tf)/t16.sh $(tf)/t17.sh $(tf)/t18.sh $(tf)/t19.sh \
  $(tf)/t20.sh $(tf)/t21.sh $(tf)/t22.sh $(tf)/t23.sh $(tf)/t24.sh \
  $(tf)/t25.sh $(tf)/t26.sh $(tf)/t27.sh $(tf)/t28.sh $(tf)/t29.sh \
  $(tf)/t30.sh $(tf)/t31.sh $(tf)/t32.sh $(tf)/t33.sh $(tf)/t34.sh \
  $(tf)/t35.sh $(tf)/t36.sh

$(factor_tests): $(tf)/run.sh $(tf)/create-test.sh
	$(AM_V_GEN)$(MKDIR_P) $(tf)
	$(AM_V_at)$(SHELL) $(srcdir)/$(tf)/create-test.sh $@ \
	  $(srcdir)/$(tf)/run.sh > $@-t
	$(AM_V_at)chmod a+x $@-t
	$(AM_V_at)mv -f $@-t $@

CLEANFILES += $(factor_tests)

pr_data =					\
  tests/pr/0F					\
  tests/pr/0FF					\
  tests/pr/0FFnt				\
  tests/pr/0FFt					\
  tests/pr/0FnFnt				\
  tests/pr/0FnFt				\
  tests/pr/0Fnt					\
  tests/pr/0Ft					\
  tests/pr/2-S_f-t_notab			\
  tests/pr/2-Sf-t_notab				\
  tests/pr/2f-t_notab				\
  tests/pr/2s_f-t_notab				\
  tests/pr/2s_w60f-t_nota			\
  tests/pr/2sf-t_notab				\
  tests/pr/2sw60f-t_notab			\
  tests/pr/2w60f-t_notab			\
  tests/pr/3-0F					\
  tests/pr/3-5l24f-t				\
  tests/pr/3-FF					\
  tests/pr/3a2l17-FF				\
  tests/pr/3a3f-0F				\
  tests/pr/3a3l15-t				\
  tests/pr/3a3l15f-t				\
  tests/pr/3b2l17-FF				\
  tests/pr/3b3f-0F				\
  tests/pr/3b3f-0FF				\
  tests/pr/3b3f-FF				\
  tests/pr/3b3l15-t				\
  tests/pr/3b3l15f-t				\
  tests/pr/3f-0F				\
  tests/pr/3f-FF				\
  tests/pr/3l24-t				\
  tests/pr/3l24f-t				\
  tests/pr/3ml24-FF				\
  tests/pr/3ml24-t				\
  tests/pr/3ml24-t-FF				\
  tests/pr/3ml24f-t				\
  tests/pr/4-7l24-FF				\
  tests/pr/4l24-FF				\
  tests/pr/FF					\
  tests/pr/FFn					\
  tests/pr/FFtn					\
  tests/pr/FnFn					\
  tests/pr/Ja3l24f-lm				\
  tests/pr/Jb3l24f-lm				\
  tests/pr/Jml24f-lm-lo				\
  tests/pr/W-72l24f-ll				\
  tests/pr/W20l24f-ll				\
  tests/pr/W26l24f-ll				\
  tests/pr/W27l24f-ll				\
  tests/pr/W28l24f-ll				\
  tests/pr/W35Ja3l24f-lm			\
  tests/pr/W35Jb3l24f-lm			\
  tests/pr/W35Jml24f-lmlo			\
  tests/pr/W35a3l24f-lm				\
  tests/pr/W35b3l24f-lm				\
  tests/pr/W35ml24f-lm-lo			\
  tests/pr/W72Jl24f-ll				\
  tests/pr/a2l15-FF				\
  tests/pr/a2l17-FF				\
  tests/pr/a3-0F				\
  tests/pr/a3f-0F				\
  tests/pr/a3f-0FF				\
  tests/pr/a3f-FF				\
  tests/pr/a3l15-t				\
  tests/pr/a3l15f-t				\
  tests/pr/a3l24f-lm				\
  tests/pr/b2l15-FF				\
  tests/pr/b2l17-FF				\
  tests/pr/b3-0F				\
  tests/pr/b3f-0F				\
  tests/pr/b3f-0FF				\
  tests/pr/b3f-FF				\
  tests/pr/b3l15-t				\
  tests/pr/b3l15f-t				\
  tests/pr/b3l24f-lm				\
  tests/pr/l24-FF				\
  tests/pr/l24-t				\
  tests/pr/l24f-t				\
  tests/pr/loli					\
  tests/pr/ml20-FF-t				\
  tests/pr/ml24-FF				\
  tests/pr/ml24-t				\
  tests/pr/ml24-t-FF				\
  tests/pr/ml24f-0F				\
  tests/pr/ml24f-lm-lo				\
  tests/pr/ml24f-t				\
  tests/pr/ml24f-t-0F				\
  tests/pr/n+2-5l24f-0FF			\
  tests/pr/n+2l24f-0FF				\
  tests/pr/n+2l24f-bl				\
  tests/pr/n+3-7l24-FF				\
  tests/pr/n+3l24f-0FF				\
  tests/pr/n+3l24f-bl				\
  tests/pr/n+3ml20f-bl-FF			\
  tests/pr/n+3ml24f-bl-tn			\
  tests/pr/n+3ml24f-tn-bl			\
  tests/pr/n+4-8a2l17-FF			\
  tests/pr/n+4b2l17f-0FF			\
  tests/pr/n+5-8b3l17f-FF			\
  tests/pr/n+5a3l13f-0FF			\
  tests/pr/n+6a2l17-FF				\
  tests/pr/n+6b3l13f-FF				\
  tests/pr/n+7l24-FF				\
  tests/pr/n+8l20-FF				\
  tests/pr/nJml24f-lmlmlo			\
  tests/pr/nJml24f-lmlolm			\
  tests/pr/nN1+3l24f-bl				\
  tests/pr/nN15l24f-bl				\
  tests/pr/nSml20-bl-FF				\
  tests/pr/nSml20-t-t-FF			\
  tests/pr/nSml20-t-tFFFF			\
  tests/pr/nSml24-bl-FF				\
  tests/pr/nSml24-t-t-FF			\
  tests/pr/nSml24-t-tFFFF			\
  tests/pr/nl24f-bl				\
  tests/pr/o3Jml24f-lm-lo			\
  tests/pr/o3a3Sl24f-tn				\
  tests/pr/o3a3Snl24f-tn			\
  tests/pr/o3a3l24f-tn				\
  tests/pr/o3b3Sl24f-tn				\
  tests/pr/o3b3Snl24f-tn			\
  tests/pr/o3b3l24f-tn				\
  tests/pr/o3mSl24f-bl-tn			\
  tests/pr/o3mSnl24fbltn			\
  tests/pr/o3ml24f-bl-tn			\
  tests/pr/t-0FF				\
  tests/pr/t-FF					\
  tests/pr/t-bl					\
  tests/pr/t-t					\
  tests/pr/tFFn					\
  tests/pr/tFFt					\
  tests/pr/tFFt-bl				\
  tests/pr/tFFt-ll				\
  tests/pr/tFFt-lm				\
  tests/pr/tFnFt				\
  tests/pr/t_notab				\
  tests/pr/t_tab				\
  tests/pr/t_tab_				\
  tests/pr/ta3-0FF				\
  tests/pr/ta3-FF				\
  tests/pr/tb3-0FF				\
  tests/pr/tb3-FF				\
  tests/pr/tn					\
  tests/pr/tn2e5o3-t_tab			\
  tests/pr/tn2e8-t_tab				\
  tests/pr/tn2e8o3-t_tab			\
  tests/pr/tn_2e8-t_tab				\
  tests/pr/tn_2e8S-t_tab			\
  tests/pr/tne8-t_tab				\
  tests/pr/tne8o3-t_tab				\
  tests/pr/tt-0FF				\
  tests/pr/tt-FF				\
  tests/pr/tt-bl				\
  tests/pr/tt-t					\
  tests/pr/tta3-0FF				\
  tests/pr/tta3-FF				\
  tests/pr/ttb3-0FF				\
  tests/pr/ttb3-FF				\
  tests/pr/w72l24f-ll

$(TEST_LOGS): $(PROGRAMS)
