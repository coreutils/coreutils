package Test;
require 5.002;
use strict;

$Test::input_via_default = {FILE => 0};

my @tv = (
# test name, options, input, expected output, expected return code
#
['q1', '-b', sub{('""""" "', 'b')}, '"""""\\ " b', 0],
['q2', '-b -Q', sub{('""""" "', 'b')}, '\\"\\"\\"\\"\\"\\ \\" b', 0],
);

sub test_vector
{
  my $t;
  foreach $t (@tv)
    {
      # FIXME: append newline
      my ($test_name, $flags, $in, $exp, $ret) = @$t;

    }

  return @tv;
}

1;
