#! @PERL@ -w
# -*- perl -*-
# @configure_input@

require 5.002;

BEGIN { push @INC, '@srcdir@' if '@srcdir@' ne '.'; }
use strict;
use Test;

$| = 1;

my $xx = $ARGV[0];

print <<EOF;
#! /bin/sh
# This script was generated automatically by build-script.
case \$# in
  0) xx='$xx';;
  *) xx="\$1";;
esac
test "\$VERBOSE" && echo=echo || echo=:
\$echo testing program: \$xx
errors=0
test "\$srcdir" || srcdir=.
test "\$VERBOSE" && \$xx --version 2> /dev/null
EOF

my %seen;

my $test_vector;
foreach $test_vector (@Test::t)
  {
    my ($test_name, $input, $flags, $expected, $e_ret_code)
	= @{$test_vector};
    die "$0: $.: duplicate test name \`$test_name'\n"
      if (defined ($seen{$test_name}));
    $seen{$test_name} = 1;
    my $in = "t$test_name.in";
    my $exp_name = "t$test_name.exp";
    my $out = "t$test_name.out";

    open (IN, ">$in") || die "$0: $in: $!\n";
    print IN $input;
    close (IN) || die "$0: $in: $!\n";
    open (EXP, ">$exp_name") || die "$0: $exp_name: $!\n";
    print EXP $expected;
    close (EXP) || die "$0: $exp_name: $!\n";

    my $err_output = "t$test_name.err";
    my $cmd = "\$xx $flags \$srcdir/$in > $out 2> $err_output";
    $exp_name = "\$srcdir/$exp_name";
    print <<EOF ;
$cmd 2> /dev/null
code=\$?
if test \$code != $e_ret_code ; then
  echo Test $test_name failed: cut return code \$code differs from expected value $e_ret_code 1>&2
  errors=`expr \$errors + 1`
else
  cmp $out $exp_name
  case \$? in
    0) if test "\$VERBOSE" ; then echo passed $test_name; fi ;; # equal files
    1) echo Test $test_name failed: files $out and $exp_name differ 1>&2;
       errors=`expr \$errors + 1` ;;
    2) echo Test $test_name may have failed. 1>&2;
       echo The command \"cmp $out $exp_name\" failed. 1>&2 ;
       errors=`expr \$errors + 1` ;;
  esac
fi
EOF
  }
print <<EOF2 ;
if test \$errors = 0 ; then
  \$echo Passed all tests. 1>&2
else
  \$echo Failed \$errors tests. 1>&2
fi
test \$errors = 0 || errors=1
exit \$errors
EOF2
