## Process this file with automake to produce Makefile.in.

AUTOMAKE_OPTIONS = 1.0

EXTRA_DIST = build-script.pl test.data.pl
noinst_SCRIPTS = tr-tests

PERL = @PERL@
editpl = sed -e 's,@''PERL''@,$(PERL),g'

TESTS = tr-tests

tr-tests: build-script test.data.pl
	./build-script test.data.pl > $@.n
	mv $@.n $@
	chmod 755 $@

SUFFIXES = .pl

.pl:
	rm -f $@ $@.tmp
	$(editpl) $< > $@.tmp && chmod +x $@.tmp && mv $@.tmp $@

MAINTAINERCLEANFILES = tr-tests t*.in t*.exp t*.err
CLEANFILES = t*.out
