## Process this file with automake to produce Makefile.in -*-Makefile-*-.

##test-files-begin
x = test
explicit =
maint_gen = ne-1.X ne-2.X ne-3.X ne-4.X ne-5.X le-1.X le-2.X le-3.X le-4.X \
ge-1.X ge-2.X ge-3.X ge-4.X N-1b.X NN-1b.X N-1c.X NN-1c.X N-1d.X NN-1d.X \
N-streq-1.X NN-streq-1.X N-streq-2.X NN-streq-2.X N-strne-1.X NN-strne-1.X \
N-strne-2.X NN-strne-2.X N-and-1.X NN-and-1.X N-and-2.X NN-and-2.X N-and-3.X \
NN-and-3.X N-and-4.X NN-and-4.X N-or-1.X NN-or-1.X N-or-2.X NN-or-2.X \
N-or-3.X NN-or-3.X N-or-4.X NN-or-4.X N-eq-1.X NN-eq-1.X N-eq-2.X NN-eq-2.X \
N-eq-3.X NN-eq-3.X N-eq-4.X NN-eq-4.X N-eq-5.X NN-eq-5.X N-gt-1.X NN-gt-1.X \
N-gt-2.X NN-gt-2.X N-gt-3.X NN-gt-3.X N-gt-4.X NN-gt-4.X N-lt-1.X NN-lt-1.X \
N-lt-2.X NN-lt-2.X N-lt-3.X NN-lt-3.X N-lt-4.X NN-lt-4.X N-t2.X NN-t2.X 1a.X \
1b.X 1c.X 1d.X streq-1.X streq-2.X strne-1.X strne-2.X and-1.X and-2.X \
and-3.X and-4.X or-1.X or-2.X or-3.X or-4.X eq-1.X eq-2.X eq-3.X eq-4.X \
eq-5.X gt-1.X gt-2.X gt-3.X gt-4.X lt-1.X lt-2.X lt-3.X lt-4.X inv-1.X t1.X \
t2.X
run_gen = ne-1.O ne-1.E ne-2.O ne-2.E ne-3.O ne-3.E ne-4.O ne-4.E ne-5.O \
ne-5.E le-1.O le-1.E le-2.O le-2.E le-3.O le-3.E le-4.O le-4.E ge-1.O ge-1.E \
ge-2.O ge-2.E ge-3.O ge-3.E ge-4.O ge-4.E N-1b.O N-1b.E NN-1b.O NN-1b.E \
N-1c.O N-1c.E NN-1c.O NN-1c.E N-1d.O N-1d.E NN-1d.O NN-1d.E N-streq-1.O \
N-streq-1.E NN-streq-1.O NN-streq-1.E N-streq-2.O N-streq-2.E NN-streq-2.O \
NN-streq-2.E N-strne-1.O N-strne-1.E NN-strne-1.O NN-strne-1.E N-strne-2.O \
N-strne-2.E NN-strne-2.O NN-strne-2.E N-and-1.O N-and-1.E NN-and-1.O \
NN-and-1.E N-and-2.O N-and-2.E NN-and-2.O NN-and-2.E N-and-3.O N-and-3.E \
NN-and-3.O NN-and-3.E N-and-4.O N-and-4.E NN-and-4.O NN-and-4.E N-or-1.O \
N-or-1.E NN-or-1.O NN-or-1.E N-or-2.O N-or-2.E NN-or-2.O NN-or-2.E N-or-3.O \
N-or-3.E NN-or-3.O NN-or-3.E N-or-4.O N-or-4.E NN-or-4.O NN-or-4.E N-eq-1.O \
N-eq-1.E NN-eq-1.O NN-eq-1.E N-eq-2.O N-eq-2.E NN-eq-2.O NN-eq-2.E N-eq-3.O \
N-eq-3.E NN-eq-3.O NN-eq-3.E N-eq-4.O N-eq-4.E NN-eq-4.O NN-eq-4.E N-eq-5.O \
N-eq-5.E NN-eq-5.O NN-eq-5.E N-gt-1.O N-gt-1.E NN-gt-1.O NN-gt-1.E N-gt-2.O \
N-gt-2.E NN-gt-2.O NN-gt-2.E N-gt-3.O N-gt-3.E NN-gt-3.O NN-gt-3.E N-gt-4.O \
N-gt-4.E NN-gt-4.O NN-gt-4.E N-lt-1.O N-lt-1.E NN-lt-1.O NN-lt-1.E N-lt-2.O \
N-lt-2.E NN-lt-2.O NN-lt-2.E N-lt-3.O N-lt-3.E NN-lt-3.O NN-lt-3.E N-lt-4.O \
N-lt-4.E NN-lt-4.O NN-lt-4.E N-t2.O N-t2.E NN-t2.O NN-t2.E 1a.O 1a.E 1b.O \
1b.E 1c.O 1c.E 1d.O 1d.E streq-1.O streq-1.E streq-2.O streq-2.E strne-1.O \
strne-1.E strne-2.O strne-2.E and-1.O and-1.E and-2.O and-2.E and-3.O and-3.E \
and-4.O and-4.E or-1.O or-1.E or-2.O or-2.E or-3.O or-3.E or-4.O or-4.E \
eq-1.O eq-1.E eq-2.O eq-2.E eq-3.O eq-3.E eq-4.O eq-4.E eq-5.O eq-5.E gt-1.O \
gt-1.E gt-2.O gt-2.E gt-3.O gt-3.E gt-4.O gt-4.E lt-1.O lt-1.E lt-2.O lt-2.E \
lt-3.O lt-3.E lt-4.O lt-4.E inv-1.O inv-1.E t1.O t1.E t2.O t2.E
##test-files-end

EXTRA_DIST = Test.pm $x-tests $(explicit) $(maint_gen)
noinst_SCRIPTS = $x-tests

PERL = @PERL@
editpl = sed -e 's,@''PERL''@,$(PERL),g' -e 's,@''srcdir''@,@srcdir@,g'

TESTS = $x-tests

mk_script = $(srcdir)/../mk-script
$(srcdir)/$x-tests: $(mk_script) Test.pm
	$(PERL) -I$(srcdir) -w -- $(mk_script) ../../src/$x > $@.n
	mv $@.n $@
	chmod 755 $@

MAINTAINERCLEANFILES = $x-tests $(maint_gen)
CLEANFILES = $(run_gen)

check: $(maint_gen)

Makefile.am: ../Makefile.am.in Test.pm $(mk_script)
	rm -f $@ $@t
	sed -n '1,/^##test-files-begin/p' $< > $@t
	tool=`echo $(subdir)|sed 's/^tests.//;s/-test//'`; \
	  echo "x = $$tool" >> $@t
	$(PERL) -I$(srcdir) -w -- $(mk_script) --list >> $@t
	sed -n '/^##test-files-end/,$$p' $< >> $@t
	mv $@t $@
