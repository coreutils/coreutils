## Process this file with automake to produce Makefile.in.

##test-files-begin
x = head
explicit = in in-1024
maint_gen = idem-0.I idem-0.X idem-1.I idem-1.X idem-2.I idem-2.X idem-3.I \
idem-3.X basic-0-10.I basic-0-10.X basic-0-09.I basic-0-09.X basic-0-11.I \
basic-0-11.X obs-0.I obs-0.X obs-1.I obs-1.X obs-2.I obs-2.X obs-3.I obs-3.X \
fail-0.I fail-0.X posix-0.I posix-0.X posix-1.I posix-1.X posix-2.I posix-2.X \
posix-3.I posix-3.X
run_gen = idem-0.O idem-0.E idem-1.O idem-1.E idem-2.O idem-2.E idem-3.O \
idem-3.E basic-0-10.O basic-0-10.E basic-0-09.O basic-0-09.E basic-0-11.O \
basic-0-11.E obs-0.O obs-0.E obs-1.O obs-1.E obs-2.O obs-2.E obs-3.O obs-3.E \
obs-4.O obs-4.E obs-5.O obs-5.E fail-0.O fail-0.E posix-0.O posix-0.E \
posix-1.O posix-1.E posix-2.O posix-2.E posix-3.O posix-3.E posix-4.O \
posix-4.E posix-5.O posix-5.E
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
