# -*-perl-*-
package Test;
require 5.002;
use strict;

sub test_vector
{

  my $d1 = '1997-01-19 08:17:48 +0';
  my @tvec =
    (
     # test-name options input expected-output expected-return-code
     #
     ['1', "-d '$d1' +'%% %a %A %b %B'", {}, '% Sun Sunday Jan January', 0],
     ['2', "-d '$d1' +'%c'", {}, 'Sun Jan 19 08:17:48 1997', 0],
     ['3', "-d '$d1' +'%d_%D_%e_%h_%H'", {}, '19_01/19/97_19_Jan_08', 0],
     ['4', "-d '$d1' +'%I_%j_%k_%l_%m'", {}, '08_019_ 8_ 8_01', 0],
     ['5', "-d '$d1' +'%M_%n_%p_%r'", {}, "17_\n_AM_08:17:48 AM", 0],
     ['6', "-d '$d1' +'%s_%S_%t_%T'", {}, "853661868_48_\t_08:17:48", 0],
     ['7', "-d '$d1' +'%U_%V_%w_%W'", {}, '03_03_0_02', 0],
     ['8', "-d '$d1' +'%x_%X_%y_%Y'", {}, '01/19/97_08:17:48_97_1997', 0],
     ['9', "-d '$d1' +'%z_%Z'", {}, '+0000_GMT', 0],
     );

  # For each test...
  # Export LANG=C so that the locale-dependent strings match.
  # Export TZ=GMT so that zone-dependent strings match.
  my @tv;
  my $t;
  foreach $t (@tvec)
    {
      my ($test_name, $flags, $in, $exp, $ret) = @$t;
      $Test::env{$test_name} = 'LANG=C TZ=UTC';
      # Append a newline to end of each expected string.
      push (@tv, [$test_name, $flags, $in, "$exp\n", $ret]);
    }

  return @tv;
}

1;
