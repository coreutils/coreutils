## Process this file with automake to produce Makefile.in.

x = sort

##test-files-begin
explicit =
maint_gen = t01a.in t01a.exp t02a.in t02a.exp t02b.in t02b.exp t02c.in \
t02c.exp t02d.in t02d.exp t02e.in t02e.exp t02f.in t02f.exp t03a.in t03a.exp \
t03b.in t03b.exp t03c.in t03c.exp t03d.in t03d.exp t03e.in t03e.exp t03f.in \
t03f.exp t03g.in t03g.exp t03h.in t03h.exp t03i.in t03i.exp t04a.in t04a.exp \
t04b.in t04b.exp t04c.in t04c.exp t04d.in t04d.exp t04e.in t04e.exp t05a.in \
t05a.exp t05b.in t05b.exp t05c.in t05c.exp t05d.in t05d.exp t05e.in t05e.exp \
t05f.in t05f.exp t06a.in t06a.exp t06b.in t06b.exp t06c.in t06c.exp t06d.in \
t06d.exp t06e.in t06e.exp t06f.in t06f.exp t07a.in t07a.exp t07b.in t07b.exp \
t07c.in t07c.exp t07d.in t07d.exp t08a.in t08a.exp t08b.in t08b.exp t09a.in \
t09a.exp t09b.in t09b.exp t09c.in t09c.exp t09d.in t09d.exp t10a.in t10a.exp \
t10b.in t10b.exp t10c.in t10c.exp t10d.in t10d.exp t10a0.in t10a0.exp \
t10a1.in t10a1.exp t10a2.in t10a2.exp t10e.in t10e.exp t10f.in t10f.exp \
t10g.in t10g.exp t11a.in t11a.exp t11b.in t11b.exp t11c.in t11c.exp t11d.in \
t11d.exp t12a.in t12a.exp t12b.in t12b.exp t12c.in t12c.exp t12d.in t12d.exp \
t13a.in t13a.exp t13b.in t13b.exp t14a.in t14a.exp t14b.in t14b.exp t15a.in \
t15a.exp t15b.in t15b.exp t15c.in t15c.exp t15d.in t15d.exp t15e.in t15e.exp \
t16a.in t16a.exp t17.in t17.exp t18a.in t18a.exp t18b.in t18b.exp t18c.in \
t18c.exp t18d.in t18d.exp t18e.in t18e.exp
run_gen = t01a.out t01a.err t02a.out t02a.err t02b.out t02b.err t02c.out \
t02c.err t02d.out t02d.err t02e.out t02e.err t02f.out t02f.err t03a.out \
t03a.err t03b.out t03b.err t03c.out t03c.err t03d.out t03d.err t03e.out \
t03e.err t03f.out t03f.err t03g.out t03g.err t03h.out t03h.err t03i.out \
t03i.err t04a.out t04a.err t04b.out t04b.err t04c.out t04c.err t04d.out \
t04d.err t04e.out t04e.err t05a.out t05a.err t05b.out t05b.err t05c.out \
t05c.err t05d.out t05d.err t05e.out t05e.err t05f.out t05f.err t06a.out \
t06a.err t06b.out t06b.err t06c.out t06c.err t06d.out t06d.err t06e.out \
t06e.err t06f.out t06f.err t07a.out t07a.err t07b.out t07b.err t07c.out \
t07c.err t07d.out t07d.err t08a.out t08a.err t08b.out t08b.err t09a.out \
t09a.err t09b.out t09b.err t09c.out t09c.err t09d.out t09d.err t10a.out \
t10a.err t10b.out t10b.err t10c.out t10c.err t10d.out t10d.err t10a0.out \
t10a0.err t10a1.out t10a1.err t10a2.out t10a2.err t10e.out t10e.err t10f.out \
t10f.err t10g.out t10g.err t11a.out t11a.err t11b.out t11b.err t11c.out \
t11c.err t11d.out t11d.err t12a.out t12a.err t12b.out t12b.err t12c.out \
t12c.err t12d.out t12d.err t13a.out t13a.err t13b.out t13b.err t14a.out \
t14a.err t14b.out t14b.err t15a.out t15a.err t15b.out t15b.err t15c.out \
t15c.err t15d.out t15d.err t15e.out t15e.err t16a.out t16a.err t17.out \
t17.err t18a.out t18a.err t18b.out t18b.err t18c.out t18c.err t18d.out \
t18d.err t18e.out t18e.err
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
