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

     ['leap-1', "--date '02/29/1996 1 year' +%Y-%m-%d", {}, '1997-03-01', 0],

     ['U95-1', "--date '1995-1-1' +%U", {}, '01', 0],
     ['U95-2', "--date '1995-1-7' +%U", {}, '01', 0],
     ['U95-3', "--date '1995-1-8' +%U", {}, '02', 0],

     ['U92-1', "--date '1992-1-1' +%U", {}, '00', 0],
     ['U92-2', "--date '1992-1-4' +%U", {}, '00', 0],
     ['U92-3', "--date '1992-1-5' +%U", {}, '01', 0],

     ['V92-1', "--date '1992-1-1' +%V", {}, '01', 0],
     ['V92-2', "--date '1992-1-5' +%V", {}, '01', 0],
     ['V92-3', "--date '1992-1-6' +%V", {}, '02', 0],

     ['W92-1', "--date '1992-1-1' +%W", {}, '00', 0],
     ['W92-2', "--date '1992-1-5' +%W", {}, '00', 0],
     ['W92-3', "--date '1992-1-6' +%W", {}, '01', 0],

     ['millen-1', "--date '1998-1-1 3 years' +%Y", {}, '2001', 0],

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
  # Verify that the test-script generation code properly handles
  # per-test overrides.
  $Test::env{9} = ['LANG=C TZ=GMT'];

  return @tv;
}

1;
