## Process this file with automake to produce Makefile.in -*-Makefile-*-.

##test-files-begin
x = date
explicit =
maint_gen = 1.X 3.X 4.X 5.X 6.X 7.X 8.X 9.X leap-1.X U95-1.X U95-2.X U95-3.X \
U92-1.X U92-2.X U92-3.X V92-1.X V92-2.X V92-3.X W92-1.X W92-2.X W92-3.X \
millen-1.X rel-0.X rel-1a.X rel-1b.X rel-2a.X rel-2b.X rel-2c.X rel-2d.X \
rel-2e.X rel-2f.X rel-2g.X rel-3a.X next-s.X next-m.X next-h.X next-d.X \
next-w.X next-mo.X next-y.X utc-0.X utc-0a.X utc-1.I utc-1.X utc-1a.I \
utc-1a.X date2sec-0.X date2sec-0a.X date2sec-1.X sec2date-0.X this-m.X \
this-h.X this-w.X this-mo.X this-y.X risks-1.X regress-1.X datevtime-1.X \
rfc822-1.X relative-1.X relative-2.X wide-fmt.X
run_gen = 1.O 1.E 3.O 3.E 4.O 4.E 5.O 5.E 6.O 6.E 7.O 7.E 8.O 8.E 9.O 9.E \
leap-1.O leap-1.E U95-1.O U95-1.E U95-2.O U95-2.E U95-3.O U95-3.E U92-1.O \
U92-1.E U92-2.O U92-2.E U92-3.O U92-3.E V92-1.O V92-1.E V92-2.O V92-2.E \
V92-3.O V92-3.E W92-1.O W92-1.E W92-2.O W92-2.E W92-3.O W92-3.E millen-1.O \
millen-1.E rel-0.O rel-0.E rel-1a.O rel-1a.E rel-1b.O rel-1b.E rel-2a.O \
rel-2a.E rel-2b.O rel-2b.E rel-2c.O rel-2c.E rel-2d.O rel-2d.E rel-2e.O \
rel-2e.E rel-2f.O rel-2f.E rel-2g.O rel-2g.E rel-3a.O rel-3a.E next-s.O \
next-s.E next-m.O next-m.E next-h.O next-h.E next-d.O next-d.E next-w.O \
next-w.E next-mo.O next-mo.E next-y.O next-y.E utc-0.O utc-0.E utc-0a.O \
utc-0a.E utc-1.O utc-1.E utc-1a.O utc-1a.E date2sec-0.O date2sec-0.E \
date2sec-0a.O date2sec-0a.E date2sec-1.O date2sec-1.E sec2date-0.O \
sec2date-0.E this-m.O this-m.E this-h.O this-h.E this-w.O this-w.E this-mo.O \
this-mo.E this-y.O this-y.E risks-1.O risks-1.E regress-1.O regress-1.E \
datevtime-1.O datevtime-1.E rfc822-1.O rfc822-1.E relative-1.O relative-1.E \
relative-2.O relative-2.E wide-fmt.O wide-fmt.E
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
