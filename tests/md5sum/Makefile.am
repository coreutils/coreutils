## Process this file with automake to produce Makefile.in -*-Makefile-*-.

##test-files-begin
x = md5sum
explicit =
maint_gen = 1.I 1.X 2.I 2.X 3.I 3.X 4.I 4.X 5.I 5.X 6.I 6.X 7.I 7.X
run_gen = 1.O 1.E 2.O 2.E 3.O 3.E 4.O 4.E 5.O 5.E 6.O 6.E 7.O 7.E
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
