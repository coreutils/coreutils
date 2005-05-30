## Process this file with automake to produce Makefile.in -*-Makefile-*-.

##test-files-begin
x = test
explicit =
maint_gen = ne-1.X ne-2.X ne-3.X ne-4.X ne-5.X ne-6.X le-1.X le-2.X le-3.X \
le-4.X le-5.X ge-1.X ge-2.X ge-3.X ge-4.X ge-5.X N-1b.X P-1b.X NP-1b.X \
NNP-1b.X N-1c.X P-1c.X NP-1c.X NNP-1c.X N-1d.X P-1d.X NP-1d.X NNP-1d.X N-1e.X \
P-1e.X NP-1e.X NNP-1e.X N-1f.X P-1f.X NP-1f.X NNP-1f.X N-1g.X P-1g.X NP-1g.X \
NNP-1g.X N-1h.X P-1h.X NP-1h.X NNP-1h.X N-1i.X P-1i.X NP-1i.X NNP-1i.X N-1j.X \
P-1j.X NP-1j.X NNP-1j.X N-1k.X P-1k.X NP-1k.X NNP-1k.X N-streq-1.X \
P-streq-1.X NP-streq-1.X NNP-streq-1.X N-streq-2.X P-streq-2.X NP-streq-2.X \
NNP-streq-2.X N-streq-3.X P-streq-3.X NP-streq-3.X NNP-streq-3.X N-streq-4.X \
P-streq-4.X NP-streq-4.X NNP-streq-4.X N-streq-5.X P-streq-5.X NP-streq-5.X \
NNP-streq-5.X N-streq-6.X N-strne-1.X P-strne-1.X NP-strne-1.X NNP-strne-1.X \
N-strne-2.X P-strne-2.X NP-strne-2.X NNP-strne-2.X N-strne-3.X P-strne-3.X \
NP-strne-3.X NNP-strne-3.X N-strne-4.X P-strne-4.X NP-strne-4.X NNP-strne-4.X \
N-strne-5.X P-strne-5.X NP-strne-5.X NNP-strne-5.X N-strne-6.X N-and-1.X \
P-and-1.X NP-and-1.X NNP-and-1.X N-and-2.X P-and-2.X NP-and-2.X NNP-and-2.X \
N-and-3.X P-and-3.X NP-and-3.X NNP-and-3.X N-and-4.X P-and-4.X NP-and-4.X \
NNP-and-4.X N-or-1.X P-or-1.X NP-or-1.X NNP-or-1.X N-or-2.X P-or-2.X \
NP-or-2.X NNP-or-2.X N-or-3.X P-or-3.X NP-or-3.X NNP-or-3.X N-or-4.X P-or-4.X \
NP-or-4.X NNP-or-4.X N-eq-1.X P-eq-1.X NP-eq-1.X NNP-eq-1.X N-eq-2.X P-eq-2.X \
NP-eq-2.X NNP-eq-2.X N-eq-3.X P-eq-3.X NP-eq-3.X NNP-eq-3.X N-eq-4.X P-eq-4.X \
NP-eq-4.X NNP-eq-4.X N-eq-5.X P-eq-5.X NP-eq-5.X NNP-eq-5.X N-eq-6.X P-eq-6.X \
NP-eq-6.X NNP-eq-6.X N-gt-1.X P-gt-1.X NP-gt-1.X NNP-gt-1.X N-gt-2.X P-gt-2.X \
NP-gt-2.X NNP-gt-2.X N-gt-3.X P-gt-3.X NP-gt-3.X NNP-gt-3.X N-gt-4.X P-gt-4.X \
NP-gt-4.X NNP-gt-4.X N-gt-5.X P-gt-5.X NP-gt-5.X NNP-gt-5.X N-lt-1.X P-lt-1.X \
NP-lt-1.X NNP-lt-1.X N-lt-2.X P-lt-2.X NP-lt-2.X NNP-lt-2.X N-lt-3.X P-lt-3.X \
NP-lt-3.X NNP-lt-3.X N-lt-4.X P-lt-4.X NP-lt-4.X NNP-lt-4.X N-lt-5.X P-lt-5.X \
NP-lt-5.X NNP-lt-5.X N-t1.X P-t1.X NP-t1.X NNP-t1.X N-t2.X P-t2.X NP-t2.X \
NNP-t2.X N-paren-1.X N-paren-2.X N-paren-3.X N-paren-4.X N-paren-5.X 1a.X \
1b.X 1c.X 1d.X 1e.X 1f.X 1g.X 1h.X 1i.X 1j.X 1k.X streq-1.X streq-2.X \
streq-3.X streq-4.X streq-5.X streq-6.X strne-1.X strne-2.X strne-3.X \
strne-4.X strne-5.X strne-6.X and-1.X and-2.X and-3.X and-4.X or-1.X or-2.X \
or-3.X or-4.X eq-1.X eq-2.X eq-3.X eq-4.X eq-5.X eq-6.X gt-1.X gt-2.X gt-3.X \
gt-4.X gt-5.X lt-1.X lt-2.X lt-3.X lt-4.X lt-5.X inv-1.X t1.X t2.X paren-1.X \
paren-2.X paren-3.X paren-4.X paren-5.X
run_gen = ne-1.O ne-1.E ne-2.O ne-2.E ne-3.O ne-3.E ne-4.O ne-4.E ne-5.O \
ne-5.E ne-6.O ne-6.E le-1.O le-1.E le-2.O le-2.E le-3.O le-3.E le-4.O le-4.E \
le-5.O le-5.E ge-1.O ge-1.E ge-2.O ge-2.E ge-3.O ge-3.E ge-4.O ge-4.E ge-5.O \
ge-5.E N-1b.O N-1b.E P-1b.O P-1b.E NP-1b.O NP-1b.E NNP-1b.O NNP-1b.E N-1c.O \
N-1c.E P-1c.O P-1c.E NP-1c.O NP-1c.E NNP-1c.O NNP-1c.E N-1d.O N-1d.E P-1d.O \
P-1d.E NP-1d.O NP-1d.E NNP-1d.O NNP-1d.E N-1e.O N-1e.E P-1e.O P-1e.E NP-1e.O \
NP-1e.E NNP-1e.O NNP-1e.E N-1f.O N-1f.E P-1f.O P-1f.E NP-1f.O NP-1f.E \
NNP-1f.O NNP-1f.E N-1g.O N-1g.E P-1g.O P-1g.E NP-1g.O NP-1g.E NNP-1g.O \
NNP-1g.E N-1h.O N-1h.E P-1h.O P-1h.E NP-1h.O NP-1h.E NNP-1h.O NNP-1h.E N-1i.O \
N-1i.E P-1i.O P-1i.E NP-1i.O NP-1i.E NNP-1i.O NNP-1i.E N-1j.O N-1j.E P-1j.O \
P-1j.E NP-1j.O NP-1j.E NNP-1j.O NNP-1j.E N-1k.O N-1k.E P-1k.O P-1k.E NP-1k.O \
NP-1k.E NNP-1k.O NNP-1k.E N-streq-1.O N-streq-1.E P-streq-1.O P-streq-1.E \
NP-streq-1.O NP-streq-1.E NNP-streq-1.O NNP-streq-1.E N-streq-2.O N-streq-2.E \
P-streq-2.O P-streq-2.E NP-streq-2.O NP-streq-2.E NNP-streq-2.O NNP-streq-2.E \
N-streq-3.O N-streq-3.E P-streq-3.O P-streq-3.E NP-streq-3.O NP-streq-3.E \
NNP-streq-3.O NNP-streq-3.E N-streq-4.O N-streq-4.E P-streq-4.O P-streq-4.E \
NP-streq-4.O NP-streq-4.E NNP-streq-4.O NNP-streq-4.E N-streq-5.O N-streq-5.E \
P-streq-5.O P-streq-5.E NP-streq-5.O NP-streq-5.E NNP-streq-5.O NNP-streq-5.E \
N-streq-6.O N-streq-6.E N-strne-1.O N-strne-1.E P-strne-1.O P-strne-1.E \
NP-strne-1.O NP-strne-1.E NNP-strne-1.O NNP-strne-1.E N-strne-2.O N-strne-2.E \
P-strne-2.O P-strne-2.E NP-strne-2.O NP-strne-2.E NNP-strne-2.O NNP-strne-2.E \
N-strne-3.O N-strne-3.E P-strne-3.O P-strne-3.E NP-strne-3.O NP-strne-3.E \
NNP-strne-3.O NNP-strne-3.E N-strne-4.O N-strne-4.E P-strne-4.O P-strne-4.E \
NP-strne-4.O NP-strne-4.E NNP-strne-4.O NNP-strne-4.E N-strne-5.O N-strne-5.E \
P-strne-5.O P-strne-5.E NP-strne-5.O NP-strne-5.E NNP-strne-5.O NNP-strne-5.E \
N-strne-6.O N-strne-6.E N-and-1.O N-and-1.E P-and-1.O P-and-1.E NP-and-1.O \
NP-and-1.E NNP-and-1.O NNP-and-1.E N-and-2.O N-and-2.E P-and-2.O P-and-2.E \
NP-and-2.O NP-and-2.E NNP-and-2.O NNP-and-2.E N-and-3.O N-and-3.E P-and-3.O \
P-and-3.E NP-and-3.O NP-and-3.E NNP-and-3.O NNP-and-3.E N-and-4.O N-and-4.E \
P-and-4.O P-and-4.E NP-and-4.O NP-and-4.E NNP-and-4.O NNP-and-4.E N-or-1.O \
N-or-1.E P-or-1.O P-or-1.E NP-or-1.O NP-or-1.E NNP-or-1.O NNP-or-1.E N-or-2.O \
N-or-2.E P-or-2.O P-or-2.E NP-or-2.O NP-or-2.E NNP-or-2.O NNP-or-2.E N-or-3.O \
N-or-3.E P-or-3.O P-or-3.E NP-or-3.O NP-or-3.E NNP-or-3.O NNP-or-3.E N-or-4.O \
N-or-4.E P-or-4.O P-or-4.E NP-or-4.O NP-or-4.E NNP-or-4.O NNP-or-4.E N-eq-1.O \
N-eq-1.E P-eq-1.O P-eq-1.E NP-eq-1.O NP-eq-1.E NNP-eq-1.O NNP-eq-1.E N-eq-2.O \
N-eq-2.E P-eq-2.O P-eq-2.E NP-eq-2.O NP-eq-2.E NNP-eq-2.O NNP-eq-2.E N-eq-3.O \
N-eq-3.E P-eq-3.O P-eq-3.E NP-eq-3.O NP-eq-3.E NNP-eq-3.O NNP-eq-3.E N-eq-4.O \
N-eq-4.E P-eq-4.O P-eq-4.E NP-eq-4.O NP-eq-4.E NNP-eq-4.O NNP-eq-4.E N-eq-5.O \
N-eq-5.E P-eq-5.O P-eq-5.E NP-eq-5.O NP-eq-5.E NNP-eq-5.O NNP-eq-5.E N-eq-6.O \
N-eq-6.E P-eq-6.O P-eq-6.E NP-eq-6.O NP-eq-6.E NNP-eq-6.O NNP-eq-6.E N-gt-1.O \
N-gt-1.E P-gt-1.O P-gt-1.E NP-gt-1.O NP-gt-1.E NNP-gt-1.O NNP-gt-1.E N-gt-2.O \
N-gt-2.E P-gt-2.O P-gt-2.E NP-gt-2.O NP-gt-2.E NNP-gt-2.O NNP-gt-2.E N-gt-3.O \
N-gt-3.E P-gt-3.O P-gt-3.E NP-gt-3.O NP-gt-3.E NNP-gt-3.O NNP-gt-3.E N-gt-4.O \
N-gt-4.E P-gt-4.O P-gt-4.E NP-gt-4.O NP-gt-4.E NNP-gt-4.O NNP-gt-4.E N-gt-5.O \
N-gt-5.E P-gt-5.O P-gt-5.E NP-gt-5.O NP-gt-5.E NNP-gt-5.O NNP-gt-5.E N-lt-1.O \
N-lt-1.E P-lt-1.O P-lt-1.E NP-lt-1.O NP-lt-1.E NNP-lt-1.O NNP-lt-1.E N-lt-2.O \
N-lt-2.E P-lt-2.O P-lt-2.E NP-lt-2.O NP-lt-2.E NNP-lt-2.O NNP-lt-2.E N-lt-3.O \
N-lt-3.E P-lt-3.O P-lt-3.E NP-lt-3.O NP-lt-3.E NNP-lt-3.O NNP-lt-3.E N-lt-4.O \
N-lt-4.E P-lt-4.O P-lt-4.E NP-lt-4.O NP-lt-4.E NNP-lt-4.O NNP-lt-4.E N-lt-5.O \
N-lt-5.E P-lt-5.O P-lt-5.E NP-lt-5.O NP-lt-5.E NNP-lt-5.O NNP-lt-5.E N-t1.O \
N-t1.E P-t1.O P-t1.E NP-t1.O NP-t1.E NNP-t1.O NNP-t1.E N-t2.O N-t2.E P-t2.O \
P-t2.E NP-t2.O NP-t2.E NNP-t2.O NNP-t2.E N-paren-1.O N-paren-1.E N-paren-2.O \
N-paren-2.E N-paren-3.O N-paren-3.E N-paren-4.O N-paren-4.E N-paren-5.O \
N-paren-5.E 1a.O 1a.E 1b.O 1b.E 1c.O 1c.E 1d.O 1d.E 1e.O 1e.E 1f.O 1f.E 1g.O \
1g.E 1h.O 1h.E 1i.O 1i.E 1j.O 1j.E 1k.O 1k.E streq-1.O streq-1.E streq-2.O \
streq-2.E streq-3.O streq-3.E streq-4.O streq-4.E streq-5.O streq-5.E \
streq-6.O streq-6.E strne-1.O strne-1.E strne-2.O strne-2.E strne-3.O \
strne-3.E strne-4.O strne-4.E strne-5.O strne-5.E strne-6.O strne-6.E and-1.O \
and-1.E and-2.O and-2.E and-3.O and-3.E and-4.O and-4.E or-1.O or-1.E or-2.O \
or-2.E or-3.O or-3.E or-4.O or-4.E eq-1.O eq-1.E eq-2.O eq-2.E eq-3.O eq-3.E \
eq-4.O eq-4.E eq-5.O eq-5.E eq-6.O eq-6.E gt-1.O gt-1.E gt-2.O gt-2.E gt-3.O \
gt-3.E gt-4.O gt-4.E gt-5.O gt-5.E lt-1.O lt-1.E lt-2.O lt-2.E lt-3.O lt-3.E \
lt-4.O lt-4.E lt-5.O lt-5.E inv-1.O inv-1.E t1.O t1.E t2.O t2.E paren-1.O \
paren-1.E paren-2.O paren-2.E paren-3.O paren-3.E paren-4.O paren-4.E \
paren-5.O paren-5.E
##test-files-end

EXTRA_DIST = Test.pm $x-tests $(explicit) $(maint_gen)
noinst_SCRIPTS = $x-tests
TESTS_ENVIRONMENT = \
  PATH="`pwd`/../../src$(PATH_SEPARATOR)$$PATH"

editpl = sed -e 's,@''PERL''@,$(PERL),g' -e 's,@''srcdir''@,$(srcdir),g'

TESTS = $x-tests

mk_script = $(srcdir)/../mk-script
$(srcdir)/$x-tests: $(mk_script) Test.pm Makefile.am
	test '$x' = test && prog=../../src/$x || prog=$x; \
	$(PERL) -I$(srcdir) -w -- $(mk_script) $(srcdir) $$prog > $@.n
	mv $@.n $@
	chmod 755 $@

MAINTAINERCLEANFILES = $x-tests $(maint_gen)
CLEANFILES = $(run_gen)

check: $(maint_gen)

$(srcdir)/Makefile.am: ../Makefile.am.in Test.pm $(mk_script)
	rm -f $@ $@t
	sed -n '1,/^##test-files-begin/p' $(srcdir)/../Makefile.am.in > $@t
	tool=`echo $(subdir)|sed 's/^tests.//;s/-test//'`; \
	  echo "x = $$tool" >> $@t
	$(PERL) -I$(srcdir) -w -- $(mk_script) $(srcdir) --list >> $@t
	sed -n '/^##test-files-end/,$$p' $(srcdir)/../Makefile.am.in >> $@t
	mv $@t $@
