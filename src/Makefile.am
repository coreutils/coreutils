## Process this file with automake to produce Makefile.in -*-Makefile-*-

## Copyright (C) 1990, 1991, 1993-2008 Free Software Foundation, Inc.

## This program is free software: you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation, either version 3 of the License, or
## (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program.  If not, see <http://www.gnu.org/licenses/>.

# These are the names of programs that are not installed by default.
# This list is *not* intended for programs like who, nice, chroot, etc.,
# that are built only when certain requisite system features are detected.
# Hence, if you want to install programs from this list anyway, say A and B,
# use --enable-install-program=A,B
no_install__progs = \
  arch hostname su

build_if_possible__progs = \
  chroot df hostid nice pinky stty su uname uptime users who

EXTRA_PROGRAMS = \
  $(no_install__progs) \
  $(build_if_possible__progs) \
  [ chcon chgrp chown chmod cp dd dircolors du \
  ginstall link ln dir vdir ls mkdir \
  mkfifo mknod mktemp \
  mv nohup readlink rm rmdir shred stat sync touch unlink \
  cat cksum comm csplit cut expand fmt fold head join groups md5sum \
  nl od paste pr ptx sha1sum sha224sum sha256sum sha384sum sha512sum \
  shuf sort split sum tac tail tr tsort unexpand uniq wc \
  basename date dirname echo env expr factor false \
  id kill logname pathchk printenv printf pwd \
  runcon seq sleep tee \
  test true tty whoami yes \
  base64

bin_PROGRAMS = $(OPTIONAL_BIN_PROGS)

noinst_PROGRAMS = setuidgid

noinst_HEADERS = \
  chown-core.h \
  copy.h \
  cp-hash.h \
  dircolors.h \
  fs.h \
  group-list.h \
  ls.h \
  prog-fprintf.h \
  remove.h \
  system.h \
  wheel-size.h \
  wheel.h \
  uname.h

EXTRA_DIST = dcgen dircolors.hin tac-pipe.c \
  wheel-gen.pl extract-magic c99-to-c89.diff
BUILT_SOURCES =
CLEANFILES = $(SCRIPTS) su

AM_CPPFLAGS = -I$(top_srcdir)/lib

# Sometimes, the expansion of $(LIBINTL) includes -lc which may
# include modules defining variables like `optind', so libcoreutils.a
# must precede $(LIBINTL) in order to ensure we use GNU getopt.
# But libcoreutils.a must also follow $(LIBINTL), since libintl uses
# replacement functions defined in libcoreutils.a.
LDADD = ../lib/libcoreutils.a $(LIBINTL) ../lib/libcoreutils.a

# for eaccess in lib/euidaccess.c.
chcon_LDADD = $(LDADD) $(LIB_SELINUX)
cp_LDADD = $(LDADD) $(LIB_EACCESS) $(LIB_SELINUX)
ginstall_LDADD = $(LDADD) $(LIB_EACCESS) $(LIB_SELINUX)
mkdir_LDADD = $(LDADD) $(LIB_SELINUX)
mkfifo_LDADD = $(LDADD) $(LIB_SELINUX)
mknod_LDADD = $(LDADD) $(LIB_SELINUX)
mv_LDADD = $(LDADD) $(LIB_EACCESS) $(LIB_SELINUX)
runcon_LDADD = $(LDADD) $(LIB_SELINUX)
pathchk_LDADD = $(LDADD) $(LIB_EACCESS)
rm_LDADD = $(LDADD) $(LIB_EACCESS)
test_LDADD = $(LDADD) $(LIB_EACCESS)
# This is for the '[' program.  Automake transliterates '[' to '_'.
__LDADD = $(LDADD) $(LIB_EACCESS)

# for clock_gettime and fdatasync
dd_LDADD = $(LDADD) $(LIB_GETHRXTIME) $(LIB_FDATASYNC)
dir_LDADD = $(LDADD) $(LIB_CLOCK_GETTIME) $(LIB_SELINUX)
id_LDADD = $(LDADD) $(LIB_SELINUX)
ls_LDADD = $(LDADD) $(LIB_CLOCK_GETTIME) $(LIB_SELINUX)
mktemp_LDADD = $(LDADD) $(LIB_GETHRXTIME)
pr_LDADD = $(LDADD) $(LIB_CLOCK_GETTIME)
shred_LDADD = $(LDADD) $(LIB_GETHRXTIME) $(LIB_FDATASYNC)
shuf_LDADD = $(LDADD) $(LIB_GETHRXTIME)
tac_LDADD = $(LDADD) $(LIB_CLOCK_GETTIME)
vdir_LDADD = $(LDADD) $(LIB_CLOCK_GETTIME) $(LIB_SELINUX)

## If necessary, add -lm to resolve use of pow in lib/strtod.c.
sort_LDADD = $(LDADD) $(POW_LIB) $(LIB_GETHRXTIME)

# for get_date and gettime
date_LDADD = $(LDADD) $(LIB_CLOCK_GETTIME)
touch_LDADD = $(LDADD) $(LIB_CLOCK_GETTIME)

# If necessary, add -lm to resolve use of pow in lib/strtod.c.
# If necessary, add -liconv to resolve use of iconv in lib/unicodeio.c.
printf_LDADD = $(LDADD) $(POW_LIB) $(LIBICONV)

# If necessary, add -lm to resolve use of pow in lib/strtod.c.
seq_LDADD = $(LDADD) $(POW_LIB)

# If necessary, add libraries to resolve the `pow' reference in lib/strtod.c
# and the `nanosleep' reference in lib/xnanosleep.c.
nanosec_libs = $(LDADD) $(POW_LIB) $(LIB_NANOSLEEP)

sleep_LDADD = $(nanosec_libs)
tail_LDADD = $(nanosec_libs)

# If necessary, add -lm to resolve use of pow in lib/strtod.c.
uptime_LDADD = $(LDADD) $(POW_LIB) $(GETLOADAVG_LIBS)

su_LDADD = $(LDADD) $(LIB_CRYPT)

dir_LDADD += $(LIB_ACL)
ls_LDADD += $(LIB_ACL)
vdir_LDADD += $(LIB_ACL)
cp_LDADD += $(LIB_ACL)
mv_LDADD += $(LIB_ACL)
ginstall_LDADD += $(LIB_ACL)

stat_LDADD = $(LDADD) $(LIB_SELINUX)

$(PROGRAMS): ../lib/libcoreutils.a

# Get the release year from ../lib/version-etc.c.
RELEASE_YEAR = \
  `sed -n '/.*COPYRIGHT_YEAR = \([0-9][0-9][0-9][0-9]\) };/s//\1/p' \
    $(top_srcdir)/lib/version-etc.c`

all-local: su$(EXEEXT)

installed_su = $(DESTDIR)$(bindir)/`echo su|sed '$(transform)'`

setuid_root_mode = a=rx,u+s

install_su =						\
  if test "$(INSTALL_SU)" = yes; then			\
    p=su;						\
    echo " $(INSTALL_PROGRAM) $$p $(installed_su)";	\
    $(INSTALL_PROGRAM) $$p $(installed_su);		\
    echo " chown root $(installed_su)";			\
    chown root $(installed_su);				\
    echo " chmod $(setuid_root_mode) $(installed_su)";	\
    chmod $(setuid_root_mode) $(installed_su);		\
  else							\
    :;							\
  fi

install-root: su$(EXEEXT)
	@$(install_su)

install-exec-hook: su$(EXEEXT)
	@if test "$(INSTALL_SU)" = yes; then			\
	  TMPFILE=$(DESTDIR)$(bindir)/.su-$$$$;			\
	  rm -f $$TMPFILE;					\
	  echo > $$TMPFILE;					\
## See if we can create a setuid root executable in $(bindir).
## If not, then don't even try to install su.
	  can_create_suid_root_executable=no;			\
	  chown root $$TMPFILE > /dev/null 2>&1			\
	    && chmod $(setuid_root_mode) $$TMPFILE > /dev/null 2>&1 \
	    && can_create_suid_root_executable=yes;		\
	  rm -f $$TMPFILE;					\
	  if test $$can_create_suid_root_executable = yes; then \
	    $(install_su);					\
	  else							\
	    echo "WARNING: insufficient access; not installing su"; \
	    echo "NOTE: to install su, run 'make install-root' as root"; \
	    rm -f $(installed_su);				\
	  fi;							\
	else :;							\
	fi

uninstall-local:
# Remove su only if it's one we installed.
	@if test "$(INSTALL_SU)" = yes; then			\
	  if grep '$(PACKAGE_NAME)' $(installed_su) > /dev/null 2>&1; then \
	    echo "  rm -f $(installed_su)"; \
	    rm -f $(installed_su); \
	  else :; \
	  fi; \
	fi

copy_sources = copy.c cp-hash.c

# Use `ginstall' in the definition of PROGRAMS and in dependencies to avoid
# confusion with the `install' target.  The install rule transforms `ginstall'
# to install before applying any user-specified name transformations.

transform = s/ginstall/install/; @program_transform_name@
ginstall_SOURCES = install.c prog-fprintf.c $(copy_sources)

# This is for the '[' program.  Automake transliterates '[' to '_'.
__SOURCES = lbracket.c

cp_SOURCES = cp.c $(copy_sources)
dir_SOURCES = ls.c ls-dir.c
vdir_SOURCES = ls.c ls-vdir.c
id_SOURCES = id.c group-list.c
groups_SOURCES = groups.c group-list.c
ln_SOURCES = ln.c
ls_SOURCES = ls.c ls-ls.c
chown_SOURCES = chown.c chown-core.c
chgrp_SOURCES = chgrp.c chown-core.c

mv_SOURCES = mv.c remove.c $(copy_sources)
rm_SOURCES = rm.c remove.c

mkdir_SOURCES = mkdir.c prog-fprintf.c
rmdir_SOURCES = rmdir.c prog-fprintf.c

uname_SOURCES = uname.c uname-uname.c
arch_SOURCES = uname.c uname-arch.c

md5sum_SOURCES = md5sum.c
md5sum_CPPFLAGS = -DHASH_ALGO_MD5=1 $(AM_CPPFLAGS)
sha1sum_SOURCES = md5sum.c
sha1sum_CPPFLAGS = -DHASH_ALGO_SHA1=1 $(AM_CPPFLAGS)
sha224sum_SOURCES = md5sum.c
sha224sum_CPPFLAGS = -DHASH_ALGO_SHA224=1 $(AM_CPPFLAGS)
sha256sum_SOURCES = md5sum.c
sha256sum_CPPFLAGS = -DHASH_ALGO_SHA256=1 $(AM_CPPFLAGS)
sha384sum_SOURCES = md5sum.c
sha384sum_CPPFLAGS = -DHASH_ALGO_SHA384=1 $(AM_CPPFLAGS)
sha512sum_SOURCES = md5sum.c
sha512sum_CPPFLAGS = -DHASH_ALGO_SHA512=1 $(AM_CPPFLAGS)

editpl = sed -e 's,@''PERL''@,$(PERL),g'

BUILT_SOURCES += dircolors.h
dircolors.h: dcgen dircolors.hin
	@rm -f $@ $@-t
	$(PERL) -w -- $(srcdir)/dcgen $(srcdir)/dircolors.hin > $@-t
	@chmod a-w $@-t
	mv $@-t $@

wheel_size = 5

BUILT_SOURCES += wheel-size.h
wheel-size.h: Makefile.am
	@rm -f $@ $@-t
	echo '#define WHEEL_SIZE $(wheel_size)' > $@-t
	@chmod a-w $@-t
	mv $@-t $@

BUILT_SOURCES += wheel.h
wheel.h: wheel-gen.pl Makefile.am
	@rm -f $@ $@-t
	$(srcdir)/wheel-gen.pl $(wheel_size) > $@-t
	@chmod a-w $@-t
	mv $@-t $@

# false exits nonzero even with --help or --version.
# test doesn't support --help or --version.
# Tell automake to exempt then from that installcheck test.
AM_INSTALLCHECK_STD_OPTIONS_EXEMPT = false test

BUILT_SOURCES += fs.h
fs.h: stat.c extract-magic
	rm -f $@
	$(PERL) $(srcdir)/extract-magic $(srcdir)/stat.c > $@t
	@chmod a-w $@t
	mv $@t $@

MAINTAINERCLEANFILES = $(BUILT_SOURCES)

# Sort in traditional ASCII order, regardless of the current locale;
# otherwise we may get into trouble with distinct strings that the
# current locale considers to be equal.
ASSORT = LC_ALL=C sort

all_programs = \
    $(bin_PROGRAMS) \
    $(bin_SCRIPTS) \
  $(EXTRA_PROGRAMS)

built_programs.list:
	@echo $(bin_PROGRAMS) $(bin_SCRIPTS) | tr ' ' '\n' \
	  | sed -e 's,$(EXEEXT)$$,,' | $(ASSORT) -u | tr '\n' ' '

all_programs.list:
	@echo $(all_programs) | tr ' ' '\n' | sed -e 's,$(EXEEXT)$$,,' \
	  | $(ASSORT) -u

pm = progs-makefile
pr = progs-readme
# Ensure that the list of programs in README matches the list
# of programs we can build.
check: check-README check-misc check-duplicate-no-install
.PHONY: check-README
check-README:
	rm -rf $(pr) $(pm)
	echo $(all_programs) \
	 | tr -s ' ' '\n' | sed -e 's,$(EXEEXT)$$,,;s/ginstall/install/' \
	 | $(ASSORT) -u > $(pm) && \
	sed -n '/^The programs .* are:/,/^[a-zA-Z]/p' $(top_srcdir)/README \
	  | sed -n '/^   */s///p' | tr -s ' ' '\n' > $(pr)
	diff $(pm) $(pr) && rm -rf $(pr) $(pm)

# Ensure that a by-default-not-installed program (listed in
# $(no_install__progs) is not also listed in $(EXTRA_PROGRAMS), because
# if that were to happen, it *would* be installed by default.
.PHONY: check-duplicate-no-install
check-duplicate-no-install: tr
	test -z "`echo '$(EXTRA_PROGRAMS)'| ./tr ' ' '\n' | uniq -d`"

# Ensure that the list of programs and author names is accurate.
au_dotdot = authors-dotdot
au_actual = authors-actual
.PHONY: check-AUTHORS
check-AUTHORS: $(all_programs)
	rm -f $(au_actual) $(au_dotdot)
	for i in `ls $(all_programs) | sed -e 's,$(EXEEXT)$$,,' \
	    | $(ASSORT) -u`; do				\
	  test "$$i" = '[' && continue;			\
	  exe=$$i;					\
	  if test "$$i" = install; then			\
	    exe=ginstall;				\
	  elif test "$$i" = test; then			\
	    exe='[';					\
	  fi;						\
	  ./$$exe --version				\
	    |sed -n '/Written by /{ s//'"$$i"': /; s/,* and /, /; s/\.$$//; p; }'; \
	done > $(au_actual)
	sed -n '/:/p' $(top_srcdir)/AUTHORS > $(au_dotdot)
	diff $(au_actual) $(au_dotdot) && rm -f $(au_actual) $(au_dotdot)

# Make sure we don't define any S_IS* macros in src/*.c files.
# Not a big deal, but they're already defined via system.h.
#
# Also make sure we don't use st_blocks.  Use ST_NBLOCKS instead.
# This is a bit of a kludge, since it prevents use of the string
# even in comments, but for now it does the job with no false positives.
.PHONY: check-misc
check-misc:
	cd $(srcdir); grep '^# *define  *S_IS' $(SOURCES) && exit 1 || :
	cd $(srcdir); grep st_blocks $(SOURCES) && exit 1 || :
	cd $(srcdir); grep '^# *define .*defined' $(SOURCES) && exit 1 || :

# Extract the list of authors from each file.
sed_filter = s/^ *//;s/N_ (//;s/^"//;s/")*$$//
# Sometimes the string is on the same line as the #define...
s1 = '/^\#define AUTHORS \([^\\]\)/{;s//\1/;$(sed_filter);p;q;}'
# Sometimes the string is on the backslash-continued line after the #define.
s2 = '/^\#define AUTHORS \\\\/{;n;$(sed_filter);p;q;}'
# FIXME: handle *.sh;  and use $(all_programs), not $(SOURCES)
../AUTHORS: $(SOURCES)
	rm -f $@-t
	(						\
	  set -e;					\
	  echo "Here are the names of the programs in this package,";	\
	  echo "each followed by the name(s) of its author(s).";	\
	  echo;						\
	  for i in $(SOURCES); do			\
	    a=`sed -n $(s1) $$i`;			\
	    test "$$a" && :				\
	      || a=`sed -n $(s2) $$i`;			\
	    if test "$$a"; then				\
	      prog=`echo $$i|sed 's/\.c$$//'`;		\
	      echo "$$prog: $$a";			\
	    fi;						\
	  done | $(ASSORT) -u ) > $@-t
	chmod a-w $@-t
	mv $@-t $@

# The following rule is not designed to be portable,
# and relies on tools that not everyone has.

# Most functions in src/*.c should have static scope.
# Any that don't must be marked with `extern', but `main'
# and `usage' are exceptions.  They're always extern, but
# don't need to be marked.
#
# The second nm|grep checks for file-scope variables with `extern' scope.
.PHONY: sc_tight_scope
sc_tight_scope: $(all_programs)
	@t=exceptions-$$$$;						\
	trap "s=$$?; rm -f $$t; exit $$s" 0 1 2 13 15;			\
	( printf 'main\nusage\n';					\
	  grep -h -A1 '^extern .*[^;]$$' $(SOURCES)			\
	    | grep -vE '^(extern |--)' | sed 's/ .*//';			\
	  perl -ne '/^extern \S+ (\S*) \(/ and print "$$1\n"'		\
	    $(noinst_HEADERS);						\
	) | $(ASSORT) -u | sed 's/^/^/;s/$$/$$/' > $$t;			\
	nm -e *.$(OBJEXT)						\
	    | sed -n 's/.* T //p'					\
	    | grep -Ev -f $$t &&					\
	  { echo 'the above functions should have static scope' 1>&2;	\
	    exit 1; } || : ;						\
	( printf '^program_name$$\n';					\
	  sed -n 's/^extern int \([^ ][^ ]*\);$$/^\1$$/p'		\
	    $(noinst_HEADERS) ) > $$t;					\
	nm -e *.$(OBJEXT)						\
	    | sed -n 's/.* [BD] //p'					\
	    | grep -Ev -f $$t &&					\
	  { echo 'the above variables should have static scope' 1>&2;	\
	    exit 1; } || :
