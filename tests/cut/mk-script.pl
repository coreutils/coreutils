#! @PERL@ -w
# -*- perl -*-
# @configure_input@

require 5.002;
use strict;
use POSIX qw (assert);

BEGIN { push @INC, '@srcdir@' if '@srcdir@' ne '.'; }
use Test;

sub spec_to_list ($$$)
{
  my ($spec, $test_name, $type) = @_;

  assert ($type eq 'in' || $type eq 'exp');

  my @all_file;
  my @gen_file;
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
	      push (@all_file, $existing_file);
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
      my $gen_file = "t$test_name.$type$i";
      push (@all_file, $gen_file);
      push (@gen_file, $gen_file);
      open (F, ">$gen_file") || die "$0: $gen_file: $!\n";
      print F $file_contents;
      close (F) || die "$0: $gen_file: $!\n";
      ++$i;
    }

  my %h = (
    ALL_FILES => \@all_file,
    GEN_FILES => \@gen_file
  );

  return \%h;
}

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

my $test_vector;
foreach $test_vector (@Test::t)
  {
    my ($test_name, $flags, $in_spec, $expected, $e_ret_code)
	= @{$test_vector};

    my $in = spec_to_list ($in_spec, $test_name, 'in');

    my $exp_name = "t$test_name.exp";
    my $out = "t$test_name.out";

    open (EXP, ">$exp_name") || die "$0: $exp_name: $!\n";
    print EXP $expected;
    close (EXP) || die "$0: $exp_name: $!\n";

    my $err_output = "t$test_name.err";

    my @srcdir_rel_in_file;
    my $f;
    foreach $f (@{ $in->{ALL_FILES} })
      {
	push (@srcdir_rel_in_file, "\$srcdir/$f")
      }

    my $cmd = "\$xx $flags " . join (' ', @srcdir_rel_in_file)
      . " > $out 2> $err_output";
    $exp_name = "\$srcdir/$exp_name";
    print <<EOF ;
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
