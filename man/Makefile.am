## Process this file with automake to produce Makefile.in -*-Makefile-*-
dist_man_MANS = \
  basename.1 cat.1 chgrp.1 chmod.1 chown.1 chroot.1 cksum.1 comm.1 \
  cp.1 csplit.1 cut.1 date.1 dd.1 df.1 dir.1 dircolors.1 dirname.1 du.1 \
  echo.1 env.1 expand.1 expr.1 factor.1 false.1 fmt.1 fold.1 groups.1 \
  head.1 hostid.1 hostname.1 id.1 install.1 join.1 link.1 ln.1 logname.1 \
  ls.1 md5sum.1 mkdir.1 mkfifo.1 mknod.1 mv.1 nice.1 nl.1 nohup.1 od.1 \
  paste.1 pathchk.1 pinky.1 pr.1 printenv.1 printf.1 ptx.1 pwd.1 readlink.1 \
  rm.1 rmdir.1 seq.1 sha1sum.1 shred.1 sleep.1 sort.1 split.1 stat.1 stty.1 \
  su.1 sum.1 sync.1 tac.1 tail.1 tee.1 test.1 touch.1 tr.1 true.1 tsort.1 \
  tty.1 uname.1 unexpand.1 uniq.1 unlink.1 uptime.1 users.1 vdir.1 wc.1 \
  who.1 whoami.1 yes.1

man_aux = $(dist_man_MANS:.1=.x)

EXTRA_DIST = $(man_aux)
MAINTAINERCLEANFILES = $(man_MANS)

# Depend on configure.ac to get version number changes.
common_dep = $(top_srcdir)/configure.ac

basename.1:	$(common_dep)	$(srcdir)/basename.x	../src/basename.c
cat.1:		$(common_dep)	$(srcdir)/cat.x		../src/cat.c
chgrp.1:	$(common_dep)	$(srcdir)/chgrp.x	../src/chgrp.c
chmod.1:	$(common_dep)	$(srcdir)/chmod.x	../src/chmod.c
chown.1:	$(common_dep)	$(srcdir)/chown.x	../src/chown.c
chroot.1:	$(common_dep)	$(srcdir)/chroot.x	../src/chroot.c
cksum.1:	$(common_dep)	$(srcdir)/cksum.x	../src/cksum.c
comm.1:		$(common_dep)	$(srcdir)/comm.x	../src/comm.c
cp.1:		$(common_dep)	$(srcdir)/cp.x		../src/cp.c
csplit.1:	$(common_dep)	$(srcdir)/csplit.x	../src/csplit.c
cut.1:		$(common_dep)	$(srcdir)/cut.x		../src/cut.c
date.1:		$(common_dep)	$(srcdir)/date.x	../src/date.c
dd.1:		$(common_dep)	$(srcdir)/dd.x		../src/dd.c
df.1:		$(common_dep)	$(srcdir)/df.x		../src/df.c

# Note that dir depends on ls.c, since that's where it's --help text is.
dir.1:		$(common_dep)	$(srcdir)/dir.x		../src/ls.c

dircolors.1:	$(common_dep)	$(srcdir)/dircolors.x	../src/dircolors.c
dirname.1:	$(common_dep)	$(srcdir)/dirname.x	../src/dirname.c
du.1:		$(common_dep)	$(srcdir)/du.x		../src/du.c
echo.1:		$(common_dep)	$(srcdir)/echo.x	../src/echo.c
env.1:		$(common_dep)	$(srcdir)/env.x		../src/env.c
expand.1:	$(common_dep)	$(srcdir)/expand.x	../src/expand.c
expr.1:		$(common_dep)	$(srcdir)/expr.x	../src/expr.c
factor.1:	$(common_dep)	$(srcdir)/factor.x	../src/factor.c
false.1:	$(common_dep)	$(srcdir)/false.x	../src/false.c
fmt.1:		$(common_dep)	$(srcdir)/fmt.x		../src/fmt.c
fold.1:		$(common_dep)	$(srcdir)/fold.x	../src/fold.c
groups.1:	$(common_dep)	$(srcdir)/groups.x	../src/groups.sh
head.1:		$(common_dep)	$(srcdir)/head.x	../src/head.c
hostid.1:	$(common_dep)	$(srcdir)/hostid.x	../src/hostid.c
hostname.1:	$(common_dep)	$(srcdir)/hostname.x	../src/hostname.c
id.1:		$(common_dep)	$(srcdir)/id.x		../src/id.c
install.1:	$(common_dep)	$(srcdir)/install.x	../src/install.c
join.1:		$(common_dep)	$(srcdir)/join.x	../src/join.c
link.1:		$(common_dep)	$(srcdir)/link.x	../src/link.c
ln.1:		$(common_dep)	$(srcdir)/ln.x		../src/ln.c
logname.1:	$(common_dep)	$(srcdir)/logname.x	../src/logname.c
ls.1:		$(common_dep)	$(srcdir)/ls.x		../src/ls.c
md5sum.1:	$(common_dep)	$(srcdir)/md5sum.x	../src/md5sum.c
mkdir.1:	$(common_dep)	$(srcdir)/mkdir.x	../src/mkdir.c
mkfifo.1:	$(common_dep)	$(srcdir)/mkfifo.x	../src/mkfifo.c
mknod.1:	$(common_dep)	$(srcdir)/mknod.x	../src/mknod.c
mv.1:		$(common_dep)	$(srcdir)/mv.x		../src/mv.c
nice.1:		$(common_dep)	$(srcdir)/nice.x	../src/nice.c
nl.1:		$(common_dep)	$(srcdir)/nl.x		../src/nl.c
nohup.1:	$(common_dep)	$(srcdir)/nohup.x	../src/nohup.sh
od.1:		$(common_dep)	$(srcdir)/od.x		../src/od.c
paste.1:	$(common_dep)	$(srcdir)/paste.x	../src/paste.c
pathchk.1:	$(common_dep)	$(srcdir)/pathchk.x	../src/pathchk.c
pinky.1:	$(common_dep)	$(srcdir)/pinky.x	../src/pinky.c
pr.1:		$(common_dep)	$(srcdir)/pr.x		../src/pr.c
printenv.1:	$(common_dep)	$(srcdir)/printenv.x	../src/printenv.c
printf.1:	$(common_dep)	$(srcdir)/printf.x	../src/printf.c
ptx.1:		$(common_dep)	$(srcdir)/ptx.x		../src/ptx.c
pwd.1:		$(common_dep)	$(srcdir)/pwd.x		../src/pwd.c
readlink.1:	$(common_dep)	$(srcdir)/readlink.x	../src/readlink.c
rm.1:		$(common_dep)	$(srcdir)/rm.x		../src/rm.c
rmdir.1:	$(common_dep)	$(srcdir)/rmdir.x	../src/rmdir.c
seq.1:		$(common_dep)	$(srcdir)/seq.x		../src/seq.c
sha1sum.1:	$(common_dep)	$(srcdir)/sha1sum.x	../src/md5sum.c
shred.1:	$(common_dep)	$(srcdir)/shred.x	../src/shred.c
sleep.1:	$(common_dep)	$(srcdir)/sleep.x	../src/sleep.c
sort.1:		$(common_dep)	$(srcdir)/sort.x	../src/sort.c
split.1:	$(common_dep)	$(srcdir)/split.x	../src/split.c
stat.1:		$(common_dep)	$(srcdir)/stat.x	../src/stat.c
stty.1:		$(common_dep)	$(srcdir)/stty.x	../src/stty.c
su.1:		$(common_dep)	$(srcdir)/su.x		../src/su.c
sum.1:		$(common_dep)	$(srcdir)/sum.x		../src/sum.c
sync.1:		$(common_dep)	$(srcdir)/sync.x	../src/sync.c
tac.1:		$(common_dep)	$(srcdir)/tac.x		../src/tac.c
tail.1:		$(common_dep)	$(srcdir)/tail.x	../src/tail.c
tee.1:		$(common_dep)	$(srcdir)/tee.x		../src/tee.c
test.1:		$(common_dep)	$(srcdir)/test.x	../src/test.c
touch.1:	$(common_dep)	$(srcdir)/touch.x	../src/touch.c
tr.1:		$(common_dep)	$(srcdir)/tr.x		../src/tr.c
true.1:		$(common_dep)	$(srcdir)/true.x	../src/true.c
tsort.1:	$(common_dep)	$(srcdir)/tsort.x	../src/tsort.c
tty.1:		$(common_dep)	$(srcdir)/tty.x		../src/tty.c
uname.1:	$(common_dep)	$(srcdir)/uname.x	../src/uname.c
unexpand.1:	$(common_dep)	$(srcdir)/unexpand.x	../src/unexpand.c
uniq.1:		$(common_dep)	$(srcdir)/uniq.x	../src/uniq.c
unlink.1:	$(common_dep)	$(srcdir)/unlink.x	../src/unlink.c
uptime.1:	$(common_dep)	$(srcdir)/uptime.x	../src/uptime.c
users.1:	$(common_dep)	$(srcdir)/users.x	../src/users.c
vdir.1:		$(common_dep)	$(srcdir)/vdir.x	../src/ls.c
wc.1:		$(common_dep)	$(srcdir)/wc.x		../src/wc.c
who.1:		$(common_dep)	$(srcdir)/who.x		../src/who.c
whoami.1:	$(common_dep)	$(srcdir)/whoami.x	../src/whoami.c
yes.1:		$(common_dep)	$(srcdir)/yes.x		../src/yes.c

SUFFIXES = .x .1

# Ensure that help2man runs the ../src/ginstall binary as
# `install' when creating install.1.
t = $*.td
mapped_name = `echo $*|sed 's/install/ginstall/'`

# Note the use of $t/$*, rather than just `$*' as in other packages.
# That is necessary to avoid failures for programs that are also shell built-in
# functions like echo, false, printf, pwd.
.x.1:
	@echo "Updating man page $@";		\
	mkdir $t;				\
	(cd $t && $(LN_S) ../../src/$(mapped_name)$(EXEEXT) $*$(EXEEXT)); \
	$(HELP2MAN)				\
	    --include=$(srcdir)/$*.x		\
	    --output=$@ $t/$*$(EXEEXT);		\
	rm -rf $t

check-local: check-x-vs-1

# Ensure that for each .x file in this directory, there is a
# corresponding .1 file in the definition of $(dist_man_MANS) above.
.PHONY: check-x-vs-1
check-x-vs-1:
	PATH=../src@PATH_SEPARATOR@$$PATH; export PATH;			\
	t=ls-files.$$$$;						\
	(cd $(srcdir) && ls -1 *.x) | sed 's/\.x$$//' | sort > $$t;	\
	echo $(dist_man_MANS) | fmt -w1 | sed 's/\.1$$//' | sort -u	\
	  | diff - $$t || { rm $$t; exit 1; };				\
	rm $$t
