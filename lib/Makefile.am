## Makefile for gnulib/lib				-*-Makefile-*-

# Copyright (C) 1995-2005 Free Software Foundation, Inc.

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
## Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
## 02110-1301, USA.

noinst_LIBRARIES = libcoreutils.a

noinst_PROGRAMS = t-fpending
LDADD = $(noinst_LIBRARIES)

AM_CPPFLAGS = -I.. -I$(srcdir)
DEFS += -DLIBDIR=\"$(libdir)\"

libcoreutils_a_SOURCES = \
  allocsa.c allocsa.h \
  euidaccess.h \
  exit.h \
  fprintftime.c fprintftime.h \
  full-read.c full-read.h \
  full-write.c full-write.h \
  getaddrinfo.h \
  gettext.h \
  localcharset.c localcharset.h \
  mbchar.h \
  mbswidth.c mbswidth.h \
  mbuiter.h \
  readtokens0.c readtokens0.h \
  strcase.h \
  strnlen1.c strnlen1.h \
  strstr.h \
  time_r.c time_r.h \
  unicodeio.c unicodeio.h \
  verify.h \
  xalloc-die.c \
  xgethostname.c xgethostname.h \
  xmemcoll.c xmemcoll.h \
  xstrndup.c xstrndup.h \
  xstrtoimax.c \
  xstrtoumax.c

libcoreutils_a_SOURCES += \
  printf-args.h \
  printf-parse.h \
  vasprintf.h \
  vasnprintf.h

libcoreutils_a_LIBADD = $(LIBOBJS) $(ALLOCA)

lib_OBJECTS = $(libcoreutils_a_OBJECTS)


BUILT_SOURCES = getdate.c
CLEANFILES =
MAINTAINERCLEANFILES = $(BUILT_SOURCES)
MOSTLYCLEANFILES =
SUFFIXES =

EXTRA_DIST = getdate.c config.charset ref-add.sin ref-del.sin \
  getndelim2.c getndelim2.h

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

BUILT_SOURCES += $(GETOPT_H)
EXTRA_DIST += getopt_.h getopt_int.h

check: unit-test

.PHONY: unit-test
unit-test: t-fpending
	./t-fpending > /dev/null

# We need the following in order to create an <getopt.h> when the system
# doesn't have one that works with the given compiler.
all-local $(lib_OBJECTS): $(GETOPT_H)
getopt.h: getopt_.h
	cp $(srcdir)/getopt_.h $@-t
	mv $@-t $@
MOSTLYCLEANFILES += getopt.h getopt.h-t
