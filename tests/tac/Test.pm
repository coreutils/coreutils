package Test;
require 5.002;
use strict;

my @tv = (
# test name, options, input, expected output, expected return code
#
['basic-0', '', "", "", 0],
['basic-a', '', "a", "a\n", 0],
['basic-b', '', "\n", "\n", 0],
['basic-c', '', "a\n", "a\n", 0],
['basic-d', '', "a\nb", "b\na\n", 0],
['basic-e', '', "a\nb\n", "b\na\n", 0],
);

sub test_vector
{
  my $t;
  foreach $t (@tv)
    {
      my ($test_name, $flags, $in, $exp, $ret) = @$t;

      # If you run the minus* tests with a FILE arg they'd hang.
      if ($test_name =~ /^minus/)
	{
	  $Test::input_via{$test_name} = {REDIR => 0, PIPE => 0};
	}
      else
	{
	  $Test::input_via{$test_name} = {REDIR => 0, FILE => 0, PIPE => 0}
	}
    }

  return @tv;
}

1;
