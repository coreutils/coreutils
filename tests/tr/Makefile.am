## Process this file with automake to produce Makefile.in.

AUTOMAKE_OPTIONS = 1.1c

x = tr
in = \
  t1.in t2.in t3.in t4.in t5.in t6.in t7.in t8.in t9.in tA.in tB.in \
  tC.in tD.in tE.in tF.in tG.in tH.in tI.in tR0.0.in tR0.1.in tR1.0.in \
  tR1.1.in tR2.in tR3.in tR4.in tR5.in tR6.in ta.in tb.in tc.in td.in \
  te.in tf.in tg.in th.in ti.in tj.in tk.in tl.in tm.in tn.in to.in \
  tp.in tq.in tr.in ts.in tt.in tu.in tv.in tw.in tx.in ty.in tz.in

exp = $(in:.in=.exp)
out = $(in:.in=.out)
err = $(in:.in=.err)

EXTRA_DIST = build-script.pl Test.pm $(in) $(exp)
noinst_SCRIPTS = $x-tests

PERL = @PERL@
editpl = sed -e 's,@''PERL''@,$(PERL),g' -e 's,@''srcdir''@,@srcdir@,g'

TESTS = $x-tests

$x-tests: build-script Test.pm
	./build-script ../../src/$x > $@.n
	mv $@.n $@
	chmod 755 $@

SUFFIXES = .pl

.pl:
	rm -f $@ $@.tmp
	$(editpl) $< > $@.tmp && chmod +x $@.tmp && mv $@.tmp $@

MAINTAINERCLEANFILES = $x-tests $(in) $(exp)
CLEANFILES = $(out) $(err)
