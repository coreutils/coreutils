## Process this file with automake to produce Makefile.in -*-Makefile-*-.

##test-files-begin
x = factor
explicit =
maint_gen = 1.X 1a.X 2.X 3.X 4.X a.X b.X c.X d.X e.X f.X g.X h.X i.X j.X k.X \
l.X m.X n.X o.X p.X q.X s.X t.X u.X v.X w.X x.X y.X
run_gen = 1.O 1.E 1a.O 1a.E 2.O 2.E 3.O 3.E 4.O 4.E a.O a.E b.O b.E c.O c.E \
d.O d.E e.O e.E f.O f.E g.O g.E h.O h.E i.O i.E j.O j.E k.O k.E l.O l.E m.O \
m.E n.O n.E o.O o.E p.O p.E q.O q.E s.O s.E t.O t.E u.O u.E v.O v.E w.O w.E \
x.O x.E y.O y.E
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
