## Process this file with automake to produce Makefile.in.

##test-files-begin
x = factor
explicit =
maint_gen = 1.X 2.X 3.X 4.X
run_gen = 1.O 1.E 2.O 2.E 3.O 3.E 4.O 4.E
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
