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

     ['and-1', 't -a t', {}, '', 0],
     ['and-2', '"" -a t', {}, '', 1],
     ['and-3', 't -a ""', {}, '', 1],
     ['and-4', '"" -a ""', {}, '', 1],

     ['or-1', 't -o t', {}, '', 0],
     ['or-2', '"" -o t', {}, '', 0],
     ['or-3', 't -o ""', {}, '', 0],
     ['or-4', '"" -o ""', {}, '', 1],

     ['t1', "-t", {}, '', 1],
     ['t2', "-t 1", {}, '', 1],
     );

  return @tvec;
}

1;
