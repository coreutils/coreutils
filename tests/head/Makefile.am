## Process this file with automake to produce Makefile.in.

##test-files-begin
x = head
explicit =
maint_gen = idem-1.I idem-1.X idem-2.I idem-2.X idem-3.I idem-3.X idem-4.I \
idem-4.X
run_gen = idem-1.O idem-1.E idem-2.O idem-2.E idem-3.O idem-3.E idem-4.O \
idem-4.E
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
