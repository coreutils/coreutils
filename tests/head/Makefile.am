## Process this file with automake to produce Makefile.in -*-Makefile-*-.

##test-files-begin
x = head
explicit = in in-1024
maint_gen = idem-0.I idem-0.X idem-1.I idem-1.X idem-2.I idem-2.X idem-3.I \
idem-3.X basic-0-10.I basic-0-10.X basic-0-09.I basic-0-09.X basic-0-11.I \
basic-0-11.X obs-0.I obs-0.X obs-1.I obs-1.X obs-2.I obs-2.X obs-3.I obs-3.X \
fail-1.I fail-1.X null-1.I null-1.X no-oct-1.I no-oct-1.X no-oct-2.I \
no-oct-2.X no-oct-3.I no-oct-3.X no-oct-4.I no-oct-4.X posix-0.I posix-0.X \
posix-1.I posix-1.X posix-2.I posix-2.X posix-3.I posix-3.X
run_gen = idem-0.O idem-0.E idem-1.O idem-1.E idem-2.O idem-2.E idem-3.O \
idem-3.E basic-0-10.O basic-0-10.E basic-0-09.O basic-0-09.E basic-0-11.O \
basic-0-11.E obs-0.O obs-0.E obs-1.O obs-1.E obs-2.O obs-2.E obs-3.O obs-3.E \
obs-4.O obs-4.E obs-5.O obs-5.E fail-1.O fail-1.E null-1.O null-1.E \
no-oct-1.O no-oct-1.E no-oct-2.O no-oct-2.E no-oct-3.O no-oct-3.E no-oct-4.O \
no-oct-4.E posix-0.O posix-0.E posix-1.O posix-1.E posix-2.O posix-2.E \
posix-3.O posix-3.E posix-4.O posix-4.E posix-5.O posix-5.E
##test-files-end

EXTRA_DIST = Test.pm $x-tests $(explicit) $(maint_gen)
noinst_SCRIPTS = $x-tests
TESTS_ENVIRONMENT = \
  PATH="`pwd`/../../src$(PATH_SEPARATOR)$$PATH"

editpl = sed -e 's,@''PERL''@,$(PERL),g' -e 's,@''srcdir''@,$(srcdir),g'

TESTS = $x-tests

mk_script = $(srcdir)/../mk-script
$(srcdir)/$x-tests: $(mk_script) Test.pm Makefile.am
	test '$x' = test && prog=../../src/$x || prog=$x; \
	$(PERL) -I$(srcdir) -w -- $(mk_script) $(srcdir) $$prog > $@.n
	mv $@.n $@
	chmod 755 $@

MAINTAINERCLEANFILES = $x-tests $(maint_gen)
CLEANFILES = $(run_gen)

check: $(maint_gen)

$(srcdir)/Makefile.am: ../Makefile.am.in Test.pm $(mk_script)
	rm -f $@ $@t
	sed -n '1,/^##test-files-begin/p' $(srcdir)/../Makefile.am.in > $@t
	tool=`echo $(subdir)|sed 's/^tests.//;s/-test//'`; \
	  echo "x = $$tool" >> $@t
	$(PERL) -I$(srcdir) -w -- $(mk_script) $(srcdir) --list >> $@t
	sed -n '/^##test-files-end/,$$p' $(srcdir)/../Makefile.am.in >> $@t
	mv $@t $@
