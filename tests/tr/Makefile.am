## Process this file with automake to produce Makefile.in.

AUTOMAKE_OPTIONS = 1.0

EXTRA_DIST = build-script.pl Trtest.pm
noinst_SCRIPTS = tr-tests

PERL = @PERL@
editpl = sed -e 's,@''PERL''@,$(PERL),g'

TESTS = tr-tests

tr-tests: build-script Trtest.pm
	./build-script Trtest.pm > $@.n
	mv $@.n $@
	chmod 755 $@

SUFFIXES = .pl

.pl:
	rm -f $@ $@.tmp
	$(editpl) $< > $@.tmp && chmod +x $@.tmp && mv $@.tmp $@

MAINTAINERCLEANFILES = tr-tests t*.in t*.exp
CLEANFILES = t*.out t*.err
