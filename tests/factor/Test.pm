# -*-perl-*-
package Test;
require 5.002;
use strict;

sub test_vector
{
  my @tvec =
    (
     # test-name options input expected-output expected-return-code
     #
     ['1', "9", {}, '9: 3 3', 0],
     ['2', "4294967291", {}, '4294967291: 4294967291', 0],
     ['3', "4294967292", {}, '4294967292: 2 2 3 3 7 11 31 151 331', 0],
     ['4', "4294967293", {}, '4294967293: 9241 464773', 0],

     # FIXME: add a lot more...
     );

  my @tv;
  my $t;
  foreach $t (@tvec)
    {
      my ($test_name, $flags, $in, $exp, $ret) = @$t;
      # Append a newline to end of each expected string.
      push (@tv, [$test_name, $flags, $in, "$exp\n", $ret]);
    }

  return @tv;
}

1;
