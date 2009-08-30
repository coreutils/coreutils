## Process this file with automake to produce Makefile.in -*-Makefile-*-

## Copyright (C) 1990, 1991, 1993-2009 Free Software Foundation, Inc.

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
  chroot df hostid nice pinky stdbuf libstdbuf.so stty su uname uptime users who

AM_CFLAGS = $(WARN_CFLAGS) $(WERROR_CFLAGS)

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
  test timeout true truncate tty whoami yes \
  base64

bin_PROGRAMS = $(OPTIONAL_BIN_PROGS)

noinst_PROGRAMS = setuidgid getlimits

pkglib_PROGRAMS = $(OPTIONAL_PKGLIB_PROGS)

noinst_HEADERS = \
  chown-core.h \
  copy.h \
  cp-hash.h \
  dircolors.h \
  fs.h \
  group-list.h \
  ls.h \
  operand2sig.h \
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

# Also remove these sometimes-built programs.
# For example, even when excluded, they're built via sc_check-AUTHORS.
CLEANFILES += $(no_install__progs)

AM_CPPFLAGS = -I$(top_srcdir)/lib

noinst_LIBRARIES = libver.a
nodist_libver_a_SOURCES = version.c version.h

# Sometimes, the expansion of $(LIBINTL) includes -lc which may
# include modules defining variables like `optind', so libcoreutils.a
# must precede $(LIBINTL) in order to ensure we use GNU getopt.
# But libcoreutils.a must also follow $(LIBINTL), since libintl uses
# replacement functions defined in libcoreutils.a.
LDADD = libver.a ../lib/libcoreutils.a $(LIBINTL) ../lib/libcoreutils.a

cat_LDADD = $(LDADD)
df_LDADD = $(LDADD)
du_LDADD = $(LDADD)
getlimits_LDADD = $(LDADD)
ptx_LDADD = $(LDADD)
split_LDADD = $(LDADD)
stdbuf_LDADD = $(LDADD)
timeout_LDADD = $(LDADD)
truncate_LDADD = $(LDADD)

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
dir_LDADD = $(LDADD) $(LIB_CLOCK_GETTIME) $(LIB_SELINUX) $(LIB_CAP)
id_LDADD = $(LDADD) $(LIB_SELINUX)
ls_LDADD = $(LDADD) $(LIB_CLOCK_GETTIME) $(LIB_SELINUX) $(LIB_CAP)
mktemp_LDADD = $(LDADD) $(LIB_GETHRXTIME)
pr_LDADD = $(LDADD) $(LIB_CLOCK_GETTIME)
shred_LDADD = $(LDADD) $(LIB_GETHRXTIME) $(LIB_FDATASYNC)
shuf_LDADD = $(LDADD) $(LIB_GETHRXTIME)
tac_LDADD = $(LDADD) $(LIB_CLOCK_GETTIME)
vdir_LDADD = $(LDADD) $(LIB_CLOCK_GETTIME) $(LIB_SELINUX) $(LIB_CAP)

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

# for various GMP functions
expr_LDADD = $(LDADD) $(LIB_GMP)

# for various GMP functions
factor_LDADD = $(LDADD) $(LIB_GMP)

sleep_LDADD = $(nanosec_libs)
tail_LDADD = $(nanosec_libs)

# If necessary, add -lm to resolve use of pow in lib/strtod.c.
uptime_LDADD = $(LDADD) $(POW_LIB) $(GETLOADAVG_LIBS)

su_LDADD = $(LDADD) $(LIB_CRYPT)

dir_LDADD += $(LIB_ACL)
ls_LDADD += $(LIB_ACL)
vdir_LDADD += $(LIB_ACL)
cp_LDADD += $(LIB_ACL) $(LIB_XATTR)
mv_LDADD += $(LIB_ACL) $(LIB_XATTR)
ginstall_LDADD += $(LIB_ACL) $(LIB_XATTR)

stat_LDADD = $(LDADD) $(LIB_SELINUX)

# Append $(LIBICONV) to each program that uses proper_name_utf8.
cat_LDADD += $(LIBICONV)
cp_LDADD += $(LIBICONV)
df_LDADD += $(LIBICONV)
du_LDADD += $(LIBICONV)
getlimits_LDADD += $(LIBICONV)
ptx_LDADD += $(LIBICONV)
split_LDADD += $(LIBICONV)
stdbuf_LDADD += $(LIBICONV)
timeout_LDADD += $(LIBICONV)
truncate_LDADD += $(LIBICONV)

# programs that use getaddrinfo (e.g., via canon_host)
pinky_LDADD = $(LDADD) $(GETADDRINFO_LIB)
who_LDADD = $(LDADD) $(GETADDRINFO_LIB)

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

transform = s/ginstall/install/; $(program_transform_name)
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
kill_SOURCES = kill.c operand2sig.c
timeout_SOURCES = timeout.c operand2sig.c

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

ginstall_CPPFLAGS = -DENABLE_MATCHPATHCON=1 $(AM_CPPFLAGS)

# Ensure we don't link against libcoreutils.a as that lib is
# not compiled with -fPIC which causes issues on 64 bit at least
libstdbuf_so_LDADD =

# Note libstdbuf is only compiled if GCC is available
# (as per the check in configure.ac), so these flags should be available.
# libtool is probably required to relax this dependency.
libstdbuf_so_LDFLAGS = -shared
libstdbuf_so_CFLAGS = -fPIC $(AM_CFLAGS)

editpl = sed -e 's,@''PERL''@,$(PERL),g'

BUILT_SOURCES += dircolors.h
dircolors.h: dcgen dircolors.hin
	$(AM_V_GEN)rm -f $@ $@-t
	$(AM_V_at)$(PERL) -w -- $(srcdir)/dcgen $(srcdir)/dircolors.hin > $@-t
	$(AM_V_at)chmod a-w $@-t
	$(AM_V_at)mv $@-t $@

wheel_size = 5

BUILT_SOURCES += wheel-size.h
wheel-size.h: Makefile.am
	$(AM_V_GEN)rm -f $@ $@-t
	$(AM_V_at)echo '#define WHEEL_SIZE $(wheel_size)' > $@-t
	$(AM_V_at)chmod a-w $@-t
	$(AM_V_at)mv $@-t $@

BUILT_SOURCES += wheel.h
wheel.h: wheel-gen.pl Makefile.am
	$(AM_V_GEN)rm -f $@ $@-t
	$(AM_V_at)$(srcdir)/wheel-gen.pl $(wheel_size) > $@-t
	$(AM_V_at)chmod a-w $@-t
	$(AM_V_at)mv $@-t $@

# false exits nonzero even with --help or --version.
# test doesn't support --help or --version.
# Tell automake to exempt then from that installcheck test.
AM_INSTALLCHECK_STD_OPTIONS_EXEMPT = false test

BUILT_SOURCES += fs.h
fs.h: stat.c extract-magic
	$(AM_V_GEN)rm -f $@
	$(AM_V_at)$(PERL) $(srcdir)/extract-magic $(srcdir)/stat.c > $@t
	$(AM_V_at)chmod a-w $@t
	$(AM_V_at)mv $@t $@

BUILT_SOURCES += version.c
version.c: Makefile
	$(AM_V_GEN)rm -f $@
	$(AM_V_at)printf '#include <config.h>\n' > $@t
	$(AM_V_at)printf 'char const *Version = "$(PACKAGE_VERSION)";\n' >> $@t
	$(AM_V_at)chmod a-w $@t
	$(AM_V_at)mv $@t $@

BUILT_SOURCES += version.h
version.h: Makefile
	$(AM_V_GEN)rm -f $@
	$(AM_V_at)printf 'extern char const *Version;\n' > $@t
	$(AM_V_at)chmod a-w $@t
	$(AM_V_at)mv $@t $@

DISTCLEANFILES = version.c version.h
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

# This is required because we have broken inter-directory dependencies:
# in order to generate all man pages, even those for which we don't
# install a binary, require that all programs be built at distribution time.
dist-hook: $(all_programs)

pm = progs-makefile
pr = progs-readme
# Ensure that the list of programs in README matches the list
# of programs we can build.
check: check-README check-duplicate-no-install
.PHONY: check-README
check-README:
	$(AM_V_GEN)rm -rf $(pr) $(pm)
	$(AM_V_at)echo $(all_programs) \
	 | tr -s ' ' '\n' | sed -e 's,$(EXEEXT)$$,,;s/ginstall/install/' \
	 | sed /libstdbuf/d \
	 | $(ASSORT) -u > $(pm) && \
	sed -n '/^The programs .* are:/,/^[a-zA-Z]/p' $(top_srcdir)/README \
	  | sed -n '/^   */s///p' | tr -s ' ' '\n' > $(pr)
	$(AM_V_at)diff $(pm) $(pr) && rm -rf $(pr) $(pm)

# Ensure that a by-default-not-installed program (listed in
# $(no_install__progs) is not also listed in $(EXTRA_PROGRAMS), because
# if that were to happen, it *would* be installed by default.
.PHONY: check-duplicate-no-install
check-duplicate-no-install: tr
	$(AM_V_GEN)test -z "`echo '$(EXTRA_PROGRAMS)'| ./tr ' ' '\n' | uniq -d`"

# Ensure that the list of programs and author names is accurate.
# We need a UTF8 locale.  If a lack of locale support or a missing
# translation inhibits printing of UTF-8 names, just skip this test.
au_dotdot = authors-dotdot
au_actual = authors-actual
.PHONY: sc_check-AUTHORS
sc_check-AUTHORS: $(all_programs)
	$(AM_V_GEN)locale=en_US.UTF-8;			\
	LC_ALL="$$locale" ./cat --version		\
	    | grep ' Torbjorn '	> /dev/null		\
	  && { echo "$@: skipping this check"; exit 0; }; \
	rm -f $(au_actual) $(au_dotdot);		\
	for i in `ls $(all_programs) | sed -e 's,$(EXEEXT)$$,,' \
	    | sed /libstdbuf/d				\
	    | $(ASSORT) -u`; do				\
	  test "$$i" = '[' && continue;			\
	  exe=$$i;					\
	  if test "$$i" = install; then			\
	    exe=ginstall;				\
	  elif test "$$i" = test; then			\
	    exe='[';					\
	  fi;						\
	  LC_ALL="$$locale" ./$$exe --version		\
	    | perl -0 -pi -e 's/,\n/, /gm'		\
	    | sed -n -e '/Written by /{ s//'"$$i"': /;'	\
		  -e 's/,* and /, /; s/\.$$//; p; }';	\
	done > $(au_actual) &&				\
	sed -n '/^[^ ][^ ]*:/p' $(top_srcdir)/AUTHORS > $(au_dotdot) && \
	diff $(au_actual) $(au_dotdot) && rm -f $(au_actual) $(au_dotdot)

# The following rule is not designed to be portable,
# and relies on tools that not everyone has.

# Most functions in src/*.c should have static scope.
# Any that don't must be marked with `extern', but `main'
# and `usage' are exceptions.  They're always extern, but
# don't need to be marked. Also functions starting with __
# are exempted due to possibly being added by the compiler
# (when compiled as a shared library for example).
#
# The second nm|grep checks for file-scope variables with `extern' scope.
.PHONY: sc_tight_scope
sc_tight_scope: $(bin_PROGRAMS)
	$(AM_V_GEN)t=exceptions-$$$$;					\
	trap "s=$$?; rm -f $$t; exit $$s" 0 1 2 13 15;			\
	src=`for f in $(SOURCES); do					\
	       test -f $$f && d= || d=$(srcdir)/; echo $$d$$f; done`;	\
	hdr=`for f in $(noinst_HEADERS); do				\
	       test -f $$f && d= || d=$(srcdir)/; echo $$d$$f; done`;	\
	( printf 'main\nusage\n_.*\n';					\
	  grep -h -A1 '^extern .*[^;]$$' $$src				\
	    | grep -vE '^(extern |--)' | sed 's/ .*//';			\
	  perl -ne '/^extern \S+ (\S*) \(/ and print "$$1\n"' $$hdr;	\
	) | $(ASSORT) -u | sed 's/^/^/;s/$$/$$/' > $$t;			\
	nm -e *.$(OBJEXT)						\
	    | sed -n 's/.* T //p'					\
	    | sed 's/^_//'						\
	    | grep -Ev -f $$t &&					\
	  { echo 'the above functions should have static scope' 1>&2;	\
	    exit 1; } || : ;						\
	( printf '^program_name$$\n';					\
	  perl -ne '/^extern .*?\**(\w+);/ and print "^$$1\$$\n"'	\
	    $$hdr *.h ) | $(ASSORT) -u > $$t;				\
	nm -e *.$(OBJEXT)						\
	    | sed -n 's/.* [BD] //p'					\
	    | sed 's/^_//'						\
	    | grep -Ev -f $$t &&					\
	  { echo 'the above variables should have static scope' 1>&2;	\
	    exit 1; } || :

# Use the just-built ./ginstall, when not cross-compiling.
if CROSS_COMPILING
cu_install_program = @INSTALL_PROGRAM@
else
cu_install_program = ./ginstall
endif
INSTALL_PROGRAM = $(cu_install_program)
