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
# With no number, this is like -10l
['obs-l', '-l', "x\n" . ("y\n" x 10) . 'z', ("y\n" x 9) . 'z', 0],

# This should get `tail: +cl: No such file or directory'
['err-1', '+cl', '', '', 1],

# This should get `tail: l: invalid number of bytes'
['err-2', '-cl', '', '', 1],

# Since the number is larger than 2^64, this should provoke
# the diagnostic: `tail: 99999999999999999999: number of bytes is so large \
# that it is not representable' on all systems... for now, probably, maybe.
['err-3', '-c99999999999999999999', '', '', 1],
['err-4', '-c', '', '', 1],

# Same as -l 10
['stdin-1', '-', '', '', 0],
['stdin-2', '-', "x\n" . ("y\n" x 10) . 'z', ("y\n" x 9) . 'z', 0],
);

sub test_vector
{
  return @tv;
}

1;
