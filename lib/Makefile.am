noinst_LIBRARIES = fu

DIST_OTHER = alloca.c euidaccess.c fnmatch.c fsusage.c ftruncate.c \
getdate.y memcmp.c memcpy.c memset.c mkdir.c mktime.c mountlist.c \
posixtm.y rename.c rmdir.c stpcpy.c strdup.c strstr.c strtol.c strtoul.c

INCLUDES = -I.. -I$(srcdir)

fu_LIBADD = @LIBOBJS@ @ALLOCA@

HEADERS = argmatch.h backupfile.h error.h fnmatch.h fsusage.h getopt.h \
group-member.h long-options.h makepath.h modechange.h mountlist.h \
obstack.h pathmax.h save-cwd.h xstrtol.h xstrtoul.h

BUILT_SOURCES = posixtm.c getdate.c
CONFIG_HEADER = ../config.h

fu_SOURCES = getdate.c posixtm.c argmatch.c backupfile.c basename.c \
dirname.c error.c fileblocks.c filemode.c \
full-write.c getopt.c getopt1.c getversion.c group-member.c idcache.c \
isdir.c long-options.c makepath.c modechange.c obstack.c \
safe-read.c save-cwd.c savedir.c stripslash.c userspec.c xgetcwd.c \
xmalloc.c xstrdup.c xstrtol.c xstrtoul.c yesno.c
