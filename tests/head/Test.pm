package Test;
require 5.002;
use strict;

my @tv = (
# test name, options, input, expected output, expected return code
#
['idem-0', '', "", "", 0],
['idem-1', '', "a", "a", 0],
['idem-2', '', "\n", "\n", 0],
['idem-3', '', "a\n", "a\n", 0],

['basic-0-10', '',
 "1\n2\n3\n4\n5\n6\n7\n8\n9\n0\n",
 "1\n2\n3\n4\n5\n6\n7\n8\n9\n0\n", 0],

['basic-0-09', '',
 "1\n2\n3\n4\n5\n6\n7\n8\n9\n",
 "1\n2\n3\n4\n5\n6\n7\n8\n9\n", 0],

['basic-0-11', '',
 "1\n2\n3\n4\n5\n6\n7\n8\n9\n0\nb\n",
 "1\n2\n3\n4\n5\n6\n7\n8\n9\n0\n", 0],

['obs-0', '-1', "1\n2\n", "1\n", 0],
['obs-1', '-1c', "", "", 0],
['obs-2', '-1c', "12", "1", 0],
['obs-3', '-14c', "1234567890abcdefg", "1234567890abcd", 0],

);

sub test_vector
{
  my $t;
  my @derived_tests;
  foreach $t (@tv)
    {
      my ($test_name, $flags, $in, $exp, $ret) = @$t;

      # Derive equivalent, posix-style tests from the obsolescent ones.
      next if $test_name !~ /^obs-/;

      $test_name =~ s/^obs-/posix-/;
      if ($flags =~ /-(\d+)$/)
	{
	  $flags = "-n $1";
	}
      elsif ($flags =~ /-(\d+)([cbk])$/)
	{
	  my $suffix = $2;
	  $suffix = '' if $suffix eq 'c';
	  $flags = "-c $1$suffix";
	}
      else
	{
	  $flags = "-l $`";
	}
      push (@derived_tests, [$test_name, $flags, $in, $exp, $ret]);
    }

  return (@tv, @derived_tests);
}

1;
