## Process this file with automake to produce Makefile.in -*-Makefile-*-

AUTOMAKE_OPTIONS = ../src/ansi2knr

noinst_LIBRARIES = libfetish.a

INCLUDES = -I.. -I$(srcdir) -I../intl

libfetish_a_SOURCES = getdate.y posixtm.c getopt.c getopt1.c \
addext.c argmatch.c backupfile.c basename.c \
closeout.c dirname.c exclude.c filemode.c \
full-write.c human.c idcache.c \
isdir.c long-options.c makepath.c modechange.c hash.c path-concat.c \
quotearg.c safe-read.c same.c save-cwd.c savedir.c stripslash.c userspec.c \
version-etc.c xgetcwd.c xmalloc.c xstrdup.c \
xstrtol.c xstrtoul.c xstrtoumax.c yesno.c

libfetish_a_LIBADD = @LIBOBJS@ @ALLOCA@
libfetish_a_DEPENDENCIES = $(libfetish_a_LIBADD)

noinst_HEADERS = argmatch.h backupfile.h closeout.h \
dirname.h error.h exclude.h filemode.h fnmatch.h fsusage.h \
getdate.h getline.h getopt.h getpagesize.h \
group-member.h hash.h human.h lchown.h long-options.h \
makepath.h modechange.h mountlist.h nanosleep.h path-concat.h pathmax.h \
posixtm.h quotearg.h regex.h safe-read.h same.h save-cwd.h \
savedir.h strverscmp.h \
version-etc.h xalloc.h xstrtol.h

BUILT_SOURCES = getdate.c lstat.c stat.c
MAINTAINERCLEANFILES = $(BUILT_SOURCES)
DISTCLEANFILES = lstat.c stat.c

EXTRA_DIST = xstat.in
lstat.c: xstat.in
	sed \
	  -e '/@IGNORE@/d' \
	  -e 's/@xstat@/lstat/g' \
	  -e '/_LSTAT_ONLY@/d' \
	  -e '/@BEGIN_STAT_ONLY@/,/@END_STAT_ONLY@/d' \
	  $(srcdir)/xstat.in > $@-t
	mv $@-t $@

stat.c: xstat.in
	sed \
	  -e '/@IGNORE@/d' \
	  -e 's/@xstat@/stat/g' \
	  -e '/_STAT_ONLY@/d' \
	  -e '/@BEGIN_LSTAT_ONLY@/,/@END_LSTAT_ONLY@/d' \
	  $(srcdir)/xstat.in > $@-t
	mv $@-t $@
