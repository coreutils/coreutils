## Process this file with automake to produce Makefile.in
noinst_LIBRARIES = libfu.a

EXTRA_DIST = chown.c getgroups.c getline.c lstat.c mktime.c stat.c strftime.c

INCLUDES = -I.. -I$(srcdir) -I../intl

libfu_a_SOURCES = getdate.y posixtm.y getopt.c getopt1.c \
addext.c argmatch.c backupfile.c basename.c dirname.c filemode.c \
full-write.c idcache.c \
isdir.c long-options.c makepath.c modechange.c oa-hash.c path-concat.c \
safe-read.c save-cwd.c savedir.c stripslash.c userspec.c xgetcwd.c \
xmalloc.c xstrdup.c xstrtol.c xstrtoul.c yesno.c

libfu_a_LIBADD = @LIBOBJS@ @ALLOCA@
libfu_a_DEPENDENCIES = $(libfu_a_LIBADD)

noinst_HEADERS = argmatch.h backupfile.h error.h fnmatch.h fsusage.h \
getdate.h getline.h getopt.h group-member.h long-options.h \
makepath.h modechange.h mountlist.h oa-hash.h path-concat.h pathmax.h \
save-cwd.h xstrtol.h xstrtoul.h strverscmp.h

BUILT_SOURCES = getdate.c posixtm.c
