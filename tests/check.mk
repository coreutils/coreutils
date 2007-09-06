# FIXME: add comment

# Ensure that all version-controlled executable files are listed in TESTS.
_v = TESTS
FIXME_hook_this_to_make_distcheck:
	sed -n '/^$(_v) =/,/[^\]$$/p' $(srcdir)/Makefile.am \
	  | sed 's/^  *//;/^\$$.*/d;/^$(_v) =/d' \
	  | tr -s '\012\\' '  ' | fmt -1 | sort -u > t1
	find `$(top_srcdir)/build-aux/vc-list-files $(srcdir)` \
	    -type f -perm -111 -printf '%f\n'|sort -u \
	  | diff -u t1 -

# Append this, because automake does the same.
TESTS_ENVIRONMENT +=			\
  abs_top_srcdir=$(abs_top_srcdir)	\
  abs_top_builddir=$(abs_top_builddir)	\
  srcdir=$(srcdir)

TEST_LOGS = $(TESTS:=.log)

# Parallel replacement of Automake's check-TESTS target.
# Include it last.
include $(top_srcdir)/build-aux/check.mk
