# -*-perl-*-
package Test;
require 5.002;
use strict;

# For each test...
# Export LANG=C so that the locale-dependent strings match.
# Export TZ=UTC so that zone-dependent strings match.
$Test::env_default = ['LANG=C TZ=UTC'];

sub test_vector
{
  my @tvec =
    (
     # test-name options input expected-output expected-return-code
     #
     ['1a', '', {}, '', 1],
     ['1b', "-z ''", {}, '', 0],
     ['1c', 'any-string', {}, '', 0],
     ['1d', '-n any-string', {}, '', 0],

     ['eq-1', 't = t', {}, '', 0],
     ['eq-2', 't = f', {}, '', 1],
     ['ne-1', 't != t', {}, '', 1],
     ['ne-2', 't != f', {}, '', 0],

     ['and-1', 't -a t', {}, '', 0],
     ['and-2', '"" -a t', {}, '', 1],
     ['and-3', 't -a ""', {}, '', 1],
     ['and-4', '"" -a ""', {}, '', 1],

     ['or-1', 't -o t', {}, '', 0],
     ['or-2', '"" -o t', {}, '', 0],
     ['or-3', 't -o ""', {}, '', 0],
     ['or-4', '"" -o ""', {}, '', 1],

     ['ieq-1', '9 -eq 9', {}, '', 0],
     ['ieq-2', '0 -eq 0', {}, '', 0],
     ['ieq-3', '0 -eq 00', {}, '', 0],
     ['ieq-4', '8 -eq 9', {}, '', 1],
     ['ieq-5', '1 -eq 0', {}, '', 1],
     ['ieq-6', '0x0 -eq 00', {}, '', 1],

     ['ge-1', '5 -ge 5', {}, '', 0],
     ['ge-2', '5 -ge 4', {}, '', 0],
     ['ge-3', '4 -ge 5', {}, '', 1],
     ['ge-4', '-1 -ge -2', {}, '', 0],

     ['gt-1', '5 -gt 5', {}, '', 1],
     ['gt-2', '5 -gt 4', {}, '', 0],
     ['gt-3', '4 -gt 5', {}, '', 1],
     ['gt-4', '-1 -gt -2', {}, '', 0],

     ['le-1', '5 -le 5', {}, '', 0],
     ['le-2', '5 -le 4', {}, '', 1],
     ['le-3', '4 -le 5', {}, '', 0],
     ['le-4', '-1 -le -2', {}, '', 1],

     ['lt-1', '5 -lt 5', {}, '', 1],
     ['lt-2', '5 -lt 4', {}, '', 1],
     ['lt-3', '4 -lt 5', {}, '', 0],
     ['lt-4', '-1 -lt -2', {}, '', 1],

     ['t1', "-t", {}, '', 1],
     ['t2', "-t 1", {}, '', 1],
     );

  return @tvec;
}

1;
