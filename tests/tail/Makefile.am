## Process this file with automake to produce Makefile.in -*-Makefile-*-.

##test-files-begin
x = tail
explicit =
maint_gen = obs-c1.I obs-c1.X obs-c2.I obs-c2.X obs-c3.I obs-c3.X obs-c4.I \
obs-c4.X obs-c5.I obs-c5.X obs-l1.I obs-l1.X obs-l2.I obs-l2.X obs-l3.I \
obs-l3.X obs-l4.I obs-l4.X obs-l5.I obs-l5.X obs-1.I obs-1.X obs-2.I obs-2.X \
obs-3.I obs-3.X obs-4.I obs-4.X obs-5.I obs-5.X obsx-1.I obsx-1.X obsx-2.I \
obsx-2.X obs-l.I obs-l.X err-1.I err-1.X err-2.I err-2.X err-3.I err-3.X \
err-4.I err-4.X err-5.I err-5.X err-6.I err-6.X minus-1.I minus-1.X minus-2.I \
minus-2.X n-1.I n-1.X n-2.I n-2.X n-3.I n-3.X n-4.I n-4.X n-4a.I n-4a.X n-5.I \
n-5.X n-5a.I n-5a.X n-5b.I n-5b.X
run_gen = obs-c1.O obs-c1.E obs-c2.O obs-c2.E obs-c3.O obs-c3.E obs-c4.O \
obs-c4.E obs-c5.O obs-c5.E obs-l1.O obs-l1.E obs-l2.O obs-l2.E obs-l3.O \
obs-l3.E obs-l4.O obs-l4.E obs-l5.O obs-l5.E obs-1.O obs-1.E obs-2.O obs-2.E \
obs-3.O obs-3.E obs-4.O obs-4.E obs-5.O obs-5.E obsx-1.O obsx-1.E obsx-2.O \
obsx-2.E obs-l.O obs-l.E err-1.O err-1.E err-2.O err-2.E err-3.O err-3.E \
err-4.O err-4.E err-5.O err-5.E err-6.O err-6.E minus-1.O minus-1.E minus-2.O \
minus-2.E n-1.O n-1.E n-2.O n-2.E n-3.O n-3.E n-4.O n-4.E n-4a.O n-4a.E n-5.O \
n-5.E n-5a.O n-5a.E n-5b.O n-5b.E
##test-files-end

EXTRA_DIST = Test.pm $x-tests $(explicit) $(maint_gen)
noinst_SCRIPTS = $x-tests

PERL = @PERL@
editpl = sed -e 's,@''PERL''@,$(PERL),g' -e 's,@''srcdir''@,@srcdir@,g'

TESTS = $x-tests

mk_script = $(srcdir)/../mk-script
$(srcdir)/$x-tests: $(mk_script) Test.pm
	$(PERL) -I$(srcdir) -w -- $(mk_script) ../../src/$x > $@.n
	mv $@.n $@
	chmod 755 $@

MAINTAINERCLEANFILES = $x-tests $(maint_gen)
CLEANFILES = $(run_gen)

check: $(maint_gen)

Makefile.am: ../Makefile.am.in Test.pm $(mk_script)
	rm -f $@ $@t
	sed -n '1,/^##test-files-begin/p' $< > $@t
	tool=`echo $(subdir)|sed 's/^tests.//;s/-test//'`; \
	  echo "x = $$tool" >> $@t
	$(PERL) -I$(srcdir) -w -- $(mk_script) --list >> $@t
	sed -n '/^##test-files-end/,$$p' $< >> $@t
	mv $@t $@
