## Makefile for gnulib/lib				-*-Makefile-*-

# Copyright (C) 1995-2004 Free Software Foundation, Inc.

## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 2, or (at your option)
## any later version.

## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.

## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
## 02111-1307, USA.

noinst_LIBRARIES = libfetish.a

AM_CPPFLAGS = -I.. -I$(srcdir)
DEFS += -DLIBDIR=\"$(libdir)\"

## Put relatively complex files at the beginning of the list so
## that parallel compiles finish a tiny bit sooner.  I don't see
## a way to make regex.c appear earlier in the list, since it's
## added by automake, but on systems with an up to date GNU libc,
## regex.c isn't even compiled.
libfetish_a_SOURCES = \
  acl.h acl.c \
  getdate.h getdate.y \
  posixtm.c posixtm.h \
  posixver.c posixver.h \
  strftime.c strftime.h \
  getopt.c getopt.h getopt1.c \
  hash.c hash.h \
  hash-pjw.c hash-pjw.h \
  __fpending.h \
  addext.c \
  argmatch.c argmatch.h \
  backupfile.c backupfile.h \
  basename.c \
  c-strtod.c c-strtod.h \
  canon-host.c \
  canonicalize.h \
  cloexec.c cloexec.h \
  closeout.c closeout.h \
  cycle-check.c cycle-check.h \
  dev-ino.h \
  diacrit.c diacrit.h \
  dirname.c dirname.h \
  dirfd.h \
  dup-safer.c \
  euidaccess.h \
  exclude.c exclude.h \
  exit.h \
  exitfail.c exitfail.h \
  filemode.c filemode.h \
  file-type.c file-type.h \
  fopen-safer.c \
  fsusage.h \
  fts.c fts_.h \
  full-read.c full-read.h \
  full-write.c full-write.h \
  getline.h \
  getpagesize.h \
  gettime.c \
  gettext.h \
  getugroups.c \
  group-member.h \
  hard-locale.c hard-locale.h \
  human.c human.h \
  idcache.c \
  isdir.c \
  imaxtostr.c \
  lchown.h \
  linebuffer.c linebuffer.h \
  localcharset.c localcharset.h \
  long-options.c long-options.h \
  makepath.c makepath.h \
  mbswidth.c mbswidth.h \
  md5.c md5.h \
  memcasecmp.c memcasecmp.h \
  memcoll.c memcoll.h \
  modechange.c modechange.h \
  mountlist.h \
  offtostr.c \
  path-concat.c path-concat.h \
  pathmax.h \
  physmem.c physmem.h \
  quote.c quote.h \
  quotearg.c quotearg.h \
  readtokens.c readtokens.h \
  readutmp.h \
  regex.h \
  root-dev-ino.c root-dev-ino.h \
  safe-read.c safe-read.h \
  safe-write.c safe-write.h \
  same.c same.h \
  save-cwd.c save-cwd.h \
  savedir.c savedir.h \
  settime.c \
  sha1.c sha1.h \
  sig2str.h \
  stdio-safer.h \
  stripslash.c \
  strndup.h \
  strverscmp.h \
  time_r.c time_r.h \
  timespec.h \
  umaxtostr.c \
  unicodeio.c unicodeio.h \
  unistd-safer.h \
  unlocked-io.h \
  userspec.c userspec.h \
  utimens.c utimens.h \
  version-etc.c version-etc.h \
  xalloc.h \
  xfts.c xfts.h \
  xgetcwd.c xgetcwd.h \
  xgethostname.c xgethostname.h \
  xmalloc.c \
  xmemcoll.c xmemcoll.h \
  xnanosleep.c xnanosleep.h \
  xreadlink.c xreadlink.h \
  xstrdup.c \
  xstrndup.c xstrndup.h \
  xstrtod.c xstrtod.h \
  xstrtoimax.c \
  xstrtol.c xstrtol.h \
  xstrtoul.c \
  xstrtoumax.c \
  yesno.c

libfetish_a_SOURCES += \
  printf-args.h \
  printf-parse.h \
  vasprintf.h \
  vasnprintf.h

libfetish_a_LIBADD = $(LIBOBJS) $(ALLOCA)
libfetish_a_DEPENDENCIES = $(libfetish_a_LIBADD)

lib_OBJECTS = $(libfetish_a_OBJECTS)


BUILT_SOURCES = getdate.c
CLEANFILES =
MAINTAINERCLEANFILES = $(BUILT_SOURCES)
MOSTLYCLEANFILES =
SUFFIXES =

EXTRA_DIST = config.charset ref-add.sin ref-del.sin \
  getndelim2.c getndelim2.h \
  inttostr.c inttostr.h

# We need the following in order to install a simple file in $(libdir)
# which is shared with other installed packages. We use a list of referencing
# packages so that "make uninstall" will remove the file if and only if it
# is not used by another installed package.
# On systems with glibc-2.1 or newer, the file is redundant, therefore we
# avoid installing it.

all-local: charset.alias ref-add.sed ref-del.sed

charset_alias = $(DESTDIR)$(libdir)/charset.alias
charset_tmp = $(DESTDIR)$(libdir)/charset.tmp
install-exec-local: all-local
	test $(GLIBC21) != no || $(mkinstalldirs) $(DESTDIR)$(libdir)
	if test -f $(charset_alias); then \
	  sed -f ref-add.sed $(charset_alias) > $(charset_tmp) ; \
	  $(INSTALL_DATA) $(charset_tmp) $(charset_alias) ; \
	  rm -f $(charset_tmp) ; \
	else \
	  if test $(GLIBC21) = no; then \
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
	$(SHELL) $(srcdir)/config.charset '$(host)' > t-$@
	mv t-$@ $@

SUFFIXES += .sed .sin
.sin.sed:
	sed -e '/^#/d' -e 's/@''PACKAGE''@/$(PACKAGE)/g' $< > t-$@
	mv t-$@ $@

CLEANFILES += charset.alias ref-add.sed ref-del.sed

BUILT_SOURCES += $(STDBOOL_H)
EXTRA_DIST += stdbool_.h
MOSTLYCLEANFILES += stdbool.h stdbool.ht
# Create stdbool.h on systems that lack a working one.
stdbool.h: stdbool_.h
	sed -e 's/@''HAVE__BOOL''@/$(HAVE__BOOL)/g' $(srcdir)/stdbool_.h > $@t
	mv $@t $@

BUILT_SOURCES += $(ALLOCA_H)
EXTRA_DIST += alloca_.h

# We need the following in order to create an <alloca.h> when the system
# doesn't have one that works with the given compiler.
all-local $(lib_OBJECTS): $(ALLOCA_H)
alloca.h: alloca_.h
	cp $(srcdir)/alloca_.h $@-t
	mv $@-t $@
MOSTLYCLEANFILES += alloca.h alloca.h-t

BUILT_SOURCES += $(FNMATCH_H)
EXTRA_DIST += fnmatch_.h fnmatch_loop.c

# We need the following in order to create an <fnmatch.h> when the system
# doesn't have one that supports the required API.
all-local $(lib_OBJECTS): $(FNMATCH_H)
fnmatch.h: fnmatch_.h
	cp $(srcdir)/fnmatch_.h $@-t
	mv $@-t $@
MOSTLYCLEANFILES += fnmatch.h fnmatch.h-t
