## Makefile for gnulib/lib				-*-Makefile-*-

# Copyright (C) 1995, 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003 Free
# Software Foundation, Inc.

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

INCLUDES = -I.. -I$(srcdir)
DEFS = -DLIBDIR=\"$(libdir)\" @DEFS@

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
  bumpalloc.h \
  canon-host.c \
  canonicalize.h \
  closeout.c closeout.h \
  cycle-check.c cycle-check.h \
  dev-ino.h \
  diacrit.c diacrit.h \
  dirname.c dirname.h \
  dirfd.h \
  dup-safer.c \
  euidaccess.h \
  exclude.c exclude.h \
  exitfail.c exitfail.h \
  filemode.c filemode.h \
  file-type.c file-type.h \
  fnmatch_.h \
  fopen-safer.c \
  fsusage.h \
  ftw_.h \
  full-read.c full-read.h \
  full-write.c full-write.h \
  getline.h \
  getpagesize.h \
  getstr.c getstr.h \
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
  localcharset.c \
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
  safe-read.c safe-read.h \
  safe-write.c safe-write.h \
  same.c same.h \
  save-cwd.c save-cwd.h \
  savedir.c savedir.h \
  search_.h \
  settime.c \
  sha.c sha.h \
  sig2str.h \
  stdio-safer.h \
  stripslash.c \
  strverscmp.h \
  timespec.h \
  umaxtostr.c \
  unicodeio.c unicodeio.h \
  unistd-safer.h \
  unlocked-io.h \
  userspec.c \
  version-etc.c version-etc.h \
  xalloc.h \
  xgetcwd.c xgetcwd.h \
  xgethostname.c \
  xmalloc.c \
  xmemcoll.c xmemcoll.h \
  xnanosleep.c xnanosleep.h \
  xreadlink.c xreadlink.h \
  xstrdup.c \
  xstrtod.c xstrtod.h \
  xstrtol.c xstrtol.h \
  xstrtoul.c \
  xstrtoimax.c \
  xstrtoumax.c \
  yesno.c

libfetish_a_LIBADD = @LIBOBJS@ @ALLOCA@
libfetish_a_DEPENDENCIES = $(libfetish_a_LIBADD)


BUILT_SOURCES = getdate.c
MAINTAINERCLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST = config.charset ref-add.sin ref-del.sin \
  fnmatch_loop.c inttostr.c inttostr.h

# The following is needed in order to install a simple file in $(libdir)
# which is shared with other installed packages. We use a list of referencing
# packages so that "make uninstall" will remove the file if and only if it
# is not used by another installed package.
# On systems with glibc-2.1 or newer, the file is redundant, therefore we
# avoid installing it.

all-local: charset.alias ref-add.sed ref-del.sed

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
	sed -e '/^#/d' -e 's/@''PACKAGE''@/@PACKAGE@/g' $< > t-$@
	mv t-$@ $@

CLEANFILES = charset.alias ref-add.sed ref-del.sed
DISTCLEANFILES = ftw.h search.h fnmatch.h
