package Test;
require 5.002;
use strict;

$Test::input_via_stdin = 1;

my @tv = (
# test name, options, input, expected output, expected return code
#
['obs-c1', '+2c', 'abcd', 'bcd', 0],
['obs-c2', '+8c', 'abcd', '', 0],
['obs-c3', '-1c', 'abcd', 'd', 0],
['obs-c4', '-9c', 'abcd', 'abcd', 0],
['obs-c5', '-12c', 'x' . ('y' x 12) . 'z', ('y' x 11) . 'z', 0],

['obs-l1', '-1l', 'x', 'x', 0],
['obs-l2', '-1l', "x\ny\n", "y\n", 0],
['obs-l3', '-1l', "x\ny", "y", 0],
['obs-l4', '+1l', "x\ny\n", "x\ny\n", 0],
['obs-l5', '+2l', "x\ny\n", "y\n", 0],

# Same as -l tests, but without the `l'.
['obs-1', '-1', 'x', 'x', 0],
['obs-2', '-1', "x\ny\n", "y\n", 0],
['obs-3', '-1', "x\ny", "y", 0],
['obs-4', '+1', "x\ny\n", "x\ny\n", 0],
['obs-5', '+2', "x\ny\n", "y\n", 0],

# This is equivalent to +10c
['obsx-1', '+c', 'x' . ('y' x 10) . 'z', 'yyz', 0],
# This is equivalent to +10l
['obsx-2', '+l', "x\n" . ("y\n" x 10) . 'z', "y\ny\nz", 0],

['err-1', '+2', "x\ny\n", "y\n", 0],
);

sub test_vector
{
  return @tv;
}

1;
