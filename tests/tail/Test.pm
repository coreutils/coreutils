package Test;
require 5.002;
use strict;

$Test::input_via_stdin = 1;

my @tv = (
# test name, options, input, expected output, expected return code
#
['1', '+2c', 'abcd', 'bcd', 0],
['2', '+8c', 'abcd', '', 0],
['3', '-1c', 'abcd', 'd', 0],
['4', '-9c', 'abcd', 'abcd', 0],
['5', '-12c', 'x' . ('y' x 12) . 'z', ('y' x 11) . 'z', 0],
);

sub test_vector
{
  return @tv;
}

1;
