## Process this file with automake to produce Makefile.in -*-Makefile-*-.

##test-files-begin
x = ls
explicit =
maint_gen = q q-.X q-N.X q-Q.X q-q.X q-qs-c.X q-qs-esc.X q-qs-lit.X \
q-qs-sh-a.X q-qs-sh.X
run_gen = q-.O q-.E q-N.O q-N.E q-q.O q-q.E q-Q.O q-Q.E q-qs-lit.O q-qs-lit.E \
q-qs-sh.O q-qs-sh.E q-qs-sh-a.O q-qs-sh-a.E q-qs-c.O q-qs-c.E q-qs-esc.O \
q-qs-esc.E
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

check: $(maint_gen)

Makefile.am: ../Makefile.am.in Test.pm $(mk_script)
	rm -f $@ $@t
	sed -n '1,/^##test-files-begin/p' $< > $@t
	tool=`echo $(subdir)|sed 's/^tests.//;s/-test//;s/-[0-9]$$//'`; \
	  echo "x = $$tool" >> $@t
	$(PERL) -I. -w -- $(mk_script) --list >> $@t
	sed -n '/^##test-files-end/,$$p' $< >> $@t
	mv $@t $@
