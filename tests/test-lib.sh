# source this file; set up for tests

# Skip this test if the shell lacks support for functions.
unset function_test
eval 'function_test() { return 11; }; function_test'
if test $? != 11; then
  echo "$0: /bin/sh lacks support for functions; skipping this test." 1>&2
  (exit 77); exit 77
fi

framework_failure()
{
  echo "$0: failure in testing framework" 1>&2
  (exit 1); exit 1
}

test_dir_=$(pwd)

this_test_() { echo "./$0" | sed 's,.*/,,'; }
this_test=$(this_test_)

. $srcdir/../envvar-check

# This is a stub function that is run upon trap (upon regular exit and
# interrupt).  Override it with a per-test function, e.g., to unmount
# a partition, or to undo any other global state changes.
cleanup_() { :; }

t_=$($abs_top_srcdir/tests/mkdtemp $test_dir_ cu-$this_test.XXXXXXXXXX) \
    || error "failed to create temporary directory in $test_dir_"

# Run each test from within a temporary sub-directory named after the
# test itself, and arrange to remove it upon exception or normal exit.
trap 'st=$?; cleanup_; d='"$t_"';
    cd '"$test_dir_"' && chmod -R u+rwx "$d" && rm -rf "$d" && exit $st' 0
trap '(exit $?); exit $?' 1 2 13 15

cd $t_ || error "failed to cd to $t_"

if ( diff --version < /dev/null 2>&1 | grep GNU ) 2>&1 > /dev/null; then
  compare() { diff -u "$@"; }
elif ( cmp --version < /dev/null 2>&1 | grep GNU ) 2>&1 > /dev/null; then
  compare() { cmp -s "$@"; }
else
  compare() { cmp "$@"; }
fi
