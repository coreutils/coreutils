## Vaucanson, a generic library for finite state machines.
## Copyright (C) 2006, 2007 The Vaucanson Group.
##
## This program is free software; you can redistribute it and/or
## modify it under the terms of the GNU General Public License
## as published by the Free Software Foundation; either version 2
## of the License, or (at your option) any later version.
##
## The complete GNU General Public Licence Notice can be found as the
## `COPYING' file in the root directory.

## Override the definition from Automake to generate a log file with
## failed tests.  It also supports parallel make checks.
##
## This file provides special support for "unit tests", that is to
## say, tests that (once run) no longer need to be re-compiled and
## re-run at each "make check", unless their sources changed.  To
## enable unit-test supports, define LAZY_TEST_SUITE.  In such a
## setting, that heavily relies on correct dependencies, its users may
## prefer to define EXTRA_PROGRAMS instead of check_PROGRAMS, because
## it allows intertwined compilation and execution of the tests.
## Sometimes this helps catching errors earlier (you don't have to
## wait for all the tests to be compiled).
##
## Define TEST_SUITE_LOG to be the name of the global log to create.
## Define TEST_LOGS to the set of logs to include in it.  It defaults
## to $(TESTS:.test=.log).

## We use GNU Make extensions (%-rules), and override check-TESTS.
AUTOMAKE_OPTIONS = -Wno-portability -Wno-override

# Restructured Text title and section.
am__rst_title   = sed 's/.*/   &   /;h;s/./=/g;p;x;p;g;p;s/.*//'
am__rst_section = sed 'p;s/./=/g;p;g'

# Put stdin (possibly several lines separated by ".  ") in a box.
am__text_box = $(AWK) '{gsub ("\\.  ", "\n"); print $$0; }' |	\
$(AWK) '							\
max < length($$0) {						\
  final= final (final ? "\n" : "") " " $$0;			\
  max = length($$0);						\
}								\
END {								\
  for (i = 0; i < max + 2 ; ++i)				\
    line = line "=";						\
  print line;							\
  print final;							\
  print line;							\
}'

# If stdout is a tty, use colors.  If test -t is not supported, then
# this fails; a conservative approach.  Of course do not redirect
# stdout here, just stderr...
am__tty_colors =				\
if test -t 1 2>/dev/null; then			\
  red='[0;31m';				\
  grn='[0;32m';				\
  lgn='[1;32m';				\
  blu='[1;34m';				\
  std='[m';					\
fi

# To be inserted before the command running the test.  Stores in $dir
# the directory containing $<, and passes the TEST_ENVIRONMENT.
am__check_pre =					\
if test -f ./$<; then dir=./;			\
elif test -f $<; then dir=;			\
else dir="$(srcdir)/"; fi;			\
$(TESTS_ENVIRONMENT)

# To be appended to the command running the test.  Handles the stdout
# and stderr redirection, and catch the exit status.
am__check_post =					\
>$@-t 2>&1;						\
estatus=$$?;						\
$(am__tty_colors);					\
case $$estatus:" $(XFAIL_TESTS) " in			\
    0:*" $$(basename $<) "*) col=$$red; res=XPASS;;	\
    0:*)                     col=$$grn; res=PASS ;;	\
    77:*)                    col=$$blu; res=SKIP ;;	\
    *:*" $$(basename $<) "*) col=$$lgn; res=XFAIL;;	\
    *:*)                     col=$$red; res=FAIL ;;	\
esac;							\
echo "$${col}$$res$${std}: $$(basename $<)";		\
echo "$$res: $$(basename $<) (exit: $$estatus)" |	\
  $(am__rst_section) >$@;				\
cat $@-t >>$@;						\
rm $@-t

# From a test file to a log file.
# Do not use a regular `.test.log:' rule here, since in that case the
# following rule (without incoming extension) will mask this one.
%.log: %.test
	@$(am__check_pre) $${dir}$< $(am__check_post)

# The exact same commands, but for programs.
%.log: %$(EXEEXT)
	@$(am__check_pre) $${dir}$< $(am__check_post)

TEST_LOGS ?= $(TESTS:.test=.log)
TEST_SUITE_LOG = test-suite.log

$(TEST_SUITE_LOG): $(TEST_LOGS)
	@results=$$(for f in $(TEST_LOGS); do sed 1q $$f; done);	\
	all=$$(echo "$$results" | wc -l | sed -e 's/^[ \t]*//');	\
	fail=$$(echo "$$results" | grep -c '^FAIL');			\
	pass=$$(echo "$$results" | grep -c '^PASS');			\
	skip=$$(echo "$$results" | grep -c '^SKIP');			\
	xfail=$$(echo "$$results" | grep -c '^XFAIL');			\
	xpass=$$(echo "$$results" | grep -c '^XPASS');			\
	failures=$$(expr $$fail + $$xpass);				\
	case fail=$$fail:xpass=$$xpass:xfail=$$xfail in			\
	  fail=0:xpass=0:xfail=0)					\
	    msg="All $$all tests passed.  ";				\
	    exit=true;;							\
	  fail=0:xpass=0:xfail=*)					\
	    msg="All $$all tests behaved as expected";			\
	    msg="$$msg ($$xfail expected failures).  ";			\
	    exit=true;;							\
	  fail=*:xpass=0:xfail=*)					\
	    msg="$$fail of $$all tests failed.  ";			\
	    exit=false;;						\
	  fail=*:xpass=*:xfail=*)					\
	    msg="$$failures of $$all tests did not behave as expected";	\
	    msg="$$msg ($$xpass unexpected passes).  ";			\
	    exit=false;;						\
	  *)								\
            echo >&2 "incorrect case"; exit 4;;				\
	esac;								\
	if test "$$skip" -ne 0; then					\
	  msg="$$msg($$skip tests were not run).  ";			\
	fi;								\
	if test "$$failures" -ne 0; then				\
	  {								\
	    echo "$(PACKAGE_STRING): $(subdir)/$(TEST_SUITE_LOG)" |	\
	      $(am__rst_title);						\
	    echo "$$msg";						\
	    echo;							\
	    echo ".. contents:: :depth: 2";				\
	    echo;							\
	    for f in $(TEST_LOGS);					\
	    do								\
	      case $$(sed 1q $$f) in					\
	        SKIP:*|PASS:*|XFAIL:*);;				\
	        *) echo; cat $$f;;					\
	      esac;							\
	    done;							\
	  } >$(TEST_SUITE_LOG).tmp;					\
	  mv $(TEST_SUITE_LOG).tmp $(TEST_SUITE_LOG);			\
	  msg="$${msg}See $(subdir)/$(TEST_SUITE_LOG).  ";		\
	  if test -n "$(PACKAGE_BUGREPORT)"; then			\
	    msg="$${msg}Please report it to $(PACKAGE_BUGREPORT).  ";	\
	  fi;								\
	fi;								\
	$(am__tty_colors);						\
	if $$exit; then echo $$grn; else echo $$red; fi;		\
	  echo "$$msg" | $(am__text_box);				\
	echo $$std;							\
	test x"$$VERBOSE" = x || $$exit || cat $(TEST_SUITE_LOG);	\
	$$exit

# if test x"$$VERBOSE" != x && ! $exit; then

# Run all the tests.
check-TESTS:
	@if test -z '$(LAZY_TEST_SUITE)'; then	\
	  rm -f $(TEST_SUITE_LOG) $(TEST_LOGS);	\
	fi
	@$(MAKE) $(TEST_SUITE_LOG)


## -------------- ##
## Produce HTML.  ##
## -------------- ##

TEST_SUITE_HTML = $(TEST_SUITE_LOG:.log=.html)

%.html: %.log
	@for r2h in $(RST2HTML) $$RST2HTML rst2html rst2html.py;	\
	do								\
	  if ($$r2h --version) >/dev/null 2>&1; then			\
	    R2H=$$r2h;							\
	  fi;								\
	done;								\
	if test -z "$$R2H"; then					\
	  echo >&2 "cannot find rst2html, cannot create $@";		\
	  exit 2;							\
	fi;								\
	$$R2H $< >$@.tmp
	@mv $@.tmp $@

# Be sure to run check-TESTS first, and then to convert the result.
# Beware of concurrent executions.  And expect check-TESTS to fail.
check-html:
	@if $(MAKE) $(AM_MAKEFLAGS) check-TESTS; then :; else	\
	  rv=$$?;						\
	  $(MAKE) $(AM_MAKEFLAGS) $(TEST_SUITE_HTML);		\
	  exit $$rv;						\
	fi

.PHONY: check-html


## ------- ##
## Clean.  ##
## ------- ##

check-clean: check-clean-local
	rm -f $(CHECK_CLEANFILES) $(TEST_SUITE_LOG) $(TEST_SUITE_HTML) $(TEST_LOGS)
.PHONY: check-clean check-clean-local
clean-local: check-clean
