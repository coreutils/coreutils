# -*-perl-*-
package Test;
require 5.002;
use strict;

$Test::input_via_stdin = 1;

sub test_vector
{
  my @tvec =
    (
     # test-name options input expected-output expected-return-code
     #
     ['1', '', '',		'd41d8cd98f00b204e9800998ecf8427e', 0],
     ['2', '', 'a',		'0cc175b9c0f1b6a831c399e269772661', 0],
     ['3', '', 'abc',		'900150983cd24fb0d6963f7d28e17f72', 0],
     ['4', '', 'message digest', 'f96b697d7cb7938d525a2f31aaf161d0', 0],
     ['5', '', 'abcdefghijklmnopqrstuvwxyz',
      'c3fcd3d76192e4007dfb496cca67e13b', 0],
     ['6', '',
      'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789',
      'd174ab98d277d9f5a5611c2c9f419d9f', 0],
     ['7', '', '1234567890123456789012345678901234567890'
      . '1234567890123456789012345678901234567890',
      '57edf4a22be3c955ac49da2e2107b67a', 0],
     );
  my @tv;

  # Append two spaces, the input file name (-), and a newline to each
  # expected output string.
  my $t;
  foreach $t (@tvec)
    {
      my ($test_name, $flags, $in, $exp, $ret) = @$t;
      push (@tv, [$test_name, $flags, $in, "$exp  -\n", $ret]);
    }

  return @tv;
}

1;
