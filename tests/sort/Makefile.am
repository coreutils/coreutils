## Process this file with automake to produce Makefile.in.

AUTOMAKE_OPTIONS = 1.1

x = sort
in = \
  t01a.in t02a.in t02b.in t02c.in t02d.in t02e.in t02f.in t03a.in \
  t03b.in t03c.in t03d.in t03e.in t03f.in t03g.in t03h.in t03i.in \
  t04a.in t04b.in t04c.in t04d.in t04e.in t05a.in t05b.in t05c.in \
  t05d.in t05e.in t05f.in t06a.in t06b.in t06c.in t06d.in t06e.in \
  t06f.in t07a.in t07b.in t07c.in t07d.in t08a.in t08b.in t09a.in \
  t09b.in t09c.in t09d.in t10a.in t10a0.in t10a1.in t10a2.in t10b.in \
  t10c.in t10d.in t10e.in t10f.in t10g.in t11a.in t11b.in t11c.in \
  t11d.in t12a.in t12b.in t12c.in t12d.in t13a.in t13b.in t14a.in \
  t14b.in t15a.in t15b.in t15c.in t15d.in t15e.in

exp = $(in:.in=.exp)
out = $(in:.in=.out)
err = $(in:.in=.err)

EXTRA_DIST = build-script.pl Test.pm $x-tests $(in) $(exp)
noinst_SCRIPTS = $x-tests

PERL = @PERL@
editpl = sed -e 's,@''PERL''@,$(PERL),g' -e 's,@''srcdir''@,@srcdir@,g'

TESTS = $x-tests

$x-tests: @MAINT@ build-script Test.pm
	./build-script ../../src/$x > $@.n
	mv $@.n $@
	chmod 755 $@

SUFFIXES = .pl

.pl:
	rm -f $@ $@.tmp
	$(editpl) $< > $@.tmp && chmod +x $@.tmp && mv $@.tmp $@

MAINTAINERCLEANFILES = $x-tests $(in) $(exp)
CLEANFILES = $(out) $(err)
