noinst_LIBRARIES = fu

EXTRA_DIST = alloca.c euidaccess.c fnmatch.c fsusage.c ftruncate.c \
getdate.y memcmp.c memcpy.c memset.c mkdir.c mktime.c mountlist.c \
posixtm.y rename.c rmdir.c stpcpy.c strdup.c strstr.c strtol.c strtoul.c

INCLUDES = -I.. -I$(srcdir)

fu_SOURCES = getdate.c posixtm.c argmatch.c backupfile.c basename.c \
dirname.c error.c fileblocks.c filemode.c \
full-write.c getopt.c getopt1.c getversion.c group-member.c idcache.c \
isdir.c long-options.c makepath.c modechange.c obstack.c \
safe-read.c save-cwd.c savedir.c stripslash.c userspec.c xgetcwd.c \
xmalloc.c xstrdup.c xstrtol.c xstrtoul.c yesno.c

fu_LIBADD = @LIBOBJS@ @ALLOCA@

noinst_HEADERS = argmatch.h backupfile.h error.h fnmatch.h fsusage.h \
getopt.h group-member.h long-options.h makepath.h modechange.h mountlist.h \
obstack.h pathmax.h save-cwd.h xstrtol.h xstrtoul.h

BUILT_SOURCES = getdate.c posixtm.c

# Since this directory contains two parsers, we have to be careful to avoid
# running two $(YACC)s during parallel makes.  See below.
getdate.c: getdate.y
	@echo expect 10 shift/reduce conflicts
	$(YACC) $(srcdir)/getdate.y
	mv y.tab.c getdate.c

# Make the rename atomic, in case sed is interrupted and later rerun.
# The artificial dependency on getdate.c keeps the two parsers from being
# built in parallel.  Enforcing this little bit of sequentiality lets
# everyone (even those without bison) still run mostly parallel builds.
posixtm.c: posixtm.y getdate.c
	$(YACC) $(srcdir)/posixtm.y
	mv y.tab.c posixtm.tab.c
	sed -e 's/yy/zz/g' posixtm.tab.c > tposixtm.c
	mv tposixtm.c posixtm.c
	rm -f posixtm.tab.c
