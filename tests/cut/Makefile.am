## Process this file with automake to produce Makefile.in.

x = cut

##test-files-begin
explicit =
maint_gen = t1.in t1.exp t2.in t2.exp t3.in t3.exp t4.in t4.exp t5.in t5.exp \
t6.in t6.exp t7.in t7.exp t8.in t8.exp t9.in t9.exp ta.in ta.exp tb.in tb.exp \
tc.in tc.exp td.in td.exp te.in te.exp tf.in tf.exp tg.in tg.exp th.in th.exp \
ti.in ti.exp tj.in tj.exp tk.in tk.exp tl.in tl.exp tm.in tm.exp tn.in tn.exp \
to.in to.exp tp.in tp.exp tq.in tq.exp tr.in tr.exp ts.in ts.exp tt.in tt.exp \
tu.in tu.exp tv.in tv.exp tw.in tw.exp tx.in tx.exp ty.in ty.exp tz.in tz.exp \
tA.in tA.exp tB.in tB.exp tC.in tC.exp tD.in tD.exp
run_gen = t1.out t1.err t2.out t2.err t3.out t3.err t4.out t4.err t5.out \
t5.err t6.out t6.err t7.out t7.err t8.out t8.err t9.out t9.err ta.out ta.err \
tb.out tb.err tc.out tc.err td.out td.err te.out te.err tf.out tf.err tg.out \
tg.err th.out th.err ti.out ti.err tj.out tj.err tk.out tk.err tl.out tl.err \
tm.out tm.err tn.out tn.err to.out to.err tp.out tp.err tq.out tq.err tr.out \
tr.err ts.out ts.err tt.out tt.err tu.out tu.err tv.out tv.err tw.out tw.err \
tx.out tx.err ty.out ty.err tz.out tz.err tA.out tA.err tB.out tB.err tC.out \
tC.err tD.out tD.err
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
@MAINT@	sed -n '1,/^##test-files-begin/p' Makefile.am > $$rb; \
@MAINT@	./mk-script --list >> $$rb; \
@MAINT@	sed -n '/^##test-files-end/,$$p' Makefile.am >> $$rb; \
@MAINT@	diff -u Makefile.am $$rb && echo ok || echo no
