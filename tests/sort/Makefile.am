## Process this file with automake to produce Makefile.in.

AUTOMAKE_OPTIONS = 1.1

x = sort
t = \
  t01a t02a t02b t02c t02d t02e t02f t03a t03b t03c t03d t03e t03f t03g \
  t03h t03i t04a t04b t04c t04d t04e t05a t05b t05c t05d t05e t05f t06a t06b \
  t06c t06d t06e t06f t07a t07b t07c t07d t08a t08b t09a t09b t09c t09d t10a \
  t10a0 t10a1 t10a2 t10b t10c t10d t10e t10f t10g t11a t11b t11c t11d t12a \
  t12b t12c t12d t13a t13b t14a t14b t15a t15b t15c t15d t15e t16a t17

in = $(t:=.in)
exp = $(t:=.exp)
out = $(t:=.out)
err = $(t:=.err)

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
