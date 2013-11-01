# Make coreutils man pages.				-*-Makefile-*-
# This is included by the top-level Makefile.am.

# Copyright (C) 2002-2013 Free Software Foundation, Inc.

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

EXTRA_DIST += man/help2man man/dummy-man

## Graceful degradation for systems lacking perl.
if HAVE_PERL
run_help2man = $(PERL) -- $(srcdir)/man/help2man
else
run_help2man = $(SHELL) $(srcdir)/man/dummy-man
endif

man1_MANS = @man1_MANS@
EXTRA_DIST += $(man1_MANS:.1=.x)

EXTRA_MANS = @EXTRA_MANS@
EXTRA_DIST += $(EXTRA_MANS:.1=.x)

ALL_MANS = $(man1_MANS) $(EXTRA_MANS)

CLEANFILES += $(ALL_MANS)

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

# Most prog.1 man pages depend on src/prog.  List the exceptions:
# Note that dir and vdir are exceptions only if you consider the name
# of the .c file upon which they depend: ls.c.
man/arch.1:      src/uname
man/dir.1:       src/dir
man/install.1:   src/ginstall
man/vdir.1:      src/vdir

man/base64.1:    src/base64
man/basename.1:  src/basename
man/cat.1:       src/cat
man/chcon.1:     src/chcon
man/chgrp.1:     src/chgrp
man/chmod.1:     src/chmod
man/chown.1:     src/chown
man/chroot.1:    src/chroot
man/cksum.1:     src/cksum
man/comm.1:      src/comm
man/cp.1:        src/cp
man/csplit.1:    src/csplit
man/cut.1:       src/cut
man/date.1:      src/date
man/dd.1:        src/dd
man/df.1:        src/df
man/dircolors.1: src/dircolors
man/dirname.1:   src/dirname
man/du.1:        src/du
man/echo.1:      src/echo
man/env.1:       src/env
man/expand.1:    src/expand
man/expr.1:      src/expr
man/factor.1:    src/factor
man/false.1:     src/false
man/fmt.1:       src/fmt
man/fold.1:      src/fold
man/groups.1:    src/groups
man/head.1:      src/head
man/hostid.1:    src/hostid
man/hostname.1:  src/hostname
man/id.1:        src/id
man/join.1:      src/join
man/kill.1:      src/kill
man/link.1:      src/link
man/ln.1:        src/ln
man/logname.1:   src/logname
man/ls.1:        src/ls
man/md5sum.1:    src/md5sum
man/mkdir.1:     src/mkdir
man/mkfifo.1:    src/mkfifo
man/mknod.1:     src/mknod
man/mktemp.1:    src/mktemp
man/mv.1:        src/mv
man/nice.1:      src/nice
man/nl.1:        src/nl
man/nohup.1:     src/nohup
man/nproc.1:     src/nproc
man/numfmt.1:    src/numfmt
man/od.1:        src/od
man/paste.1:     src/paste
man/pathchk.1:   src/pathchk
man/pinky.1:     src/pinky
man/pr.1:        src/pr
man/printenv.1:  src/printenv
man/printf.1:    src/printf
man/ptx.1:       src/ptx
man/pwd.1:       src/pwd
man/readlink.1:  src/readlink
man/realpath.1:  src/realpath
man/rm.1:        src/rm
man/rmdir.1:     src/rmdir
man/runcon.1:    src/runcon
man/seq.1:       src/seq
man/sha1sum.1:   src/sha1sum
man/sha224sum.1: src/sha224sum
man/sha256sum.1: src/sha256sum
man/sha384sum.1: src/sha384sum
man/sha512sum.1: src/sha512sum
man/shred.1:     src/shred
man/shuf.1:      src/shuf
man/sleep.1:     src/sleep
man/sort.1:      src/sort
man/split.1:     src/split
man/stat.1:      src/stat
man/stdbuf.1:    src/stdbuf
man/stty.1:      src/stty
man/sum.1:       src/sum
man/sync.1:      src/sync
man/tac.1:       src/tac
man/tail.1:      src/tail
man/tee.1:       src/tee
man/test.1:      src/test
man/timeout.1:   src/timeout
man/touch.1:     src/touch
man/tr.1:        src/tr
man/true.1:      src/true
man/truncate.1:  src/truncate
man/tsort.1:     src/tsort
man/tty.1:       src/tty
man/uname.1:     src/uname
man/unexpand.1:  src/unexpand
man/uniq.1:      src/uniq
man/unlink.1:    src/unlink
man/uptime.1:    src/uptime
man/users.1:     src/users
man/wc.1:        src/wc
man/who.1:       src/who
man/whoami.1:    src/whoami
man/yes.1:       src/yes

.x.1:
	$(AM_V_GEN)name=`echo $@ | sed 's|.*/||; s|\.1$$||'` || exit 1;	\
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
	  && (cd $$t && $(LN_S) '$(abs_top_builddir)/src/'$$prog $$name) \
	  && $(run_help2man)						\
		     --source='$(PACKAGE_STRING)'			\
		     --include=$(srcdir)/man/$$name.x			\
		     --output=$$t/$$name.1 $$t/$$name			\
		     --info-page='coreutils \(aq'$$name' invocation\(aq' \
	  && sed \
	       -e 's|$*\.td/||g' \
	       -e '/For complete documentation/d' \
	       $$t/$$name.1 > $@-t			\
	  && rm -rf $$t							\
	  && chmod a-w $@-t						\
	  && mv $@-t $@
