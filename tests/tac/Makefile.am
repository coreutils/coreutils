## Process this file with automake to produce Makefile.in -*-Makefile-*-.

##test-files-begin
x = tac
explicit =
maint_gen = basic-0.I basic-0.X basic-a.I basic-a.X basic-b.I basic-b.X \
basic-c.I basic-c.X basic-d.I basic-d.X basic-e.I basic-e.X basic-f.I \
basic-f.X basic-g.I basic-g.X basic-h.I basic-h.X basic-i.I basic-i.X \
basic-j.I basic-j.X basic-k.I basic-k.X b2-e.I b2-e.X b2-f.I b2-f.X b2-g.I \
b2-g.X b2-h.I b2-h.X b2-i.I b2-i.X b2-j.I b2-j.X b2-k.I b2-k.X
run_gen = basic-0.O basic-0.E basic-a.O basic-a.E basic-b.O basic-b.E \
basic-c.O basic-c.E basic-d.O basic-d.E basic-e.O basic-e.E basic-f.O \
basic-f.E basic-g.O basic-g.E basic-h.O basic-h.E basic-i.O basic-i.E \
basic-j.O basic-j.E basic-k.O basic-k.E b2-e.O b2-e.E b2-f.O b2-f.E b2-g.O \
b2-g.E b2-h.O b2-h.E b2-i.O b2-i.E b2-j.O b2-j.E b2-k.O b2-k.E
##test-files-end

EXTRA_DIST = mk-script.pl Test.pm $x-tests $(explicit) $(maint_gen)
noinst_SCRIPTS = $x-tests

PERL = @PERL@
editpl = sed -e 's,@''PERL''@,$(PERL),g' -e 's,@''srcdir''@,@srcdir@,g'

TESTS = $x-tests

$(srcdir)/$x-tests: @MAINT@mk-script Test.pm
	./mk-script ../../src/$x > $@.n
	mv $@.n $@
	chmod 755 $@

mk-script: ../../config.status

SUFFIXES = .pl

.pl:
	rm -f $@ $@.tmp
	$(editpl) $< > $@.tmp
	chmod +x-w $@.tmp
	mv $@.tmp $@

MAINTAINERCLEANFILES = $x-tests $(maint_gen)
CLEANFILES = $(run_gen)

@MAINT@Makefile.am: ../Makefile.am.in Test.pm mk-script
@MAINT@	rm -f $@ $@t
@MAINT@	sed -n '1,/^##test-files-begin/p' $< > $@t
@MAINT@	tool=`echo $(subdir)|sed 's/^tests.//;s/-test//'`; \
@MAINT@	  echo "x = $$tool" >> $@t
@MAINT@	./mk-script --list >> $@t
@MAINT@	sed -n '/^##test-files-end/,$$p' $< >> $@t
@MAINT@	mv $@t $@
