package Test;
require 5.002;
use strict;

my @tv = (
# test name, options, input, expected output, expected return code
#
['idem-1', '', "", "", 0],
['idem-2', '', "a", "a\n", 0],
['idem-3', '', "\n", "\n", 0],
['idem-4', '', "a\n", "a\n", 0],

);

sub test_vector
{
  my $t;
  foreach $t (@tv)
    {
      my ($test_name, $flags, $in, $exp, $ret) = @$t;
      $Test::input_via{$test_name} = {REDIR => 0, FILE => 0, PIPE => 0}
    }

  return @tv;
}

1;
