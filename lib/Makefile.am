## Process this file with automake to produce Makefile.in -*-Makefile-*-

AUTOMAKE_OPTIONS = ../src/ansi2knr

noinst_LIBRARIES = libfetish.a

INCLUDES = -I.. -I$(srcdir) -I../intl
DEFS = -DLIBDIR=\"$(libdir)\" @DEFS@

## Put relatively complex files at the beginning of the list so
## that parallel compiles finish a tiny bit sooner.  I don't see
## a way to make regex.c appear earlier in the list, since it's
## added by automake, but on systems with an up to date GNU libc,
## regex.c isn't even compiled.
libfetish_a_SOURCES = \
  getdate.h getdate.y \
  posixtm.c posixtm.h \
  strftime.c \
  getopt.c getopt.h getopt1.c \
  hash.c hash.h \
  __fpending.h \
  addext.c \
  argmatch.c argmatch.h \
  backupfile.c backupfile.h \
  basename.c \
  bumpalloc.h \
  canon-host.c \
  closeout.c closeout.h \
  diacrit.c diacrit.h \
  dirname.c dirname.h \
  dup-safer.c \
  exclude.c exclude.h \
  filemode.c filemode.h \
  fnmatch.h \
  fopen-safer.c \
  fsusage.h \
  full-write.c \
  getline.h \
  getpagesize.h \
  getstr.c getstr.h \
  getugroups.c \
  group-member.h \
  hard-locale.c hard-locale.h \
  human.c human.h \
  idcache.c \
  isdir.c \
  lchown.h \
  linebuffer.c linebuffer.h \
  localcharset.c \
  long-options.c long-options.h \
  makepath.c makepath.h \
  mbswidth.c mbswidth.h \
  md5.c md5.h \
  memcasecmp.c memcasecmp.h \
  memcoll.c memcoll.h \
  modechange.c modechange.h \
  mountlist.h \
  nanosleep.h \
  path-concat.c path-concat.h \
  pathmax.h \
  physmem.c physmem.h \
  quote.c quote.h \
  quotearg.c quotearg.h \
  readtokens.c readtokens.h \
  readutmp.h \
  regex.h \
  safe-read.c safe-read.h \
  same.c same.h \
  save-cwd.c save-cwd.h \
  savedir.c savedir.h \
  sha.c sha.h \
  stdio-safer.h \
  stripslash.c \
  strverscmp.h \
  unicodeio.c unicodeio.h \
  unistd-safer.h \
  userspec.c \
  version-etc.c version-etc.h \
  xalloc.h \
  xgetcwd.c \
  xgethostname.c \
  xmalloc.c \
  xstrdup.c \
  xstrtod.c xstrtod.h \
  xstrtol.c xstrtol.h \
  xstrtoul.c \
  xstrtoumax.c \
  yesno.c

libfetish_a_LIBADD = @LIBOBJS@ @ALLOCA@
libfetish_a_DEPENDENCIES = $(libfetish_a_LIBADD)


BUILT_SOURCES = getdate.c lstat.c stat.c unlocked-io.h
MAINTAINERCLEANFILES = $(BUILT_SOURCES)
DISTCLEANFILES = lstat.c stat.c unlocked-io.h

EXTRA_DIST = xstat.in config.charset ref-add.sin ref-del.sin unlocked-io.hin
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
# On systems with glibc-2.1 or newer, the file is redundant, therefore we
# avoid installing it.

all-local: charset.alias ref-add.sed ref-del.sed lstat.c stat.c unlocked-io.h

charset_alias = $(DESTDIR)$(libdir)/charset.alias
charset_tmp = $(DESTDIR)$(libdir)/charset.tmp
install-exec-local: all-local
	$(mkinstalldirs) $(DESTDIR)$(libdir)
	if test -f $(charset_alias); then \
	  sed -f ref-add.sed $(charset_alias) > $(charset_tmp) ; \
	  $(INSTALL_DATA) $(charset_tmp) $(charset_alias) ; \
	  rm -f $(charset_tmp) ; \
	else \
	  if test @GLIBC21@ = no; then \
	    sed -f ref-add.sed charset.alias > $(charset_tmp) ; \
	    $(INSTALL_DATA) $(charset_tmp) $(charset_alias) ; \
	    rm -f $(charset_tmp) ; \
	  fi ; \
	fi

uninstall-local: all-local
	if test -f $(charset_alias); then \
	  sed -f ref-del.sed $(charset_alias) > $(charset_tmp); \
	  if grep '^# Packages using this file: $$' $(charset_tmp) \
	      > /dev/null; then \
	    rm -f $(charset_alias); \
	  else \
	    $(INSTALL_DATA) $(charset_tmp) $(charset_alias); \
	  fi; \
	  rm -f $(charset_tmp); \
	fi

charset.alias: config.charset
	$(SHELL) $(srcdir)/config.charset '@host@' > t-$@
	mv t-$@ $@

SUFFIXES = .sed .sin
.sin.sed:
	sed -e '/^#/d' -e 's/@''PACKAGE''@/@PACKAGE@/g' $< > $@-t
	mv $@-t $@

CLEANFILES = charset.alias ref-add.sed ref-del.sed

###############################################

# FIXME: CAUTION this list is a duplicate of one in ../Makefile.cfg.
io_functions = \
  clearerr feof ferror fflush fgets fputc fputs \
  fread fwrite getc getchar putc putchar

a_z=abcdefghijklmnopqrstuvwxyz
A_Z=ABCDEFGHIJKLMNOPQRSTUVWXYZ

unlocked-io.h: $(srcdir)/unlocked-io.hin Makefile.am
	tmp=t$$$$;							\
	echo						> $$tmp;	\
	for b in $(io_functions); do					\
	  f=$${b}_unlocked;						\
	  u=`echo $$f|tr $(a_z) $(A_Z)`;				\
	  echo "#  if HAVE_$${u}_UNLOCKED"		>> $$tmp;	\
	  echo "#   undef $$f"				>> $$tmp;	\
	  echo "#   define $$f(S) $${f}_unlocked (S)"	>> $$tmp;	\
	  echo '#endif'					>> $$tmp;	\
	done;								\
	sed "/^@replace_this@$$/r$$tmp" $(srcdir)/unlocked-io.hin	\
	  | sed "/^@replace_this@$$/d"					\
	  > $@t;							\
	rm -f $$tmp;							\
	mv $@t $@
