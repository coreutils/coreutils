## Process this file with automake to produce Makefile.in.

x = tr
t = \
  t1 t2 t3 t4 t5 t6 t7 t8 t9 tA tB tC tD tE tF tG tH tI tR0.0 tR0.1 tR1.0 \
  tR1.1 tR2 tR3 tR4 tR5 tR6 ta tb tc td te tf tg th ti tj tk tl tm tn to \
  tp tq tr ts tt tu tv tw tx ty tz

in = $(t:=.in)
exp = $(t:=.exp)
out = $(t:=.out)
err = $(t:=.err)

EXTRA_DIST = mk-script.pl Test.pm $x-tests $(in) $(exp)
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
	$(editpl) $< > $@.tmp && chmod +x $@.tmp && mv $@.tmp $@

MAINTAINERCLEANFILES = $x-tests $(in) $(exp)
CLEANFILES = $(out) $(err)
