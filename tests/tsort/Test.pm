# -*-perl-*-
package Test;
require 5.002;
use strict;

my @tv = (
#test   options   input   expected-output   expected-return-code
#
["cycle-1", '', "t x\nt s\ns t\n", "FIXME", 0],

);

sub test_vector
{
  return @tv;
}

1;
