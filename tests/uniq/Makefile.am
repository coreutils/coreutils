## Process this file with automake to produce Makefile.in -*-Makefile-*-.

##test-files-begin
x = uniq
explicit =
maint_gen = 1.I 1.X 2.I 2.X 3.I 3.X 4.I 4.X 5.I 5.X 6.I 6.X 7.I 7.X 8.I 8.X \
9.I 9.X 10.I 10.X 11.I 11.X 12.I 12.X 13.I 13.X 20.I 20.X 21.I 21.X 22.I 22.X \
23.I 23.X obs30.I obs30.X 31.I 31.X 32.I 32.X 33.I 33.X 34.I 34.X 35.I 35.X \
obs40.I obs40.X obs41.I obs41.X 42.I 42.X 43.I 43.X obs44.I obs44.X obs45.I \
obs45.X 50.I 50.X 51.I 51.X 52.I 52.X 53.I 53.X 54.I 54.X 55.I 55.X 56.I 56.X \
57.I 57.X 60.I 60.X 61.I 61.X 62.I 62.X 63.I 63.X 64.I 64.X 65.I 65.X 90.I \
90.X 91.I 91.X 92.I 92.X 93.I 93.X 94.I 94.X 101.I 101.X 102.I 102.X 110.I \
110.X 111.I 111.X 112.I 112.X 113.I 113.X 114.I 114.X 115.I 115.X 116.I 116.X \
117.I 117.X 118.I 118.X 119.I 119.X
run_gen = 1.O 1.E 2.O 2.E 3.O 3.E 4.O 4.E 5.O 5.E 6.O 6.E 7.O 7.E 8.O 8.E 9.O \
9.E 10.O 10.E 11.O 11.E 12.O 12.E 13.O 13.E 20.O 20.E 21.O 21.E 22.O 22.E \
23.O 23.E obs30.O obs30.E 31.O 31.E 32.O 32.E 33.O 33.E 34.O 34.E 35.O 35.E \
obs40.O obs40.E obs41.O obs41.E 42.O 42.E 43.O 43.E obs44.O obs44.E obs45.O \
obs45.E 50.O 50.E 51.O 51.E 52.O 52.E 53.O 53.E 54.O 54.E 55.O 55.E 56.O 56.E \
57.O 57.E 60.O 60.E 61.O 61.E 62.O 62.E 63.O 63.E 64.O 64.E 65.O 65.E 90.O \
90.E 91.O 91.E 92.O 92.E 93.O 93.E 94.O 94.E 101.O 101.E 102.O 102.E 110.O \
110.E 111.O 111.E 112.O 112.E 113.O 113.E 114.O 114.E 115.O 115.E 116.O 116.E \
117.O 117.E 118.O 118.E 119.O 119.E
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
