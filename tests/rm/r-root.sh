#!/bin/sh
# Try to remove '/' recursively.

# Copyright (C) 2013 Free Software Foundation, Inc.

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

. "${srcdir=.}/tests/init.sh"; path_prepend_ ./src
print_ver_ rm

# POSIX mandates rm(1) to skip '/' arguments.  This test verifies this mandated
# behavior as well as the --preserve-root and --no-preserve-root options.
# Especially the latter case is a live fire exercise as rm(1) is supposed to
# enter the unlinkat() system call.  Therefore, limit the risk as much
# as possible -- if there's a bug this test would wipe the system out!

# Faint-hearted: skip this test for the 'root' user.
skip_if_root_

# Pull the teeth from rm(1) by intercepting the unlinkat() system call via the
# LD_PRELOAD environment variable.  This requires shared libraries to work.
require_gcc_shared_

# This isn't terribly expensive, but it must not be run under heavy load.
# The reason is the conservative 'timeout' setting below to limit possible
# damage in the worst case which yields a race under heavy load.
# Marking this test as "expensive" therefore is a compromise, i.e., adding
# this test to the list ensures it still gets _some_ (albeit minimal)
# coverage while not causing false-positive failures in day to day runs.
expensive_

cat > k.c <<'EOF' || framework_failure_
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int unlinkat (int dirfd, const char *pathname, int flags)
{
  /* Prove that LD_PRELOAD works: create the evidence file "x".  */
  fclose (fopen ("x", "w"));

  /* Immediately terminate, unless indicated otherwise.  */
  if (! getenv("CU_TEST_SKIP_EXIT"))
    _exit (0);

  /* Pretend success.  */
  return 0;
}
EOF

# Then compile/link it:
gcc -Wall --std=gnu99 -shared -fPIC -ldl -O2 k.c -o k.so \
  || framework_failure_ 'failed to build shared library'

#-------------------------------------------------------------------------------
# exercise_rm_r_root: shell function to test "rm -r '/'"
# The caller must provide the FILE to remove as well as any options
# which should be passed to 'rm'.
# Paranoia mode on:
# For the worst case where both rm(1) would fail to refuse to process the "/"
# argument (in the cases without the --no-preserve-root option), and
# intercepting the unlinkat(1) system call would fail (which actually already
# has been proven to work above), and the current non root user has
# write access to "/", limit the damage to the current file system via
# the --one-file-system option.
# Furthermore, run rm(1) via timeout(1) that kills that process after
# a maximum of 2 seconds.
exercise_rm_r_root ()
{
  # Remove the evidence file "x"; verify that.
  rm -f x   || framework_failure_
  test -f x && framework_failure_

  local skip_exit=
  if [ "$CU_TEST_SKIP_EXIT" = 1 ]; then
    # Pass on this variable into 'rm's environment.
    skip_exit='CU_TEST_SKIP_EXIT=1'
  fi

  timeout --signal=KILL 2 \
    env LD_PRELOAD=./k.so $skip_exit \
      rm -rv --one-file-system "$@" > out 2> err

  return $?
}

# Verify that "rm -r dir" basically works.
mkdir   dir || framework_failure_
rm -r   dir || framework_failure_
test -d dir && framework_failure_

# Now verify that intercepting unlinkat() works:
# rm(1) must succeed as before, but this time both the evidence file "x"
# and the test file / directory must still exist afterward.
mkdir dir || framework_failure_
: > file  || framework_failure_

skip=
for file in dir file ; do
  exercise_rm_r_root "$file" || skip=1
  test -e "$file"            || skip=1
  test -f x                  || skip=1

  test $skip = 1 \
    && { cat out; cat err; \
         skip_ "internal test failure: maybe LD_PRELOAD doesn't work?"; }
done

# "rm -r /" without --no-preserve-root should output the following
# diagnostic error message.
cat <<EOD > exp || framework_failure_
rm: it is dangerous to operate recursively on '/'
rm: use --no-preserve-root to override this failsafe
EOD

#-------------------------------------------------------------------------------
# Exercise "rm -r /" without and with the --preserve-root option.
# Exercise various synonyms of "/" including symlinks to it.
# Expect a non-Zero exit status.
# Prepare a few symlinks to "/".
ln -s /        rootlink  || framework_failure_
ln -s rootlink rootlink2 || framework_failure_
ln -sr /       rootlink3 || framework_failure_

for opts in           \
  '/'                 \
  '--preserve-root /' \
  '//'                \
  '///'               \
  '////'              \
  'rootlink/'         \
  'rootlink2/'        \
  'rootlink3/'        ; do

  exercise_rm_r_root $opts \
    && fail=1

  # For some of the synonyms, the error diagnostic slightly differs from that
  # of the basic "/" case (see gnulib's fts_open' and ROOT_DEV_INO_WARN):
  #   rm: it is dangerous to operate recursively on 'FILE' (same as '/')
  # Strip that part off for the following comparison.
  sed "s/\(rm: it is dangerous to operate recursively on\).*$/\1 '\/'/" err \
    > err2 || framework_failure_

  # Expect nothing in 'out' and the above error diagnostic in 'err2'.
  # As rm(1) should have skipped the "/" argument, it does not call unlinkat().
  # Therefore, the evidence file "x" should not exist.
  compare /dev/null out  || fail=1
  compare exp       err2 || fail=1
  test -f x              && fail=1

  # Do nothing more if this test failed.
  test $fail = 1 && { cat out; cat err; Exit $fail; }
done

#-------------------------------------------------------------------------------
# Exercise "rm -r file1 / file2".
# Expect a non-Zero exit status representing failure to remove "/",
# yet 'file1' and 'file2' should be removed.
: > file1 || framework_failure_
: > file2 || framework_failure_

# Now that we know that 'rm' won't call the unlinkat() system function for "/",
# we could probably execute it without the LD_PRELOAD'ed safety net.
# Nevertheless, it's still better to use it for this test.
# Tell the unlinkat() replacement function to not _exit(0) immediately
# by setting the following variable.
CU_TEST_SKIP_EXIT=1

exercise_rm_r_root --preserve-root file1 '/' file2 \
  && fail=1

unset CU_TEST_SKIP_EXIT

cat <<EOD > out_removed
removed 'file1'
removed 'file2'
EOD

# The above error diagnostic should appear in 'err'.
# Both 'file1' and 'file2' should be removed.  Simply verify that in the
# "out" file, as the replacement unlinkat() dummy did not remove them.
# Expect the evidence file "x" to exist.
compare out_removed out || fail=1
compare exp         err || fail=1
test -f x               || fail=1

# Do nothing more if this test failed.
test $fail = 1 && { cat out; cat err; Exit $fail; }

#-------------------------------------------------------------------------------
# Exercise various synonyms of "/" having a trailing "." or ".." in the name.
# This triggers another check in the code first and therefore leads to a
# different diagnostic.  However, we want to test anyway to protect against
# future reordering of the checks in the code.
# Expect that other error diagnostic in 'err' and nothing in 'out'.
# Expect a non-Zero exit status.  The evidence file "x" should not exist.
for file in      \
  '//.'          \
  '/./'          \
  '/.//'         \
  '/../'         \
  '/.././'       \
  '/etc/..'      \
  'rootlink/..'  \
  'rootlink2/.'  \
  'rootlink3/./' ; do

  test -d "$file" || continue   # if e.g. /etc does not exist.

  exercise_rm_r_root --preserve-root "$file" \
    && fail=1

  grep "^rm: refusing to remove '\.' or '\.\.' directory: skipping" err \
    || fail=1

  compare /dev/null out  || fail=1
  test -f x              && fail=1

  # Do nothing more if this test failed.
  test $fail = 1 && { cat out; cat err; Exit $fail; }
done

#-------------------------------------------------------------------------------
# Until now, it was all just fun.
# Now exercise the --no-preserve-root option with which rm(1) should enter
# the intercepted unlinkat() system call.
# As the interception code terminates the process immediately via _exit(0),
# the exit status should be 0.
# Use the option --interactive=never to bypass the following prompt:
#   "rm: descend into write-protected directory '/'?"
exercise_rm_r_root  --interactive=never --no-preserve-root '/' \
  || fail=1

# The 'err' file should not contain the above error diagnostic.
grep "^rm: it is dangerous to operate recursively on '/'" err \
  && fail=1

# Instead, rm(1) should have called the intercepted unlinkat() function,
# i.e. the evidence file "x" should exist.
test -f x || fail=1

test $fail = 1 && { cat out; cat err; }

Exit $fail
