## Process this file with automake to produce Makefile.in -*-Makefile-*-.

##test-files-begin
x = cut
explicit =
maint_gen = 1.I 1.X 2.I 2.X 3.I 3.X 4.I 4.X 5.I 5.X 6.I 6.X 7.I 7.X 8.I 8.X \
9.I 9.X a.I a.X b.I b.X c.I c.X d.I d.X e.I e.X f.I f.X g.I g.X h.I h.X i.I \
i.X j.I j.X k.I k.X l.I l.X m.I m.X n.I n.X o.I o.X p.I p.X q.I q.X r.I r.X \
s.I s.X t.I t.X u.I u.X v.I v.X w.I w.X x.I x.X y.I y.X z.I z.X empty-fl.I \
empty-fl.X missing-fl.I missing-fl.X empty-bl.I empty-bl.X missing-bl.I \
missing-bl.X empty-f1.I empty-f1.X empty-f2.I empty-f2.X o-delim.I o-delim.X \
nul-idelim.I nul-idelim.X nul-odelim.I nul-odelim.X multichar-od.I \
multichar-od.X 8bit-delim.I 8bit-delim.X out-delim1.I out-delim1.X \
out-delim2.I out-delim2.X out-delim3.I out-delim3.X out-delim4.I out-delim4.X \
out-delim5.I out-delim5.X
run_gen = 1.O 1.E 2.O 2.E 3.O 3.E 4.O 4.E 5.O 5.E 6.O 6.E 7.O 7.E 8.O 8.E 9.O \
9.E a.O a.E b.O b.E c.O c.E d.O d.E e.O e.E f.O f.E g.O g.E h.O h.E i.O i.E \
j.O j.E k.O k.E l.O l.E m.O m.E n.O n.E o.O o.E p.O p.E q.O q.E r.O r.E s.O \
s.E t.O t.E u.O u.E v.O v.E w.O w.E x.O x.E y.O y.E z.O z.E empty-fl.O \
empty-fl.E missing-fl.O missing-fl.E empty-bl.O empty-bl.E missing-bl.O \
missing-bl.E empty-f1.O empty-f1.E empty-f2.O empty-f2.E o-delim.O o-delim.E \
nul-idelim.O nul-idelim.E nul-odelim.O nul-odelim.E multichar-od.O \
multichar-od.E 8bit-delim.O 8bit-delim.E out-delim1.O out-delim1.E \
out-delim2.O out-delim2.E out-delim3.O out-delim3.E out-delim4.O out-delim4.E \
out-delim5.O out-delim5.E
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
