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
trange-a-a.in trange-a-a.exp tnull.in tnull.exp tupcase.in tupcase.exp \
tdncase.in tdncase.exp trep-cclass.in trep-cclass.exp trep-1.in trep-1.exp \
trep-2.in trep-2.exp trep-3.in trep-3.exp tesc.in tesc.exp tross-0a.in \
tross-0a.exp tross-0b.in tross-0b.exp tross-1a.in tross-1a.exp tross-1b.in \
tross-1b.exp tross-2.in tross-2.exp tross-3.in tross-3.exp tross-4.in \
tross-4.exp tross-5.in tross-5.exp tross-6.in tross-6.exp
run_gen = t1.out t1.err t2.out t2.err t3.out t3.err t4.out t4.err t5.out \
t5.err t6.out t6.err t7.out t7.err t8.out t8.err t9.out t9.err ta.out ta.err \
tb.out tb.err tc.out tc.err td.out td.err te.out te.err tf.out tf.err tg.out \
tg.err th.out th.err ti.out ti.err tj.out tj.err tk.out tk.err tl.out tl.err \
tm.out tm.err tn.out tn.err to.out to.err tp.out tp.err tq.out tq.err tr.out \
tr.err ts.out ts.err tt.out tt.err tu.out tu.err tv.out tv.err tw.out tw.err \
tx.out tx.err ty.out ty.err tz.out tz.err trange-a-a.out trange-a-a.err \
tnull.out tnull.err tupcase.out tupcase.err tdncase.out tdncase.err \
trep-cclass.out trep-cclass.err trep-1.out trep-1.err trep-2.out trep-2.err \
trep-3.out trep-3.err tesc.out tesc.err tross-0a.out tross-0a.err \
tross-0b.out tross-0b.err tross-1a.out tross-1a.err tross-1b.out tross-1b.err \
tross-2.out tross-2.err tross-3.out tross-3.err tross-4.out tross-4.err \
tross-5.out tross-5.err tross-6.out tross-6.err
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
