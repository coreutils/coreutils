## Process this file with automake to produce Makefile.in
noinst_LIBRARIES = fu

EXTRA_DIST = basename.c euidaccess.c \
group-member.c memcpy.c memset.c mkdir.c \
rename.c rmdir.c rpmatch.c \
stpcpy.c strcasecmp.c strdup.c strndup.c strstr.c strtol.c strtoul.c

INCLUDES = -I.. -I$(srcdir)
INCLUDES = -I.. -I$(srcdir) -I../intl

fu_SOURCES = getdate.y posixtm.y getopt.c getopt1.c \
argmatch.c backupfile.c dirname.c filemode.c \
full-write.c getversion.c idcache.c \
isdir.c long-options.c makepath.c modechange.c path-concat.c \
safe-read.c save-cwd.c savedir.c stripslash.c userspec.c xgetcwd.c \
xmalloc.c xstrdup.c xstrtol.c xstrtoul.c yesno.c

fu_LIBADD = @LIBOBJS@ @ALLOCA@
fu_DEPENDENCIES = $(fu_LIBADD)

noinst_HEADERS = argmatch.h backupfile.h error.h fnmatch.h fsusage.h \
getline.h getopt.h group-member.h long-options.h makepath.h modechange.h \
mountlist.h path-concat.h pathmax.h \
save-cwd.h xstrtol.h xstrtoul.h

BUILT_SOURCES = getdate.c posixtm.c
