## Process this file with automake to produce Makefile.in -*-Makefile-*-.

##test-files-begin
x = date
explicit =
maint_gen = 1a.X 1b.X 1c.X 1d.X and-1.X and-2.X and-3.X and-4.X or-1.X or-2.X \
or-3.X or-4.X t1.X t2.X
run_gen = 1a.O 1a.E 1b.O 1b.E 1c.O 1c.E 1d.O 1d.E and-1.O and-1.E and-2.O \
and-2.E and-3.O and-3.E and-4.O and-4.E or-1.O or-1.E or-2.O or-2.E or-3.O \
or-3.E or-4.O or-4.E t1.O t1.E t2.O t2.E
##test-files-end

EXTRA_DIST = Test.pm $x-tests $(explicit) $(maint_gen)
noinst_SCRIPTS = $x-tests

PERL = @PERL@
editpl = sed -e 's,@''PERL''@,$(PERL),g' -e 's,@''srcdir''@,@srcdir@,g'

TESTS = $x-tests

mk_script = $(srcdir)/../mk-script
$(srcdir)/$x-tests: $(mk_script) Test.pm
	$(PERL) -I. -w -- $(mk_script) ../../src/$x > $@.n
	mv $@.n $@
	chmod 755 $@

MAINTAINERCLEANFILES = $x-tests $(maint_gen)
CLEANFILES = $(run_gen)

Makefile.am: ../Makefile.am.in Test.pm $(mk_script)
	rm -f $@ $@t
	sed -n '1,/^##test-files-begin/p' $< > $@t
	tool=`echo $(subdir)|sed 's/^tests.//;s/-test//'`; \
	  echo "x = $$tool" >> $@t
	$(PERL) -I. -w -- $(mk_script) --list >> $@t
	sed -n '/^##test-files-end/,$$p' $< >> $@t
	mv $@t $@
