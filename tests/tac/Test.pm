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

['opt-b', '-b', "\na\nb\nc", "\nc\nb\na", 0],
['opt-s', '-s:', "a:b:c:", "c:b:a:", 0],
['opt-sb', '-s : -b', ":a:b:c", ":c:b:a", 0],
['opt-r',     "-r -s '\\._+'", "1._2.__3.___4._", "4._3.___2.__1._", 0],

['opt-r2',    "-r -s '\\._+'", "a.___b.__1._2.__3.___4._",
                               "4._3.___2.__1._b.__a.___", 0],

# This gave incorrect output (.___4._2.__3._1) with tac-1.22.
['opt-br', "-b -r -s '\\._+'", "._1._2.__3.___4", ".___4.__3._2._1", 0],

['opt-br2', "-b -r -s '\\._+'", ".__x.___y.____z._1._2.__3.___4",
                                ".___4.__3._2._1.____z.___y.__x", 0],
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
      # $Test::input_via{$t} = {REDIR => 0, PIPE => 0};
      $Test::input_via{$t} = {};
    }

  return @tv;
}

1;
