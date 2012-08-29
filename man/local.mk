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

dist_man1_MANS = $(MAN)

man_aux = $(dist_man1_MANS:.1=.x)

EXTRA_DIST += $(man_aux) man/help2man
MAINTAINERCLEANFILES += $(dist_man1_MANS)

# The "$(VAR:%=dir/%.x)" idiom is not portable according to POSIX, but in
# practice it works with several make implementation (GNU, BSD, CCS make
# from Solaris 10, Sun distributed make).  In addition, since only
# maintainers are expected to build release tarballs (and they should
# use GNU make when doing so), it's not big deal if this won't work with
# some dumber make implementation.
EXTRA_DIST += \
  $(NO_INSTALL_PROGS_DEFAULT:%=man/%.x) \
  $(NO_INSTALL_PROGS_DEFAULT:%=man/%.1)

# Depend on this to get version number changes.
mandep = .version

# Note that arch depends on uname.c
man/arch.1:      $(mandep)  src/uname.c

man/base64.1:    $(mandep)  src/base64.c
man/basename.1:  $(mandep)  src/basename.c
man/cat.1:       $(mandep)  src/cat.c
man/chcon.1:     $(mandep)  src/chcon.c
man/chgrp.1:     $(mandep)  src/chgrp.c
man/chmod.1:     $(mandep)  src/chmod.c
man/chown.1:     $(mandep)  src/chown.c
man/chroot.1:    $(mandep)  src/chroot.c
man/cksum.1:     $(mandep)  src/cksum.c
man/comm.1:      $(mandep)  src/comm.c
man/cp.1:        $(mandep)  src/cp.c
man/csplit.1:    $(mandep)  src/csplit.c
man/cut.1:       $(mandep)  src/cut.c
man/date.1:      $(mandep)  src/date.c
man/dd.1:        $(mandep)  src/dd.c
man/df.1:        $(mandep)  src/df.c

# Note that dir depends on ls.c, since that's where its --help text is.
man/dir.1:       $(mandep)  src/ls.c

man/dircolors.1: $(mandep)  src/dircolors.c
man/dirname.1:   $(mandep)  src/dirname.c
man/du.1:        $(mandep)  src/du.c
man/echo.1:      $(mandep)  src/echo.c
man/env.1:       $(mandep)  src/env.c
man/expand.1:    $(mandep)  src/expand.c
man/expr.1:      $(mandep)  src/expr.c
man/factor.1:    $(mandep)  src/factor.c
man/false.1:     $(mandep)  src/false.c
man/fmt.1:       $(mandep)  src/fmt.c
man/fold.1:      $(mandep)  src/fold.c
man/groups.1:    $(mandep)  src/groups.c
man/head.1:      $(mandep)  src/head.c
man/hostid.1:    $(mandep)  src/hostid.c
man/hostname.1:  $(mandep)  src/hostname.c
man/id.1:        $(mandep)  src/id.c
man/install.1:   $(mandep)  src/install.c
man/join.1:      $(mandep)  src/join.c
man/kill.1:      $(mandep)  src/kill.c
man/link.1:      $(mandep)  src/link.c
man/ln.1:        $(mandep)  src/ln.c
man/logname.1:   $(mandep)  src/logname.c
man/ls.1:        $(mandep)  src/ls.c
man/md5sum.1:    $(mandep)  src/md5sum.c
man/mkdir.1:     $(mandep)  src/mkdir.c
man/mkfifo.1:    $(mandep)  src/mkfifo.c
man/mknod.1:     $(mandep)  src/mknod.c
man/mktemp.1:    $(mandep)  src/mktemp.c
man/mv.1:        $(mandep)  src/mv.c
man/nice.1:      $(mandep)  src/nice.c
man/nl.1:        $(mandep)  src/nl.c
man/nohup.1:     $(mandep)  src/nohup.c
man/nproc.1:     $(mandep)  src/nproc.c
man/od.1:        $(mandep)  src/od.c
man/paste.1:     $(mandep)  src/paste.c
man/pathchk.1:   $(mandep)  src/pathchk.c
man/pinky.1:     $(mandep)  src/pinky.c
man/pr.1:        $(mandep)  src/pr.c
man/printenv.1:  $(mandep)  src/printenv.c
man/printf.1:    $(mandep)  src/printf.c
man/ptx.1:       $(mandep)  src/ptx.c
man/pwd.1:       $(mandep)  src/pwd.c
man/readlink.1:  $(mandep)  src/readlink.c
man/realpath.1:  $(mandep)  src/realpath.c
man/rm.1:        $(mandep)  src/rm.c
man/rmdir.1:     $(mandep)  src/rmdir.c
man/runcon.1:    $(mandep)  src/runcon.c
man/seq.1:       $(mandep)  src/seq.c
man/sha1sum.1:   $(mandep)  src/md5sum.c
man/sha224sum.1: $(mandep)  src/md5sum.c
man/sha256sum.1: $(mandep)  src/md5sum.c
man/sha384sum.1: $(mandep)  src/md5sum.c
man/sha512sum.1: $(mandep)  src/md5sum.c
man/shred.1:     $(mandep)  src/shred.c
man/shuf.1:      $(mandep)  src/shuf.c
man/sleep.1:     $(mandep)  src/sleep.c
man/sort.1:      $(mandep)  src/sort.c
man/split.1:     $(mandep)  src/split.c
man/stat.1:      $(mandep)  src/stat.c
man/stdbuf.1:    $(mandep)  src/stdbuf.c
man/stty.1:      $(mandep)  src/stty.c
man/sum.1:       $(mandep)  src/sum.c
man/sync.1:      $(mandep)  src/sync.c
man/tac.1:       $(mandep)  src/tac.c
man/tail.1:      $(mandep)  src/tail.c
man/tee.1:       $(mandep)  src/tee.c
man/test.1:      $(mandep)  src/test.c
man/timeout.1:   $(mandep)  src/timeout.c
man/touch.1:     $(mandep)  src/touch.c
man/tr.1:        $(mandep)  src/tr.c
man/true.1:      $(mandep)  src/true.c
man/truncate.1:  $(mandep)  src/truncate.c
man/tsort.1:     $(mandep)  src/tsort.c
man/tty.1:       $(mandep)  src/tty.c
man/uname.1:     $(mandep)  src/uname.c
man/unexpand.1:  $(mandep)  src/unexpand.c
man/uniq.1:      $(mandep)  src/uniq.c
man/unlink.1:    $(mandep)  src/unlink.c
man/uptime.1:    $(mandep)  src/uptime.c
man/users.1:     $(mandep)  src/users.c
man/vdir.1:      $(mandep)  src/ls.c
man/wc.1:        $(mandep)  src/wc.c
man/who.1:       $(mandep)  src/who.c
man/whoami.1:    $(mandep)  src/whoami.c
man/yes.1:       $(mandep)  src/yes.c

# This is required so that changes to e.g., emit_bug_reporting_address
# provoke regeneration of all $(MAN) files.
$(MAN): $(top_srcdir)/src/system.h

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
	  && mkdir $$t							\
	  && (cd $$t && $(LN_S) $(abs_top_builddir)/src/$$prog $$name)	\
	  && $(PERL) -- $(srcdir)/man/help2man				\
		     --source='$(PACKAGE_STRING)'			\
		     --include=$(srcdir)/man/$$name.x			\
		     --output=$$t/$$name.1 $$t/$$name			\
	  && sed 's|$*\.td/||g' $$t/$$name.1 > $@-t			\
	  && rm -rf $$t							\
	  && chmod -w $@-t						\
	  && mv $@-t $@
