## Process this file with automake to produce Makefile.in.

##test-files-begin
x = tr
explicit =
maint_gen = t1.in t1.exp t2.in t2.exp t3.in t3.exp t4.in t4.exp t5.in t5.exp \
t6.in t6.exp t7.in t7.exp t8.in t8.exp t9.in t9.exp ta.in ta.exp tb.in tb.exp \
tc.in tc.exp td.in td.exp te.in te.exp tf.in tf.exp tg.in tg.exp th.in th.exp \
ti.in ti.exp tj.in tj.exp tk.in tk.exp tl.in tl.exp tm.in tm.exp tn.in tn.exp \
to.in to.exp tp.in tp.exp tq.in tq.exp tr.in tr.exp ts.in ts.exp tt.in tt.exp \
tu.in tu.exp tv.in tv.exp tw.in tw.exp tx.in tx.exp ty.in ty.exp tz.in tz.exp \
tA.in tA.exp tB.in tB.exp tC.in tC.exp tD.in tD.exp tE.in tE.exp tF.in tF.exp \
tG.in tG.exp tH.in tH.exp tI.in tI.exp tR0.0.in tR0.0.exp tR0.1.in tR0.1.exp \
tR1.0.in tR1.0.exp tR1.1.in tR1.1.exp tR2.in tR2.exp tR3.in tR3.exp tR4.in \
tR4.exp tR5.in tR5.exp tR6.in tR6.exp
run_gen = t1.out t1.err t2.out t2.err t3.out t3.err t4.out t4.err t5.out \
t5.err t6.out t6.err t7.out t7.err t8.out t8.err t9.out t9.err ta.out ta.err \
tb.out tb.err tc.out tc.err td.out td.err te.out te.err tf.out tf.err tg.out \
tg.err th.out th.err ti.out ti.err tj.out tj.err tk.out tk.err tl.out tl.err \
tm.out tm.err tn.out tn.err to.out to.err tp.out tp.err tq.out tq.err tr.out \
tr.err ts.out ts.err tt.out tt.err tu.out tu.err tv.out tv.err tw.out tw.err \
tx.out tx.err ty.out ty.err tz.out tz.err tA.out tA.err tB.out tB.err tC.out \
tC.err tD.out tD.err tE.out tE.err tF.out tF.err tG.out tG.err tH.out tH.err \
tI.out tI.err tR0.0.out tR0.0.err tR0.1.out tR0.1.err tR1.0.out tR1.0.err \
tR1.1.out tR1.1.err tR2.out tR2.err tR3.out tR3.err tR4.out tR4.err tR5.out \
tR5.err tR6.out tR6.err
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
