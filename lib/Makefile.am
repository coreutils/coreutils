## Process this file with automake to produce Makefile.in -*-Makefile-*-

AUTOMAKE_OPTIONS = ../src/ansi2knr

noinst_LIBRARIES = libfetish.a

INCLUDES = -I.. -I$(srcdir) -I../intl
DEFS = -DLIBDIR=\"$(libdir)\" @DEFS@

libfetish_a_SOURCES = \
  getdate.y posixtm.c addext.c argmatch.c backupfile.c basename.c \
  canon-host.c closeout.c dirname.c exclude.c filemode.c diacrit.c \
  full-write.c getopt.c getopt1.c getugroups.c hard-locale.c hash.c \
  human.c idcache.c isdir.c linebuffer.c localcharset.c long-options.c \
  makepath.c md5.c memcasecmp.c memcoll.c modechange.c path-concat.c \
  quotearg.c readtokens.c readutmp.c safe-read.c same.c save-cwd.c \
  savedir.c stripslash.c unicodeio.c userspec.c version-etc.c xgetcwd.c \
  xgethostname.c xmalloc.c xstrdup.c xstrtod.c xstrtol.c xstrtoul.c \
  xstrtoumax.c yesno.c

libfetish_a_LIBADD = @LIBOBJS@ @ALLOCA@
libfetish_a_DEPENDENCIES = $(libfetish_a_LIBADD)

noinst_HEADERS = \
  argmatch.h backupfile.h bumpalloc.h closeout.h diacrit.h dirname.h error.h \
  exclude.h filemode.h fnmatch.h fsusage.h getdate.h getline.h getopt.h \
  getpagesize.h group-member.h hard-locale.h hash.h human.h lchown.h \
  linebuffer.h long-options.h md5.h memcasecmp.h memcoll.h \
  makepath.h modechange.h mountlist.h nanosleep.h obstack.h \
  path-concat.h pathmax.h posixtm.h quotearg.h readtokens.h \
  readutmp.h regex.h safe-read.h same.h save-cwd.h savedir.h \
  strverscmp.h unicodeio.h version-etc.h xalloc.h xstrtod.h xstrtol.h

BUILT_SOURCES = getdate.c lstat.c stat.c
MAINTAINERCLEANFILES = $(BUILT_SOURCES)
DISTCLEANFILES = lstat.c stat.c

EXTRA_DIST = xstat.in config.charset ref-add.sed.in ref-del.sed.in
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


# The following is needed in order to install a simple file in $(libdir)
# which is shared with other installed packages. We use a list of referencing
# packages so that "make uninstall" will remove the file if and only if it
# is not used by another installed package.

all-local: charset.alias ref-add.sed ref-del.sed

charset_alias = $(DESTDIR)$(libdir)/charset.alias
install-exec-local: all-local
	$(mkinstalldirs) $(DESTDIR)$(libdir)
	test -f $(charset_alias) \
	  && orig=$(charset_alias) \
	  || orig=charset.alias; \
	sed -f ref-add.sed $$orig > $(charset_alias)-t
	$(INSTALL_DATA) $(charset_alias)-t $(charset_alias)
	rm -f $(charset_alias)-t

uninstall-local: all-local
	if test -f $(charset_alias); then \
	  sed -f ref-del.sed $(charset_alias) > $(charset_alias)-t; \
	  if grep '^# Packages using this file: $$' $(charset_alias)-t \
	      > /dev/null; then \
	    rm -f $(charset_alias); \
	  else \
	    $(INSTALL_DATA) $(charset_alias)-t $(charset_alias); \
	  fi; \
	  rm -f $(charset_alias)-t; \
	fi

charset.alias: config.charset
	$(SHELL) $(srcdir)/config.charset '@host@' > $@-t
	mv $@-t $@

SUFFIXES = .sed .sed.in
.sed.in.sed:
	sed -e '/^#/d' -e 's/@''PACKAGE''@/@PACKAGE@/g' $< > $@-t
	mv $@-t $@

CLEANFILES = charset.alias ref-add.sed ref-del.sed
