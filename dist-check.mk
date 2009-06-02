# Most of this is probably too coreutils-centric to be useful to other packages.

bin=bin-$$$$

write_loser = printf '\#!%s\necho $$0: bad path 1>&2; exit 1\n' '$(SHELL)'

TMPDIR ?= /tmp
t=$(TMPDIR)/$(PACKAGE)/test
pfx=$(t)/i

# More than once, tainted build and source directory names would
# have caused at least one "make check" test to apply "chmod 700"
# to all directories under $HOME.  Make sure it doesn't happen again.
tp := $(shell echo "$(TMPDIR)/$(PACKAGE)-$$$$")
t_prefix = $(tp)/a
t_taint = '$(t_prefix) b'
fake_home = $(tp)/home

# Ensure that tests run from tainted build and src dir names work,
# and don't affect anything in $HOME.  Create witness files in $HOME,
# record their attributes, and build/test.  Then ensure that the
# witnesses were not affected.
ALL_RECURSIVE_TARGETS += taint-distcheck
taint-distcheck: $(DIST_ARCHIVES)
	test -d $(t_taint) && chmod -R 700 $(t_taint) || :
	-rm -rf $(t_taint) $(fake_home)
	mkdir -p $(t_prefix) $(t_taint) $(fake_home)
	GZIP=$(GZIP_ENV) $(AMTAR) -C $(t_taint) -zxf $(distdir).tar.gz
	mkfifo $(fake_home)/fifo
	touch $(fake_home)/f
	mkdir -p $(fake_home)/d/e
	ls -lR $(fake_home) $(t_prefix) > $(tp)/.ls-before
	cd $(t_taint)/$(distdir)			\
	  && ./configure				\
	  && $(MAKE)					\
	  && HOME=$(fake_home) $(MAKE) check		\
	  && ls -lR $(fake_home) $(t_prefix) > $(tp)/.ls-after \
	  && diff $(tp)/.ls-before $(tp)/.ls-after	\
	  && test -d $(t_prefix)
	rm -rf $(tp)

# Verify that a twisted use of --program-transform-name=PROGRAM works.
define install-transform-check
  echo running install-transform-check			\
    && rm -rf $(pfx)					\
    && $(MAKE) program_transform_name='s/.*/zyx/'	\
      prefix=$(pfx) install				\
    && test "$$(echo $(pfx)/bin/*)" = "$(pfx)/bin/zyx"	\
    && test "$$(find $(pfx)/share/man -type f|sed 's,.*/,,;s,\..*,,')" = "zyx"
endef

# Install, then verify that all binaries and man pages are in place.
# Note that neither the binary, ginstall, nor the ].1 man page is installed.
define my-instcheck
  $(MAKE) prefix=$(pfx) install				\
    && test ! -f $(pfx)/bin/ginstall			\
    && { fail=0;					\
      for i in $(built_programs); do			\
        test "$$i" = ginstall && i=install;		\
        for j in "$(pfx)/bin/$$i"			\
                 "$(pfx)/share/man/man1/$$i.1"; do	\
          case $$j in *'[.1') continue;; esac;		\
          test -f "$$j" && :				\
            || { echo "$$j not installed"; fail=1; };	\
        done;						\
      done;						\
      test $$fail = 1 && exit 1 || :;			\
    }
endef

define coreutils-path-check
  {							\
    if test -f $(srcdir)/src/true.c; then		\
      fail=1;						\
      mkdir $(bin)					\
	&& ($(write_loser)) > $(bin)/loser		\
	&& chmod a+x $(bin)/loser			\
	&& for i in $(built_programs); do		\
	       case $$i in				\
		 rm|expr|basename|echo|sort|ls|tr);;	\
		 cat|dirname|mv|wc);;			\
		 *) ln $(bin)/loser $(bin)/$$i;;	\
	       esac;					\
	     done					\
	  && ln -sf ../src/true $(bin)/false		\
	  && PATH=`pwd`/$(bin)$(PATH_SEPARATOR)$$PATH	\
		$(MAKE) -C tests check			\
	  && { test -d gnulib-tests			\
		 && $(MAKE) -C gnulib-tests check	\
		 || :; }				\
	  && rm -rf $(bin)				\
	  && fail=0;					\
    else						\
      fail=0;						\
    fi;							\
    test $$fail = 1 && exit 1 || :;			\
  }
endef

# Use -Wformat -Werror to detect format-string/arg-list mismatches.
# Also, check for shadowing problems with -Wshadow, and for pointer
# arithmetic problems with -Wpointer-arith.
# These CFLAGS are pretty strict.  If you build this target, you probably
# have to have a recent version of gcc and glibc headers.
# The hard-linking for-loop below ensures that there is a bin/ directory
# full of all of the programs under test (except the ones that are required
# for basic Makefile rules), all symlinked to the just-built "false" program.
# This is to ensure that if ever a test neglects to make PATH include
# the build srcdir, these always-failing programs will run.
# Otherwise, it is too easy to test the wrong programs.
# Note that "false" itself is a symlink to true, so it too will malfunction.
ALL_RECURSIVE_TARGETS += my-distcheck
my-distcheck: $(DIST_ARCHIVES) $(local-check)
	$(MAKE) syntax-check
	$(MAKE) check
	-rm -rf $(t)
	mkdir -p $(t)
	GZIP=$(GZIP_ENV) $(AMTAR) -C $(t) -zxf $(distdir).tar.gz
	cd $(t)/$(distdir)				\
	  && ./configure --enable-gcc-warnings --disable-nls \
	  && $(MAKE) AM_MAKEFLAGS='$(null_AM_MAKEFLAGS)' \
	  && $(MAKE) dvi				\
	  && $(install-transform-check)			\
	  && $(my-instcheck)				\
	  && $(coreutils-path-check)			\
	  && $(MAKE) distclean
	(cd $(t) && mv $(distdir) $(distdir).old	\
	  && $(AMTAR) -zxf - ) < $(distdir).tar.gz
	diff -ur $(t)/$(distdir).old $(t)/$(distdir)
	-rm -rf $(t)
	@echo "========================"; \
	echo "$(distdir).tar.gz is ready for distribution"; \
	echo "========================"
