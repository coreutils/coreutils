## Process this file with automake to produce Makefile.in.

##test-files-begin
x = join
explicit =
maint_gen = t1a.in1 t1a.in2 t1a.exp t1b.in1 t1b.in2 t1b.exp t1c.in1 t1c.in2 \
t1c.exp t1d.in1 t1d.in2 t1d.exp t1e.in1 t1e.in2 t1e.exp t1f.in1 t1f.in2 \
t1f.exp t2a.in1 t2a.in2 t2a.exp t2b.in1 t2b.in2 t2b.exp t2c.in1 t2c.in2 \
t2c.exp t3a.in1 t3a.in2 t3a.exp t4a.in1 t4a.in2 t4a.exp t4b.in1 t4b.in2 \
t4b.exp t4c.in1 t4c.in2 t4c.exp t4d.in1 t4d.in2 t4d.exp t4e.in1 t4e.in2 \
t4e.exp t5a.in1 t5a.in2 t5a.exp t5b.in1 t5b.in2 t5b.exp t5c.in1 t5c.in2 \
t5c.exp t5d.in1 t5d.in2 t5d.exp t5e.in1 t5e.in2 t5e.exp t5f.in1 t5f.in2 \
t5f.exp t5g.in1 t5g.in2 t5g.exp t5h.in1 t5h.in2 t5h.exp t5i.in1 t5i.in2 \
t5i.exp t5j.in1 t5j.in2 t5j.exp t5k.in1 t5k.in2 t5k.exp t5l.in1 t5l.in2 \
t5l.exp t5m.in1 t5m.in2 t5m.exp t6a.in1 t6a.in2 t6a.exp t6b.in1 t6b.in2 \
t6b.exp t6c.in1 t6c.in2 t6c.exp t7a.in1 t7a.in2 t7a.exp t8a.in1 t8a.in2 \
t8a.exp t8b.in1 t8b.in2 t8b.exp t9a.in1 t9a.in2 t9a.exp
run_gen = t1a.out t1a.err t1b.out t1b.err t1c.out t1c.err t1d.out t1d.err \
t1e.out t1e.err t1f.out t1f.err t2a.out t2a.err t2b.out t2b.err t2c.out \
t2c.err t3a.out t3a.err t4a.out t4a.err t4b.out t4b.err t4c.out t4c.err \
t4d.out t4d.err t4e.out t4e.err t5a.out t5a.err t5b.out t5b.err t5c.out \
t5c.err t5d.out t5d.err t5e.out t5e.err t5f.out t5f.err t5g.out t5g.err \
t5h.out t5h.err t5i.out t5i.err t5j.out t5j.err t5k.out t5k.err t5l.out \
t5l.err t5m.out t5m.err t6a.out t6a.err t6b.out t6b.err t6c.out t6c.err \
t7a.out t7a.err t8a.out t8a.err t8b.out t8b.err t9a.out t9a.err
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
