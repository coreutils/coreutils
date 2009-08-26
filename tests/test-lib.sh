# source this file; set up for tests

# Copyright (C) 2009 Free Software Foundation, Inc.

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

# Skip this test if the shell lacks support for functions.
unset function_test
eval 'function_test() { return 11; }; function_test'
if test $? != 11; then
  echo "$0: /bin/sh lacks support for functions; skipping this test." 1>&2
  Exit 77
fi

skip_test_()
{
  echo "$0: skipping test: $@" | head -1 1>&9
  echo "$0: skipping test: $@" 1>&2
  Exit 77
}

getlimits_()
{
    eval $(getlimits)
    test "$INT_MAX" ||
    error_ "Error running getlimits"
}

require_acl_()
{
  getfacl --version < /dev/null > /dev/null 2>&1 \
    && setfacl --version < /dev/null > /dev/null 2>&1 \
      || skip_test_ "This test requires getfacl and setfacl."

  id -u bin > /dev/null 2>&1 \
    || skip_test_ "This test requires a local user named bin."
}

# Skip this test if we're not in SELinux "enforcing" mode.
require_selinux_enforcing_()
{
  test "$(getenforce)" = Enforcing \
    || skip_test_ "This test is useful only with SELinux in Enforcing mode."
}


require_openat_support_()
{
  # Skip this test if your system has neither the openat-style functions
  # nor /proc/self/fd support with which to emulate them.
  test -z "$CONFIG_HEADER" \
    && skip_test_ 'internal error: CONFIG_HEADER not defined'

  _skip=yes
  grep '^#define HAVE_OPENAT' "$CONFIG_HEADER" > /dev/null && _skip=no
  test -d /proc/self/fd && _skip=no
  if test $_skip = yes; then
    skip_test_ 'this system lacks openat support'
  fi
}

require_ulimit_()
{
  ulimit_works=yes
  # Expect to be able to exec a program in 10MB of virtual memory,
  # but not in 20KB.  I chose "date".  It must not be a shell built-in
  # function, so you can't use echo, printf, true, etc.
  # Of course, in coreutils, I could use $top_builddir/src/true,
  # but this should be able to work for other projects, too.
  ( ulimit -v 10000; date ) > /dev/null 2>&1 || ulimit_works=no
  ( ulimit -v 20;    date ) > /dev/null 2>&1 && ulimit_works=no

  test $ulimit_works = no \
    && skip_test_ "this shell lacks ulimit support"
}

require_readable_root_()
{
  test -r / || skip_test_ "/ is not readable"
}

# Skip the current test if strace is not available or doesn't work
# with the named syscall.  Usage: require_strace_ unlink
require_strace_()
{
  test $# = 1 || framework_failure

  strace -V < /dev/null > /dev/null 2>&1 ||
    skip_test_ 'no strace program'

  strace -qe "$1" echo > /dev/null 2>&1 ||
    skip_test_ 'strace -qe "'"$1"'" does not work'
}

# Require a controlling input `terminal'.
require_controlling_input_terminal_()
{
  tty -s || have_input_tty=no
  test -t 0 || have_input_tty=no
  if test "$have_input_tty" = no; then
    skip_test_ 'requires controlling input terminal
This test must have a controlling input "terminal", so it may not be
run via "batch", "at", or "ssh".  On some systems, it may not even be
run in the background.'
  fi
}

require_built_()
{
  skip_=no
  for i in "$@"; do
    case " $built_programs " in
      *" $i "*) ;;
      *) echo "$i: not built" 1>&2; skip_=yes ;;
    esac
  done

  test $skip_ = yes && skip_test_ "required program(s) not built"
}

uid_is_privileged_()
{
  # Make sure id -u succeeds.
  my_uid=$(id -u) \
    || { echo "$0: cannot run \`id -u'" 1>&2; return 1; }

  # Make sure it gives valid output.
  case $my_uid in
    0) ;;
    *[!0-9]*)
      echo "$0: invalid output (\`$my_uid') from \`id -u'" 1>&2
      return 1 ;;
    *) return 1 ;;
  esac
}

get_process_status_()
{
  sed -n '/^State:[	 ]*\([[:alpha:]]\).*/s//\1/p' /proc/$1/status
}

# Convert an ls-style permission string, like drwxr----x and -rw-r-x-wx
# to the equivalent chmod --mode (-m) argument, (=,u=rwx,g=r,o=x and
# =,u=rw,g=rx,o=wx).  Ignore ACLs.
rwx_to_mode_()
{
  case $# in
    1) rwx=$1;;
    *) echo "$0: wrong number of arguments" 1>&2
      echo "Usage: $0 ls-style-mode-string" 1>&2
      return;;
  esac

  case $rwx in
    [ld-][rwx-][rwx-][rwxsS-][rwx-][rwx-][rwxsS-][rwx-][rwx-][rwxtT-]) ;;
    [ld-][rwx-][rwx-][rwxsS-][rwx-][rwx-][rwxsS-][rwx-][rwx-][rwxtT-][+.]) ;;
    *) echo "$0: invalid mode string: $rwx" 1>&2; return;;
  esac

  # Perform these conversions:
  # S  s
  # s  xs
  # T  t
  # t  xt
  # The `T' and `t' ones are only valid for `other'.
  s='s/S/@/;s/s/x@/;s/@/s/'
  t='s/T/@/;s/t/x@/;s/@/t/'

  u=`echo $rwx|sed 's/^.\(...\).*/,u=\1/;s/-//g;s/^,u=$//;'$s`
  g=`echo $rwx|sed 's/^....\(...\).*/,g=\1/;s/-//g;s/^,g=$//;'$s`
  o=`echo $rwx|sed 's/^.......\(...\).*/,o=\1/;s/-//g;s/^,o=$//;'$s';'$t`
  echo "=$u$g$o"
}

skip_if_()
{
  case $1 in
    root) skip_test_ must be run as root ;;
    non-root) skip_test_ must be run as non-root ;;
    *) ;;  # FIXME?
  esac
}

require_selinux_()
{
  case `ls -Zd .` in
    '? .'|'unlabeled .')
      skip_test_ "this system (or maybe just" \
        "the current file system) lacks SELinux support"
    ;;
  esac
}

very_expensive_()
{
  if test "$RUN_VERY_EXPENSIVE_TESTS" != yes; then
    skip_test_ 'very expensive: disabled by default
This test is very expensive, so it is disabled by default.
To run it anyway, rerun make check with the RUN_VERY_EXPENSIVE_TESTS
environment variable set to yes.  E.g.,

  env RUN_VERY_EXPENSIVE_TESTS=yes make check
'
  fi
}

expensive_()
{
  if test "$RUN_EXPENSIVE_TESTS" != yes; then
    skip_test_ 'expensive: disabled by default
This test is relatively expensive, so it is disabled by default.
To run it anyway, rerun make check with the RUN_EXPENSIVE_TESTS
environment variable set to yes.  E.g.,

  env RUN_EXPENSIVE_TESTS=yes make check
'
  fi
}

require_root_()
{
  uid_is_privileged_ || skip_test_ "must be run as root"
  NON_ROOT_USERNAME=${NON_ROOT_USERNAME=nobody}
  NON_ROOT_GROUP=${NON_ROOT_GROUP=$(id -g $NON_ROOT_USERNAME)}
}

skip_if_root_() { uid_is_privileged_ && skip_test_ "must be run as non-root"; }
error_() { echo "$0: $@" 1>&2; Exit 1; }
framework_failure() { error_ 'failure in testing framework'; }

# Set `groups' to a space-separated list of at least two groups
# of which the user is a member.
require_membership_in_two_groups_()
{
  test $# = 0 || framework_failure

  groups=${COREUTILS_GROUPS-`(id -G || /usr/xpg4/bin/id -G) 2>/dev/null`}
  case "$groups" in
    *' '*) ;;
    *) skip_test_ 'requires membership in two groups
this test requires that you be a member of more than one group,
but running `id -G'\'' either failed or found just one.  If you really
are a member of at least two groups, then rerun this test with
COREUTILS_GROUPS set in your environment to the space-separated list
of group names or numbers.  E.g.,

  env COREUTILS_GROUPS='users cdrom' make check

'
     ;;
  esac
}

# Is /proc/$PID/status supported?
require_proc_pid_status_()
{
    sleep 2 &
    local pid=$!
    sleep .5
    grep '^State:[	 ]*[S]' /proc/$pid/status > /dev/null 2>&1 ||
    skip_test_ "/proc/$pid/status: missing or 'different'"
    kill $pid
}

# Does the current (working-dir) file system support sparse files?
require_sparse_support_()
{
  test $# = 0 || framework_failure
  # Test whether we can create a sparse file.
  # For example, on Darwin6.5 with a file system of type hfs, it's not possible.
  # NTFS requires 128K before a hole appears in a sparse file.
  t=sparse.$$
  dd bs=1 seek=128K of=$t < /dev/null 2> /dev/null
  set x `du -sk $t`
  kb_size=$2
  rm -f $t
  if test $kb_size -ge 128; then
    skip_test_ 'this file system does not support sparse files'
  fi
}

mkfifo_or_skip_()
{
  test $# = 1 || framework_failure
  if ! mkfifo "$1"; then
    # Make an exception of this case -- usually we interpret framework-creation
    # failure as a test failure.  However, in this case, when running on a SunOS
    # system using a disk NFS mounted from OpenBSD, the above fails like this:
    # mkfifo: cannot make fifo `fifo-10558': Not owner
    skip_test_ 'NOTICE: unable to create test prerequisites'
  fi
}

# Disable the current test if the working directory seems to have
# the setgid bit set.
skip_if_setgid_()
{
  setgid_tmpdir=setgid-$$
  (umask 77; mkdir $setgid_tmpdir)
  perms=$(stat --printf %A $setgid_tmpdir)
  rmdir $setgid_tmpdir
  case $perms in
    drwx------);;
    drwxr-xr-x);;  # Windows98 + DJGPP 2.03
    *) skip_test_ 'this directory has the setgid bit set';;
  esac
}

skip_if_mcstransd_is_running_()
{
  test $# = 0 || framework_failure

  # When mcstransd is running, you'll see only the 3-component
  # version of file-system context strings.  Detect that,
  # and if it's running, skip this test.
  __ctx=$(stat --printf='%C\n' .) || framework_failure
  case $__ctx in
    *:*:*:*) ;; # four components is ok
    *) # anything else probably means mcstransd is running
        skip_test_ "unexpected context '$__ctx'; turn off mcstransd" ;;
  esac
}

# Skip the current test if umask doesn't work as usual.
# This test should be run in the temporary directory that ends
# up being removed via the trap commands.
working_umask_or_skip_()
{
  umask 022
  touch file1 file2
  chmod 644 file2
  perms=`ls -l file1 file2 | sed 's/ .*//' | uniq`
  rm -f file1 file2

  case $perms in
  *'
  '*) skip_test_ 'your build directory has unusual umask semantics'
  esac
}

# We use a trap below for cleanup.  This requires us to go through
# hoops to get the right exit status transported through the signal.
# So use `Exit STATUS' instead of `exit STATUS' inside of the tests.
# Turn off errexit here so that we don't trip the bug with OSF1/Tru64
# sh inside this function.
Exit ()
{
  set +e
  (exit $1)
  exit $1
}

test_dir_=$(pwd)

this_test_() { echo "./$0" | sed 's,.*/,,'; }
this_test=$(this_test_)

# This is a stub function that is run upon trap (upon regular exit and
# interrupt).  Override it with a per-test function, e.g., to unmount
# a partition, or to undo any other global state changes.
cleanup_() { :; }

t_=$("$abs_top_builddir/src/mktemp" -d --tmp="$test_dir_" cu-$this_test.XXXXXXXXXX)\
    || error_ "failed to create temporary directory in $test_dir_"

remove_tmp_()
{
  __st=$?
  cleanup_
  cd "$test_dir_" && chmod -R u+rwx "$t_" && rm -rf "$t_" && exit $__st
}

# Run each test from within a temporary sub-directory named after the
# test itself, and arrange to remove it upon exception or normal exit.
trap remove_tmp_ 0
trap 'Exit $?' 1 2 13 15

cd "$t_" || error_ "failed to cd to $t_"

if ( diff --version < /dev/null 2>&1 | grep GNU ) 2>&1 > /dev/null; then
  compare() { diff -u "$@"; }
elif ( cmp --version < /dev/null 2>&1 | grep GNU ) 2>&1 > /dev/null; then
  compare() { cmp -s "$@"; }
else
  compare() { cmp "$@"; }
fi
