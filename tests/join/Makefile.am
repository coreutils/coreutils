## Process this file with automake to produce Makefile.in.

x = join
t = t1a t1b t1c t1d t1e t1f t2a t2b t2c t3a t4a t4b t4c t4d t4e t5a t5b \
t5c t5d t5e t5f t5g t5h t5i t5j t5k t5l t5m t6a t6b t6c t7a t8a t8b t9a

in1 = $(t:=.in1)
in2 = $(t:=.in2)
exp = $(t:=.exp)
out = $(t:=.out)
err = $(t:=.err)

EXTRA_DIST = build-script.pl Test.pm $x-tests $(in1) $(in2) $(exp)
noinst_SCRIPTS = $x-tests build-script

PERL = @PERL@
editpl = sed -e 's,@''PERL''@,$(PERL),g' -e 's,@''srcdir''@,@srcdir@,g'

TESTS = $x-tests

$x-tests: @MAINT@build-script Test.pm
	./build-script ../../src/$x > $@.n
	mv $@.n $@
	chmod 755 $@

SUFFIXES = .pl

.pl:
	rm -f $@ $@.tmp
	$(editpl) $< > $@.tmp && chmod +x $@.tmp && mv $@.tmp $@

MAINTAINERCLEANFILES = $x-tests $(in) $(exp)
CLEANFILES = $(out) $(err)
