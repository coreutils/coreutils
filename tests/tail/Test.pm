package Test;
require 5.002;
use strict;

my @tv = (
# test name, options, input, expected output, expected return code
#
['obs-plus-c1', '+2c', 'abcd', 'bcd', 0],
['obs-plus-c2', '+8c', 'abcd', '', 0],
['obs-c3', '-1c', 'abcd', 'd', 0],
['obs-c4', '-9c', 'abcd', 'abcd', 0],
['obs-c5', '-12c', 'x' . ('y' x 12) . 'z', ('y' x 11) . 'z', 0],


['obs-l1', '-1l', 'x', 'x', 0],
['obs-l2', '-1l', "x\ny\n", "y\n", 0],
['obs-l3', '-1l', "x\ny", "y", 0],
['obs-plus-l4', '+1l', "x\ny\n", "x\ny\n", 0],
['obs-plus-l5', '+2l', "x\ny\n", "y\n", 0],

# Same as -l tests, but without the `l'.
['obs-1', '-1', 'x', 'x', 0],
['obs-2', '-1', "x\ny\n", "y\n", 0],
['obs-3', '-1', "x\ny", "y", 0],
['obs-plus-4', '+1', "x\ny\n", "x\ny\n", 0],
['obs-plus-5', '+2', "x\ny\n", "y\n", 0],

# This is equivalent to +10c
['obs-plus-x1', '+c', 'x' . ('y' x 10) . 'z', 'yyz', 0],
# This is equivalent to +10l
['obs-plus-x2', '+l', "x\n" . ("y\n" x 10) . 'z', "y\ny\nz", 0],
# With no number, this is like -10l
['obs-l', '-l', "x\n" . ("y\n" x 10) . 'z', ("y\n" x 9) . 'z', 0],

['obs-b', '-b', "x\n" x (512 * 10 / 2 + 1), "x\n" x (512 * 10 / 2), 0],

# This should get
# `tail: cannot open `+cl' for reading: No such file or directory'
['err-1', '+cl', '', '', 1],

# This should get `tail: l: invalid number of bytes'
['err-2', '-cl', '', '', 1],

# This should get
# `tail: cannot open `+2cz' for reading: No such file or directory'
['err-3', '+2cz', '', '', 1],

# This should get `tail: invalid option -- 2'
['err-4', '-2cX', '', '', 1],

# Since the number is larger than 2^64, this should provoke
# the diagnostic: `tail: 99999999999999999999: invalid number of bytes'
# on all systems... probably, for now, maybe.
['err-5', '-c99999999999999999999', '', '', 1],
['err-6', '-c', '', '', 1],

# Same as -n 10
['minus-1', '-', '', '', 0],
['minus-2', '-', "x\n" . ("y\n" x 10) . 'z', ("y\n" x 9) . 'z', 0],

['n-1', '-n 10', "x\n" . ("y\n" x 10) . 'z', ("y\n" x 9) . 'z', 0],
['n-2', '-n -10', "x\n" . ("y\n" x 10) . 'z', ("y\n" x 9) . 'z', 0],
['n-3', '-n +10', "x\n" . ("y\n" x 10) . 'z', "y\ny\nz", 0],

# Accept +0 as synonym for +1.
['n-4',  '-n +0', "y\n" x 5, "y\n" x 5, 0],
['n-4a', '-n +1', "y\n" x 5, "y\n" x 5, 0],

# Note that -0 is *not* a synonym for -1.
['n-5',  '-n -0', "y\n" x 5, '', 0],
['n-5a', '-n -1', "y\n" x 5, "y\n", 0],
['n-5b', '-n  0', "y\n" x 5, '', 0],

# With textutils-1.22, this failed.
['f-1', '-f -n 1', "a\nb\n", "b\n", 0],
#['f-2', '-f -n 1 -', "a\nb\n", "b\n", 0],
);

sub test_vector
{
  # With _POSIX2_VERSION=199209, `tail -c' succeeds, but err-6 expects
  # a failure, so set _POSIX2_VERSION to ensure it fails.
  $Test::env{'err-6'} = ['_POSIX2_VERSION=200112'];

  my $t;
  foreach $t (@tv)
    {
      my ($test_name, $flags, $in, $exp, $ret) = @$t;

      $test_name =~ /^(obs-plus-|minus-)/
	and $Test::env{$test_name} = ['_POSIX2_VERSION=199209'];

      # If you run the minus* tests with a FILE arg they'd hang.
      # If you run the err-1 or err-3 tests with a FILE, they'd misinterpret
      # the arg unless we are using the obsolete form.
      if ($test_name =~ /^(minus|err-[13])/)
	{
	  $Test::input_via{$test_name} = {REDIR => 0, PIPE => 0};
	}
      elsif ($test_name =~ /^f-/)
	{
	  # Using redirection or a file would make this hang.
	  $Test::input_via{$test_name} = {PIPE => 0};
	}
      else
	{
	  $Test::input_via{$test_name} = {REDIR => 0, FILE => 0, PIPE => 0}
	}
    }

  return @tv;
}

1;
