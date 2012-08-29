# Make coreutils man pages.				-*-Makefile-*-

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

EXTRA_DIST = $(man_aux) help2man
MAINTAINERCLEANFILES = $(dist_man1_MANS)

# The "$(VAR:%=%.x)" idiom is not portable according to POSIX, but in
# practice it works with several make implementation (GNU, BSD, CCS make
# from Solaris 10, Sun distributed make).  In addition, since only
# maintainers are expected to build release tarballs (and they should
# use GNU make when doing so), it's not big deal if this won't work with
# some dumber make implementation.
EXTRA_DIST += \
  $(NO_INSTALL_PROGS_DEFAULT:%=%.x) \
  $(NO_INSTALL_PROGS_DEFAULT:%=%.1)

# Depend on ../.version to get version number changes.
mandep = ../.version

# Note that arch depends on uname.c
arch.1:		$(mandep)	../src/uname.c

base64.1:	$(mandep)	../src/base64.c
basename.1:	$(mandep)	../src/basename.c
cat.1:		$(mandep)	../src/cat.c
chcon.1:	$(mandep)	../src/chcon.c
chgrp.1:	$(mandep)	../src/chgrp.c
chmod.1:	$(mandep)	../src/chmod.c
chown.1:	$(mandep)	../src/chown.c
chroot.1:	$(mandep)	../src/chroot.c
cksum.1:	$(mandep)	../src/cksum.c
comm.1:		$(mandep)	../src/comm.c
cp.1:		$(mandep)	../src/cp.c
csplit.1:	$(mandep)	../src/csplit.c
cut.1:		$(mandep)	../src/cut.c
date.1:		$(mandep)	../src/date.c
dd.1:		$(mandep)	../src/dd.c
df.1:		$(mandep)	../src/df.c

# Note that dir depends on ls.c, since that's where its --help text is.
dir.1:		$(mandep)	../src/ls.c

dircolors.1:	$(mandep)	../src/dircolors.c
dirname.1:	$(mandep)	../src/dirname.c
du.1:		$(mandep)	../src/du.c
echo.1:		$(mandep)	../src/echo.c
env.1:		$(mandep)	../src/env.c
expand.1:	$(mandep)	../src/expand.c
expr.1:		$(mandep)	../src/expr.c
factor.1:	$(mandep)	../src/factor.c
false.1:	$(mandep)	../src/false.c
fmt.1:		$(mandep)	../src/fmt.c
fold.1:		$(mandep)	../src/fold.c
groups.1:	$(mandep)	../src/groups.c
head.1:		$(mandep)	../src/head.c
hostid.1:	$(mandep)	../src/hostid.c
hostname.1:	$(mandep)	../src/hostname.c
id.1:		$(mandep)	../src/id.c
install.1:	$(mandep)	../src/install.c
join.1:		$(mandep)	../src/join.c
kill.1:		$(mandep)	../src/kill.c
link.1:		$(mandep)	../src/link.c
ln.1:		$(mandep)	../src/ln.c
logname.1:	$(mandep)	../src/logname.c
ls.1:		$(mandep)	../src/ls.c
md5sum.1:	$(mandep)	../src/md5sum.c
mkdir.1:	$(mandep)	../src/mkdir.c
mkfifo.1:	$(mandep)	../src/mkfifo.c
mknod.1:	$(mandep)	../src/mknod.c
mktemp.1:	$(mandep)	../src/mktemp.c
mv.1:		$(mandep)	../src/mv.c
nice.1:		$(mandep)	../src/nice.c
nl.1:		$(mandep)	../src/nl.c
nohup.1:	$(mandep)	../src/nohup.c
nproc.1:	$(mandep)	../src/nproc.c
od.1:		$(mandep)	../src/od.c
paste.1:	$(mandep)	../src/paste.c
pathchk.1:	$(mandep)	../src/pathchk.c
pinky.1:	$(mandep)	../src/pinky.c
pr.1:		$(mandep)	../src/pr.c
printenv.1:	$(mandep)	../src/printenv.c
printf.1:	$(mandep)	../src/printf.c
ptx.1:		$(mandep)	../src/ptx.c
pwd.1:		$(mandep)	../src/pwd.c
readlink.1:	$(mandep)	../src/readlink.c
realpath.1:	$(mandep)	../src/realpath.c
rm.1:		$(mandep)	../src/rm.c
rmdir.1:	$(mandep)	../src/rmdir.c
runcon.1:	$(mandep)	../src/runcon.c
seq.1:		$(mandep)	../src/seq.c
sha1sum.1:	$(mandep)	../src/md5sum.c
sha224sum.1:	$(mandep)	../src/md5sum.c
sha256sum.1:	$(mandep)	../src/md5sum.c
sha384sum.1:	$(mandep)	../src/md5sum.c
sha512sum.1:	$(mandep)	../src/md5sum.c
shred.1:	$(mandep)	../src/shred.c
shuf.1:		$(mandep)	../src/shuf.c
sleep.1:	$(mandep)	../src/sleep.c
sort.1:		$(mandep)	../src/sort.c
split.1:	$(mandep)	../src/split.c
stat.1:		$(mandep)	../src/stat.c
stdbuf.1:	$(mandep)	../src/stdbuf.c
stty.1:		$(mandep)	../src/stty.c
sum.1:		$(mandep)	../src/sum.c
sync.1:		$(mandep)	../src/sync.c
tac.1:		$(mandep)	../src/tac.c
tail.1:		$(mandep)	../src/tail.c
tee.1:		$(mandep)	../src/tee.c
test.1:		$(mandep)	../src/test.c
timeout.1:	$(mandep)	../src/timeout.c
touch.1:	$(mandep)	../src/touch.c
tr.1:		$(mandep)	../src/tr.c
true.1:		$(mandep)	../src/true.c
truncate.1:	$(mandep)	../src/truncate.c
tsort.1:	$(mandep)	../src/tsort.c
tty.1:		$(mandep)	../src/tty.c
uname.1:	$(mandep)	../src/uname.c
unexpand.1:	$(mandep)	../src/unexpand.c
uniq.1:		$(mandep)	../src/uniq.c
unlink.1:	$(mandep)	../src/unlink.c
uptime.1:	$(mandep)	../src/uptime.c
users.1:	$(mandep)	../src/users.c
vdir.1:		$(mandep)	../src/ls.c
wc.1:		$(mandep)	../src/wc.c
who.1:		$(mandep)	../src/who.c
whoami.1:	$(mandep)	../src/whoami.c
yes.1:		$(mandep)	../src/yes.c

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
	  && $(PERL) -- $(srcdir)/help2man				\
		     --source='$(PACKAGE_STRING)'			\
		     --include=$(srcdir)/$$name.x			\
		     --output=$$t/$$name.1 $$t/$$name			\
	  && sed 's|$*\.td/||g' $$t/$$name.1 > $@-t			\
	  && rm -rf $$t							\
	  && chmod -w $@-t						\
	  && mv $@-t $@

# Option descriptions should not start with a capital letter
.PHONY: sc_option_desc_uppercase
sc_option_desc_uppercase: $(dist_man1_MANS) $(NO_INSTALL_PROGS_DEFAULT:%=%.1)
	@grep '^\\fB\\-' -A1 *.1 | LC_ALL=C grep '\.1.[A-Z][a-z]' && \
	    { echo 1>&2 '$@: found initial capitals in --help';   \
	      exit 1; } || :;

.PHONY: sc_man_file_correlation
sc_man_file_correlation: check-x-vs-1 check-programs-vs-x

# Sort in traditional ASCII order, regardless of the current locale;
# otherwise we may get into trouble with distinct strings that the
# current locale considers to be equal.
ASSORT = LC_ALL=C sort

# Ensure that for each .x file in this directory, there is a
# corresponding .1 file in the definition of $(dist_man1_MANS) above.
# But since that expansion usually lacks programs like arch and hostname,
# add them here manually.
.PHONY: check-x-vs-1
check-x-vs-1:
	@PATH=../src$(PATH_SEPARATOR)$$PATH; export PATH;		\
	t=$@-t;								\
	(cd $(srcdir) && ls -1 *.x) | sed 's/\.x$$//' | $(ASSORT) > $$t;\
	(echo $(dist_man1_MANS) $(NO_INSTALL_PROGS_DEFAULT)		\
	  | tr -s ' ' '\n' | sed 's/\.1$$//')				\
	  | $(ASSORT) -u | diff - $$t || { rm $$t; exit 1; };		\
	rm $$t

all_programs =								\
      (cd ../src && MAKEFLAGS= $(MAKE) -s all_programs.list)		\
       | grep -v '\['

.PHONY: check-programs-vs-x
check-programs-vs-x:
	@status=0;					\
	for p in dummy `$(all_programs)`; do		\
	  test $$p = dummy && continue;			\
	  test $$p = ginstall && p=install || : ;	\
	  test -f $(srcdir)/$$p.x			\
	    || { echo missing $$p.x 1>&2; status=1; };	\
	done;						\
	exit $$status
