## Process this file with automake to produce Makefile.in -*-Makefile-*-.

##test-files-begin
x = join
explicit =
maint_gen = 1a.I1 1a.I2 1a.X 1b.I1 1b.I2 1b.X 1c.I1 1c.I2 1c.X 1d.I1 1d.I2 \
1d.X 1e.I1 1e.I2 1e.X 1f.I1 1f.I2 1f.X 2a.I1 2a.I2 2a.X 2b.I1 2b.I2 2b.X \
2c.I1 2c.I2 2c.X 3a.I1 3a.I2 3a.X 4a.I1 4a.I2 4a.X 4b.I1 4b.I2 4b.X 4c.I1 \
4c.I2 4c.X 4d.I1 4d.I2 4d.X 4e.I1 4e.I2 4e.X 5a.I1 5a.I2 5a.X 5b.I1 5b.I2 \
5b.X 5c.I1 5c.I2 5c.X 5d.I1 5d.I2 5d.X 5e.I1 5e.I2 5e.X 5f.I1 5f.I2 5f.X \
5g.I1 5g.I2 5g.X 5h.I1 5h.I2 5h.X 5i.I1 5i.I2 5i.X 5j.I1 5j.I2 5j.X 5k.I1 \
5k.I2 5k.X 5l.I1 5l.I2 5l.X 5m.I1 5m.I2 5m.X 6a.I1 6a.I2 6a.X 6b.I1 6b.I2 \
6b.X 6c.I1 6c.I2 6c.X 7a.I1 7a.I2 7a.X 8a.I1 8a.I2 8a.X 8b.I1 8b.I2 8b.X \
9a.I1 9a.I2 9a.X trailing-sp.I1 trailing-sp.I2 trailing-sp.X sp-vs-blank.I1 \
sp-vs-blank.I2 sp-vs-blank.X 8-bit-t.I1 8-bit-t.I2 8-bit-t.X invalid-j.X
run_gen = 1a.O 1a.E 1b.O 1b.E 1c.O 1c.E 1d.O 1d.E 1e.O 1e.E 1f.O 1f.E 2a.O \
2a.E 2b.O 2b.E 2c.O 2c.E 3a.O 3a.E 4a.O 4a.E 4b.O 4b.E 4c.O 4c.E 4d.O 4d.E \
4e.O 4e.E 5a.O 5a.E 5b.O 5b.E 5c.O 5c.E 5d.O 5d.E 5e.O 5e.E 5f.O 5f.E 5g.O \
5g.E 5h.O 5h.E 5i.O 5i.E 5j.O 5j.E 5k.O 5k.E 5l.O 5l.E 5m.O 5m.E 6a.O 6a.E \
6b.O 6b.E 6c.O 6c.E 7a.O 7a.E 8a.O 8a.E 8b.O 8b.E 9a.O 9a.E trailing-sp.O \
trailing-sp.E sp-vs-blank.O sp-vs-blank.E 8-bit-t.O 8-bit-t.E invalid-j.O \
invalid-j.E
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
