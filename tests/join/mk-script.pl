#! @PERL@ -w
# -*- perl -*-
# @configure_input@

require 5.002;
use strict;
use POSIX qw (assert);

BEGIN { push @INC, '@srcdir@' if '@srcdir@' ne '.'; }
use Test;

sub validate
{
  my %seen;
  my $test_vector;
  foreach $test_vector (@Test::t)
    {
      my ($test_name, $flags, $in_spec, $expected, $e_ret_code, $rest) =
	@{$test_vector};
      assert (defined $e_ret_code && !defined $rest);
      assert (!ref $test_name);
      assert (!ref $flags);
      assert (!ref $expected);
      assert (!ref $e_ret_code);

      die "$0: $.: duplicate test name \`$test_name'\n"
	if (defined $seen{$test_name});
      $seen{$test_name} = 1;
    }
}

# Given a spec for the input file(s) or expected output file of a single
# test, create a file for any string.  A file is created for each literal
# string -- not for named files.  Whether a perl `string' is treated as
# a string to be put in a file for a test or the name of an existing file
# depends on how many references have to be traversed to get from
# the top level variable to the actual string literal.
# If $SPEC is a literal Perl string (not a reference), then treat $SPEC
# as the contents of a file.
# If $SPEC is an array reference, consider each element of the array.
# If the element is a string reference, treat the string as the name of
# an existing file.  Otherwise, the element must be a string and is treated
# just like a scalar $SPEC.  When a file is created, its name is derived
# from the name TEST_NAME of the corresponding test and the TYPE of file.
# E.g., the inputs for test `3a' would be named t3a.in1 and  t3a.in2, and
# the expected output for test `7c' would be named t7c.exp.
#
# Also, return two lists of file names:
# - maintainer-generated files -- names of files created by this function
# - files named explicitly in Test.pm

sub spec_to_list ($$$)
{
  my ($spec, $test_name, $type) = @_;

  assert ($type eq 'in' || $type eq 'exp');

  my @explicit_file;
  my @maint_gen_file;
  my @content_string;
  if (ref $spec)
    {
      assert (ref $spec eq 'ARRAY');
      my $file_spec;
      foreach $file_spec (@$spec)
	{
	  # A file spec may be a string or a reference.
	  # If it's a string, that string is to be the contents of a
	  # generated (by this script) file with name derived from the
	  # name of this test.
	  # If it's a reference, then it must be the name of an existing
	  # file.
	  if (ref $file_spec)
	    {
	      assert (ref $file_spec eq 'SCALAR');
	      my $existing_file = $$file_spec;
	      # FIXME: make sure $existing_file exists somewhere.
	      push (@explicit_file, $existing_file);
	    }
	  else
	    {
	      push (@content_string, $file_spec);
	    }
	}
    }
  else
    {
      push (@content_string, $spec);
    }

  my $i = 1;
  my $file_contents;
  foreach $file_contents (@content_string)
    {
      my $suffix = (@content_string > 1 ? $i : '');
      my $maint_gen_file = "t$test_name.$type$suffix";
      push (@maint_gen_file, $maint_gen_file);
      open (F, ">$maint_gen_file") || die "$0: $maint_gen_file: $!\n";
      print F $file_contents;
      close (F) || die "$0: $maint_gen_file: $!\n";
      ++$i;
    }

  my %h = (
    EXPLICIT => \@explicit_file,
    MAINT_GEN => \@maint_gen_file
  );

  return \%h;
}

# ~~~~~~~ main ~~~~~~~~
{
  $| = 1;

  die "Usage: $0: program-name\n" if @ARGV != 1;

  my $xx = $ARGV[0];

  if ($xx eq '--list-generated')
    {
      validate ();
      # FIXME !!!!!!!!!!!!!!!!!!!!!!!!!!!!
      # Output two lists of files:
      # MAINT_GEN -- maintainer-generated files
      # RUN_GEN -- files created when running the tests
      exit 0;
    }

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

validate ();

my $test_vector;
foreach $test_vector (@Test::t)
{
  my ($test_name, $flags, $in_spec, $exp_spec, $e_ret_code)
    = @{$test_vector};

  my $in = spec_to_list ($in_spec, $test_name, 'in');

  my @srcdir_rel_in_file;
  my $f;
  foreach $f (@{$in->{EXPLICIT}}, @{$in->{MAINT_GEN}})
    {
      push (@srcdir_rel_in_file, "\$srcdir/$f");
    }

  my $exp = spec_to_list ($exp_spec, $test_name, 'exp');
  my @all = (@{$exp->{EXPLICIT}}, @{$exp->{MAINT_GEN}});
  assert (@all == 1);
  my $exp_name = "\$srcdir/$all[0]";
  my $out = "t$test_name.out";
  my $err_output = "t$test_name.err";

  my $redirect_stdin = ((@srcdir_rel_in_file == 1
			 && defined $Test::input_via_stdin
			 && $Test::input_via_stdin)
			? '< ' : '');
  my $cmd = "\$xx $flags $redirect_stdin" . join (' ', @srcdir_rel_in_file)
    . " > $out 2> $err_output";
  print <<EOF;
$cmd
code=\$?
if test \$code != $e_ret_code ; then
  \$echo Test $test_name failed: $xx return code \$code differs from expected value $e_ret_code 1>&2
  errors=`expr \$errors + 1`
else
  cmp $out $exp_name
  case \$? in
    0) if test "\$VERBOSE" ; then \$echo passed $test_name; fi ;; # equal files
    1) \$echo Test $test_name failed: files $out and $exp_name differ 1>&2;
       errors=`expr \$errors + 1` ;;
    2) \$echo Test $test_name may have failed. 1>&2;
       \$echo The command \"cmp $out $exp_name\" failed. 1>&2 ;
       errors=`expr \$errors + 1` ;;
  esac
fi
test -s $err_output || rm -f $err_output
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
}
