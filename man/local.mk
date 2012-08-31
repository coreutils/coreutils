# Make coreutils man pages.				-*-Makefile-*-
# This is included by the top-level Makefile.am.

# Copyright (C) 2002-2012 Free Software Foundation, Inc.

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

EXTRA_DIST += man/help2man

man1_MANS = @man1_MANS@
EXTRA_DIST += $(man1_MANS) $(man1_MANS:.1=.x)

EXTRA_MANS = @EXTRA_MANS@
EXTRA_DIST += $(EXTRA_MANS) $(EXTRA_MANS:.1=.x)

ALL_MANS = $(man1_MANS) $(EXTRA_MANS)

MAINTAINERCLEANFILES += $(ALL_MANS)

# This is required because we have subtle inter-directory dependencies:
# in order to generate all man pages, even those for which we don't
# install a binary, require that all programs be built at distribution
# time.  We can't use 'dist-hook' for this, since it would run too late:
# the manpages must be generated before the distdir is created and filled.
$(EXTRA_MANS): $(all_programs)

# This is a kludge to remove generated 'man/*.1' from a non-srcdir build.
# Without this, "make distcheck" might fail.
distclean-local:
	test x$(srcdir) = x$(builddir) || rm -f $(ALL_MANS)

# Dependencies common to all man pages.  Updated below.
mandeps =

# Depend on this to get version number changes.
mandeps += .version

# This is required so that changes to e.g., emit_bug_reporting_address
# provoke regeneration of all the manpages.
mandeps += $(top_srcdir)/src/system.h

$(ALL_MANS): $(mandeps)

# Note that arch depends on uname.c
man/arch.1:      src/uname.c

man/base64.1:    src/base64.c
man/basename.1:  src/basename.c
man/cat.1:       src/cat.c
man/chcon.1:     src/chcon.c
man/chgrp.1:     src/chgrp.c
man/chmod.1:     src/chmod.c
man/chown.1:     src/chown.c
man/chroot.1:    src/chroot.c
man/cksum.1:     src/cksum.c
man/comm.1:      src/comm.c
man/cp.1:        src/cp.c
man/csplit.1:    src/csplit.c
man/cut.1:       src/cut.c
man/date.1:      src/date.c
man/dd.1:        src/dd.c
man/df.1:        src/df.c

# Note that dir depends on ls.c, since that's where its --help text is.
man/dir.1:       src/ls.c

man/dircolors.1: src/dircolors.c
man/dirname.1:   src/dirname.c
man/du.1:        src/du.c
man/echo.1:      src/echo.c
man/env.1:       src/env.c
man/expand.1:    src/expand.c
man/expr.1:      src/expr.c
man/factor.1:    src/factor.c
man/false.1:     src/false.c
man/fmt.1:       src/fmt.c
man/fold.1:      src/fold.c
man/groups.1:    src/groups.c
man/head.1:      src/head.c
man/hostid.1:    src/hostid.c
man/hostname.1:  src/hostname.c
man/id.1:        src/id.c
man/install.1:   src/install.c
man/join.1:      src/join.c
man/kill.1:      src/kill.c
man/link.1:      src/link.c
man/ln.1:        src/ln.c
man/logname.1:   src/logname.c
man/ls.1:        src/ls.c
man/md5sum.1:    src/md5sum.c
man/mkdir.1:     src/mkdir.c
man/mkfifo.1:    src/mkfifo.c
man/mknod.1:     src/mknod.c
man/mktemp.1:    src/mktemp.c
man/mv.1:        src/mv.c
man/nice.1:      src/nice.c
man/nl.1:        src/nl.c
man/nohup.1:     src/nohup.c
man/nproc.1:     src/nproc.c
man/od.1:        src/od.c
man/paste.1:     src/paste.c
man/pathchk.1:   src/pathchk.c
man/pinky.1:     src/pinky.c
man/pr.1:        src/pr.c
man/printenv.1:  src/printenv.c
man/printf.1:    src/printf.c
man/ptx.1:       src/ptx.c
man/pwd.1:       src/pwd.c
man/readlink.1:  src/readlink.c
man/realpath.1:  src/realpath.c
man/rm.1:        src/rm.c
man/rmdir.1:     src/rmdir.c
man/runcon.1:    src/runcon.c
man/seq.1:       src/seq.c
man/sha1sum.1:   src/md5sum.c
man/sha224sum.1: src/md5sum.c
man/sha256sum.1: src/md5sum.c
man/sha384sum.1: src/md5sum.c
man/sha512sum.1: src/md5sum.c
man/shred.1:     src/shred.c
man/shuf.1:      src/shuf.c
man/sleep.1:     src/sleep.c
man/sort.1:      src/sort.c
man/split.1:     src/split.c
man/stat.1:      src/stat.c
man/stdbuf.1:    src/stdbuf.c
man/stty.1:      src/stty.c
man/sum.1:       src/sum.c
man/sync.1:      src/sync.c
man/tac.1:       src/tac.c
man/tail.1:      src/tail.c
man/tee.1:       src/tee.c
man/test.1:      src/test.c
man/timeout.1:   src/timeout.c
man/touch.1:     src/touch.c
man/tr.1:        src/tr.c
man/true.1:      src/true.c
man/truncate.1:  src/truncate.c
man/tsort.1:     src/tsort.c
man/tty.1:       src/tty.c
man/uname.1:     src/uname.c
man/unexpand.1:  src/unexpand.c
man/uniq.1:      src/uniq.c
man/unlink.1:    src/unlink.c
man/uptime.1:    src/uptime.c
man/users.1:     src/users.c
man/vdir.1:      src/ls.c
man/wc.1:        src/wc.c
man/who.1:       src/who.c
man/whoami.1:    src/whoami.c
man/yes.1:       src/yes.c

.x.1:
	$(AM_V_GEN)case '$(PERL)' in				\
	  *"/missing "*)					\
	    echo 'WARNING: cannot update man page $@ since perl is missing' \
	      'or inadequate' 1>&2				\
	    exit 0;;						\
	esac; \
	name=`echo $@ | sed -e 's|.*/||' -e 's|\.1$$||'` || exit 1;	\
## Ensure that help2man runs the 'src/ginstall' binary as 'install' when
## creating 'install.1'.  Similarly, ensure that it uses the 'src/[' binary
## to create 'test.1'.
	case $$name in							\
	  install) prog='ginstall';;					\
	     test) prog='[';;						\
		*) prog=$$name;;					\
	esac;								\
## Note the use of $$t/$*, rather than just '$*' as in other packages.
## That is necessary to avoid failures for programs that are also shell
## built-in functions like echo, false, printf, pwd.
	rm -f $@ $@-t							\
	  && t=$*.td							\
	  && rm -rf $$t							\
	  && $(MKDIR_P) $$t						\
	  && (cd $$t && $(LN_S) $(abs_top_builddir)/src/$$prog $$name)	\
	  && $(PERL) -- $(srcdir)/man/help2man				\
		     --source='$(PACKAGE_STRING)'			\
		     --include=$(srcdir)/man/$$name.x			\
		     --output=$$t/$$name.1 $$t/$$name			\
	  && sed 's|$*\.td/||g' $$t/$$name.1 > $@-t			\
	  && rm -rf $$t							\
	  && chmod -w $@-t						\
	  && mv $@-t $@
