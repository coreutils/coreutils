## Process this file with automake to produce Makefile.in -*-Makefile-*-
AUTOMAKE_OPTIONS = ansi2knr

EXTRA_PROGRAMS = chroot df hostid nice pinky stty su uname uptime users who
EXTRA_SCRIPTS = nohup

bin_SCRIPTS = groups @OPTIONAL_BIN_ZCRIPTS@
bin_PROGRAMS = chgrp chown chmod cp dd dircolors du \
  ginstall link ln dir vdir ls mkdir \
  mkfifo mknod mv rm rmdir shred stat sync touch unlink \
  cat cksum comm csplit cut expand fmt fold head join md5sum \
  nl od paste pr ptx sha1sum sort split sum tac tail tr tsort unexpand uniq wc \
  basename date dirname echo env expr factor false \
  hostname id kill logname pathchk printenv printf pwd seq sleep tee \
  test true tty whoami yes \
  @OPTIONAL_BIN_PROGS@ @DF_PROG@

localedir = $(datadir)/locale

noinst_HEADERS = \
  system.h sys2.h checksum.h copy.h cp-hash.h ls.h dircolors.h remove.h \
  chown-core.h fs.h \
  wheel.h wheel-size.h
EXTRA_DIST = dcgen dircolors.hin tac-pipe.c \
  groups.sh nohup.sh wheel-gen.pl
CLEANFILES = $(SCRIPTS) su

INCLUDES = -I.. -I$(srcdir) -I$(top_srcdir)/lib -I../lib
DEFS = -DLOCALEDIR=\"$(localedir)\" -DSHAREDIR=\"$(datadir)\" @DEFS@

# Sometimes, the expansion of @LIBINTL@ includes -lc which may
# include modules defining variables like `optind', so libfetish.a
# must precede @LIBINTL@ in order to ensure we use GNU getopt.
# But libfetish.a must also follow @LIBINTL@, since libintl uses
# replacement functions defined in libfetish.a.
LDADD = ../lib/libfetish.a @LIBINTL@ ../lib/libfetish.a

dir_LDADD = $(LDADD) @LIB_CLOCK_GETTIME@
ls_LDADD = $(LDADD) @LIB_CLOCK_GETTIME@
shred_LDADD = $(LDADD) @LIB_CLOCK_GETTIME@
vdir_LDADD = $(LDADD) @LIB_CLOCK_GETTIME@

## If necessary, add -lm to resolve use of pow in lib/strtod.c.
sort_LDADD = $(LDADD) @POW_LIB@

# for clock_gettime
date_LDADD = $(LDADD) @LIB_CLOCK_GETTIME@

# For sqrt
factor_LDADD = $(LDADD) @SQRT_LIBM@

# If necessary, add -lm to resolve use of pow in lib/strtod.c.
# If necessary, add -liconv to resolve use of iconv in lib/unicodeio.c.
printf_LDADD = $(LDADD) @POW_LIB@ @LIBICONV@

# If necessary, add -lm to resolve use of floor, rint, modf.
seq_LDADD = $(LDADD) @SEQ_LIBM@

# If necessary, add -lm to resolve the `pow' reference in lib/strtod.c
# or for the fesetround reference in sleep.c.
sleep_LDADD = $(LDADD) @FESETROUND_LIBM@ @POW_LIB@ \
  @LIB_CLOCK_GETTIME@ @LIB_NANOSLEEP@

uptime_LDADD = $(LDADD) @GETLOADAVG_LIBS@

su_LDADD = $(LDADD) @LIB_CRYPT@

$(PROGRAMS): ../lib/libfetish.a

$(SCRIPTS): Makefile

SUFFIXES = .sh

.sh:
	rm -f $@ $@-t
	sed \
	  -e 's!@''bindir''@!$(bindir)!' \
	  -e 's/@''GNU_PACKAGE''@/@GNU_PACKAGE@/' \
	  -e 's/@''PACKAGE_BUGREPORT''@/@PACKAGE_BUGREPORT@/' \
	  -e 's/@''VERSION''@/@VERSION@/' $< > $@-t
	chmod +x $@-t
	mv $@-t $@

all-local: su

installed_su = $(DESTDIR)$(bindir)/`echo su|sed '$(transform)'`

setuid_root_mode = a=rx,u+s

INSTALL_SU = \
  p=su; \
  echo " $(INSTALL_PROGRAM) $$p $(installed_su)"; \
  $(INSTALL_PROGRAM) $$p $(installed_su); \
  echo " chown root $(installed_su)"; \
  chown root $(installed_su); \
  echo " chmod $(setuid_root_mode) $(installed_su)"; \
  chmod $(setuid_root_mode) $(installed_su)

install-root: su
	@$(INSTALL_SU)

install-exec-local: su
	@TMPFILE=$(DESTDIR)$(bindir)/.su-$$$$; \
	rm -f $$TMPFILE; \
	echo > $$TMPFILE; \
## See if we can create a setuid root executable in $(bindir).
## If not, then don't even try to install su.
	can_create_suid_root_executable=no; \
	chown root $$TMPFILE > /dev/null 2>&1 \
	  && chmod $(setuid_root_mode) $$TMPFILE > /dev/null 2>&1 \
	  && can_create_suid_root_executable=yes; \
	rm -f $$TMPFILE; \
	if test $$can_create_suid_root_executable = yes; then \
	  $(INSTALL_SU); \
	else \
	  echo "WARNING: insufficient access; not installing su"; \
	  echo "NOTE: to install su, run 'make install-root' as root"; \
	fi

uninstall-local:
# Remove su only if it's one we installed.
	@if grep '@GNU_PACKAGE@' $(installed_su) > /dev/null 2>&1; then \
	  echo "  rm -f $(installed_su)"; \
	  rm -f $(installed_su); \
	else :; fi

# Use `ginstall' in the definition of PROGRAMS and in dependencies to avoid
# confusion with the `install' target.  The install rule transforms `ginstall'
# to install before applying any user-specified name transformations.

transform = s/ginstall/install/; @program_transform_name@
ginstall_SOURCES = install.c copy.c cp-hash.c

cp_SOURCES = cp.c copy.c cp-hash.c
dir_SOURCES = ls.c ls-dir.c
vdir_SOURCES = ls.c ls-vdir.c
ls_SOURCES = ls.c ls-ls.c
chown_SOURCES = chown.c chown-core.c
chgrp_SOURCES = chgrp.c chown-core.c

mv_SOURCES = mv.c copy.c cp-hash.c remove.c
rm_SOURCES = rm.c remove.c

md5sum_SOURCES = md5sum.c md5.c
sha1sum_SOURCES = md5sum.c sha1sum.c

PERL = @PERL@
editpl = sed -e 's,@''PERL''@,$(PERL),g'

MAINTAINERCLEANFILES = dircolors.h \
  wheel.h wheel-size.h

dircolors.h: dcgen dircolors.hin
	$(PERL) -w -- $(srcdir)/dcgen $(srcdir)/dircolors.hin > $@-t
	mv $@-t $@

PERL = @PERL@

wheel_size = 5

wheel-size.h: Makefile.am
	echo '#define WHEEL_SIZE $(wheel_size)' > $@-t
	mv $@-t $@

wheel.h: wheel-gen.pl Makefile.am
	$(srcdir)/wheel-gen.pl $(wheel_size) \
	  > $@-t
	mv $@-t $@

BUILT_SOURCES = dircolors.h false.c wheel.h wheel-size.h

# false exits nonzero even with --help or --version.
# Tell automake to exempt it from that installcheck test.
AM_INSTALLCHECK_STD_OPTIONS_EXEMPT = false

false.c: true.c
	sed \
	  -e s/true/false/g \
	  -e s/success/failure/g \
	  -e 's/^  exit (EXIT_SUCCESS/  exit (EXIT_FAILURE/g' \
	  $(srcdir)/true.c > $@-t
	mv $@-t $@

pm = progs-makefile
pr = progs-readme
# Ensure that the list of programs in README matches the list
# of programs we can build.
check: check-README
.PHONY: check-README
check-README:
	rm -rf $(pr) $(pm)
	echo $(EXTRA_PROGRAMS) $(EXTRA_SCRIPTS) $(bin_SCRIPTS) $(bin_PROGRAMS) \
	  | tr -s ' ' '\n' | sort -u > $(pm)
	sed -n '/^The programs .* are:/,/^[a-zA-Z]/p' $(top_srcdir)/README \
	  | sed -n '/^   */s///p' | tr -s ' ' '\n' > $(pr)
	diff $(pm) $(pr) && rm -rf $(pr) $(pm)
