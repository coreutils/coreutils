## Process this file with automake to produce Makefile.in.

##test-files-begin
x = tail
explicit =
maint_gen = tobs-c1.in tobs-c1.exp tobs-c2.in tobs-c2.exp tobs-c3.in \
tobs-c3.exp tobs-c4.in tobs-c4.exp tobs-c5.in tobs-c5.exp tobs-l1.in \
tobs-l1.exp tobs-l2.in tobs-l2.exp tobs-l3.in tobs-l3.exp tobs-l4.in \
tobs-l4.exp tobs-l5.in tobs-l5.exp tobs-1.in tobs-1.exp tobs-2.in tobs-2.exp \
tobs-3.in tobs-3.exp tobs-4.in tobs-4.exp tobs-5.in tobs-5.exp tobsx-1.in \
tobsx-1.exp tobsx-2.in tobsx-2.exp tobs-l.in tobs-l.exp terr-1.in terr-1.exp \
terr-2.in terr-2.exp terr-3.in terr-3.exp terr-4.in terr-4.exp tstdin-1.in \
tstdin-1.exp tstdin-2.in tstdin-2.exp tn-1.in tn-1.exp tn-2.in tn-2.exp \
tn-3.in tn-3.exp tn-4.in tn-4.exp tn-4a.in tn-4a.exp tn-5.in tn-5.exp \
tn-5a.in tn-5a.exp tn-5b.in tn-5b.exp
run_gen = tobs-c1.out tobs-c1.err tobs-c2.out tobs-c2.err tobs-c3.out \
tobs-c3.err tobs-c4.out tobs-c4.err tobs-c5.out tobs-c5.err tobs-l1.out \
tobs-l1.err tobs-l2.out tobs-l2.err tobs-l3.out tobs-l3.err tobs-l4.out \
tobs-l4.err tobs-l5.out tobs-l5.err tobs-1.out tobs-1.err tobs-2.out \
tobs-2.err tobs-3.out tobs-3.err tobs-4.out tobs-4.err tobs-5.out tobs-5.err \
tobsx-1.out tobsx-1.err tobsx-2.out tobsx-2.err tobs-l.out tobs-l.err \
terr-1.out terr-1.err terr-2.out terr-2.err terr-3.out terr-3.err terr-4.out \
terr-4.err tstdin-1.out tstdin-1.err tstdin-2.out tstdin-2.err tn-1.out \
tn-1.err tn-2.out tn-2.err tn-3.out tn-3.err tn-4.out tn-4.err tn-4a.out \
tn-4a.err tn-5.out tn-5.err tn-5a.out tn-5a.err tn-5b.out tn-5b.err
##test-files-end

EXTRA_DIST = mk-script.pl Test.pm $x-tests $(explicit) $(maint_gen)
noinst_SCRIPTS = $x-tests

PERL = @PERL@
editpl = sed -e 's,@''PERL''@,$(PERL),g' -e 's,@''srcdir''@,@srcdir@,g'

TESTS = $x-tests

$x-tests: @MAINT@mk-script Test.pm
	./mk-script ../../src/$x > $@.n
	mv $@.n $@
	chmod 755 $@

SUFFIXES = .pl

.pl:
	rm -f $@ $@.tmp
	$(editpl) $< > $@.tmp
	chmod +x-w $@.tmp
	mv $@.tmp $@

MAINTAINERCLEANFILES = $x-tests $(maint_gen)
CLEANFILES = $(run_gen)

@MAINT@rebuild-check: Test.pm mk-script
@MAINT@	rb=rb-check; rm -f $rb; \
@MAINT@	m_template=../Makefile.am.in; \
@MAINT@	sed -n '1,/^##test-files-begin/p' $$m_template > $$rb; \
@MAINT@	tool=`echo $(subdir)|sed 's/^tests.//;s/-test//'`; \
@MAINT@	echo "x = $$tool" >> $$rb; \
@MAINT@	./mk-script --list >> $$rb; \
@MAINT@	sed -n '/^##test-files-end/,$$p' $$m_template >> $$rb; \
@MAINT@	diff -u Makefile.am $$rb && echo ok || echo no
