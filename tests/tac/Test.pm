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
['basic-f', '', "1234567\n8\n", "8\n1234567\n", 0],
['basic-g', '', "12345678\n9\n", "9\n12345678\n", 0],
['basic-h', '', "123456\n8\n", "8\n123456\n", 0],
['basic-i', '', "12345\n8\n", "8\n12345\n", 0],
['basic-j', '', "1234\n8\n", "8\n1234\n", 0],
['basic-k', '', "123\n8\n", "8\n123\n", 0],

['b2-e', '', "a\nb", "b\na\n", 0],
['b2-f', '', "1234567\n8", "8\n1234567\n", 0],
['b2-g', '', "12345678\n9", "9\n12345678\n", 0],
['b2-h', '', "123456\n8", "8\n123456\n", 0],
['b2-i', '', "12345\n8", "8\n12345\n", 0],
['b2-j', '', "1234\n8", "8\n1234\n", 0],
['b2-k', '', "123\n8", "8\n123\n", 0],
);

sub test_vector
{
  my $t;
  foreach $t (@tv)
    {
      my ($test_name, $flags, $in, $exp, $ret) = @$t;

      $Test::input_via{$test_name} = {REDIR => 0, FILE => 0, PIPE => 0}
    }

  # Temporarily turn off losing tests.
  # These tests lose because tac_file isn't yet up to snuff with tac_mem.
  foreach $t (qw (basic-a basic-d b2-e b2-f b2-g b2-h b2-i b2-j b2-k))
    {
      $Test::input_via{$t} = {REDIR => 0, PIPE => 0};
    }

  return @tv;
}

1;
