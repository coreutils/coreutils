# -*-perl-*-
package Test;
require 5.002;
use strict;

use Config;

# For each test...
# Export LANG=C so that the locale-dependent strings match.
# Export TZ=UTC0 so that zone-dependent strings match.
$Test::env_default = ['LANG=C TZ=UTC0'];

sub test_vector
{

  my $t0 = '08:17:48';
  my $d0 = '1997-01-19';
  my $d1 = "$d0 $t0 +0";

  my $ts = '08:17:49'; # next second
  my $tm = '08:18:48'; # next minute
  my $th = '09:17:48'; # next hour

  my $dd = '1997-01-20'; # next day
  my $dw = '1997-01-26'; # next week
  my $dm = '1997-02-19'; # next month
  my $dy = '1998-01-19'; # next month

  my $fmt = "'+%Y-%m-%d %T'";

  my @tvec =
    (
     # test-name options input expected-output expected-return-code
     #
     ['1', "-d '$d1' +'%% %a %A %b %B'", {}, '% Sun Sunday Jan January', 0],

     # [Actually, skip it on *all* systems. -- this Perl code is run at
     # distribution-build-time, not at configure/test time.  ]

     # Skip the test of %c on SunOS4 systems.  Such systems would fail this
     # test because their underlying strftime doesn't handle the %c format
     # properly.  GNU strftime must rely on the underlying host library
     # function to get locale-dependent behavior, as strftime is the only
     # portable interface to that behavior.
     # ['2', "-d '$d1' +'%c'", {}, "Sun Jan 19 $t0 1997", 0],

     ['3', "-d '$d1' +'%d_%D_%e_%h_%H'", {}, '19_01/19/97_19_Jan_08', 0],
     ['4', "-d '$d1' +'%I_%j_%k_%l_%m'", {}, '08_019_ 8_ 8_01', 0],
     ['5', "-d '$d1' +'%M_%n_%p_%r'", {}, "17_\n_AM_$t0 AM", 0],
     ['6', "-d '$d1' +'%s_%S_%t_%T'", {}, "853661868_48_\t_$t0", 0],
     ['7', "-d '$d1' +'%U_%V_%w_%W'", {}, '03_03_0_02', 0],
     ['8', "-d '$d1' +'%x_%X_%y_%Y'", {}, "01/19/97_${t0}_97_1997", 0],
     ['9', "-d '$d1' +'%z'", {}, '+0000', 0],

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

     ['rel-0', "-d '$d1 now' '+%Y-%m-%d %T'", {}, "$d0 $t0", 0],

     ['rel-1a', "-d '$d1 yesterday' $fmt", {}, "1997-01-18 $t0", 0],
     ['rel-1b', "-d '$d1 tomorrow' $fmt", {}, "1997-01-20 $t0", 0],

     ['rel-2a', "-d '$d1 6 years ago' $fmt", {}, "1991-01-19 $t0", 0],
     ['rel-2b', "-d '$d1 7 months ago' $fmt", {}, "1996-06-19 $t0", 0],
     ['rel-2c', "-d '$d1 8 weeks ago' $fmt", {}, "1996-11-24 $t0", 0],
     ['rel-2d', "-d '$d1 1 day ago' $fmt", {}, "1997-01-18 $t0", 0],
     ['rel-2e', "-d '$d1 2 hours ago' $fmt", {}, "$d0 06:17:48", 0],
     ['rel-2f', "-d '$d1 3 minutes ago' $fmt", {}, "$d0 08:14:48", 0],
     ['rel-2g', "-d '$d1 4 seconds ago' $fmt", {}, "$d0 08:17:44", 0],

     ['rel-3a', "-d '$d1 4 seconds ago' $fmt", {}, "$d0 08:17:44", 0],

     ['next-s', "-d '$d1 next second' '+%Y-%m-%d %T'", {}, "$d0 $ts", 0],
     ['next-m', "-d '$d1 next minute' '+%Y-%m-%d %T'", {}, "$d0 $tm", 0],
     ['next-h', "-d '$d1 next hour'   '+%Y-%m-%d %T'", {}, "$d0 $th", 0],
     ['next-d', "-d '$d1 next day'    '+%Y-%m-%d %T'", {}, "$dd $t0", 0],
     ['next-w', "-d '$d1 next week'   '+%Y-%m-%d %T'", {}, "$dw $t0", 0],
     ['next-mo', "-d '$d1 next month' '+%Y-%m-%d %T'", {}, "$dm $t0", 0],
     ['next-y', "-d '$d1 next year'   '+%Y-%m-%d %T'", {}, "$dy $t0", 0],

     ['utc-0', "-u -d '08/01/97 6:00' '+%D,%H:%M'", {}, "08/01/97,06:00", 0],
     ['utc-0a', "-u -d '08/01/97 6:00 UTC +4 hours' '+%D,%H:%M'", {},
      "08/01/97,10:00", 0],
     # Make sure --file=FILE works with -u.
     ['utc-1', "-u --file=- '+%Y-%m-%d %T'", "$d0 $t0\n$d0 $t0\n",
      "$d0 $t0\n$d0 $t0", 0],
     ['utc-1a', "-u --file=- '+%Y-%m-%d %T'",
      "$d0 $t0 UTC +1 hour\n$d0 $t0 UTC +1 hour\n",
      "$d0 $th\n$d0 $th", 0],

     # From the examples in the documentation.
     ['date2sec-0', "-d '1970-01-01 00:00:01' +%s", {}, "7201", 0],
     # Same as above, but don't rely on TZ in environment.
     ['date2sec-0a', "-d '1970-01-01 00:00:01 UTC +2 hours' +%s", {}, "7201",0],

     ['date2sec-1', "-d 2000-01-01 +%s", {}, "946684800", 0],
     ['sec2date-0', "-d '1970-01-01 UTC 946684800 sec' +'%Y-%m-%d %T %z'", {},
      "2000-01-01 00:00:00 +0000", 0],

     ['this-m', "-d '$d0 $t0 this minute' $fmt", {}, "$d0 $t0", 0],
     ['this-h', "-d '$d0 $t0 this hour' $fmt", {}, "$d0 $t0", 0],
     ['this-w', "-d '$d0 $t0 this week' $fmt", {}, "$d0 $t0", 0],
     ['this-mo', "-d '$d0 $t0 this month' $fmt", {}, "$d0 $t0", 0],
     ['this-y', "-d '$d0 $t0 this year' $fmt", {}, "$d0 $t0", 0],

     ['risks-1', "-d 'Nov 10 1996' $fmt", {}, "1996-11-10 00:00:00", 0],

     ['regress-1', "-u -d '1996-11-10 0:00:00 +0' $fmt", {},
      "1996-11-10 00:00:00", 0],

     ['datevtime-1', "-d 000909 $fmt", {}, "2000-09-09 00:00:00", 0],

     # test for RFC-822 conformance
     ['rfc822-1', "-R -d '$d1'", {}, "Sun, 19 Jan 1997 08:17:48 +0000", 0],

     # Relative seconds, with time.  fixed in 2.0j
     ['relative-1', "--utc -d '1970-01-01 00:00:00 UTC +961062237 sec' $fmt",
      {}, "2000-06-15 09:43:57", 0],

     # Relative seconds, no time.
     ['relative-2', "--utc -d '1970-01-01 UTC +961062237 sec' $fmt", {},
      "2000-06-15 09:43:57", 0],

     # This would infloop (or appear to) prior to coreutils-4.5.5,
     # due to a bug in strftime.c.
     ['wide-fmt', "-d '1999-06-01' +%3004Y", {}, '0' x 3000 . '1999', 0],

     # FIXME: add a lot more...
     );

  my $sunos4 = "$Config::Config{osname}$Config::Config{osvers}" =~ /sunos4/;

  my @tv;
  my $t;
  foreach $t (@tvec)
    {
      my ($test_name, $flags, $in, $exp, $ret) = @$t;

      # Append a newline to end of each expected string.
      push (@tv, [$test_name, $flags, $in, "$exp\n", $ret]);
    }

  $Test::env{'utc-0'} = ['TZ=UTC+4'];

  # This one would pass if TZ (with any, or even no, value) were in
  # the environment.
  $Test::env{'regress-1'} = ['LANG=C'];

  $Test::env{'utc-1'} = ['TZ=UTC+1'];
  $Test::input_via{'utc-1'} = {REDIR => 0};
  $Test::input_via{'utc-1a'} = {REDIR => 0};

  $Test::env{'date2sec-0'} = ['TZ=UTC+2'];

  $Test::env{'rfc822-1'} = ['LC_ALL=de_DE TZ=UTC0'];
  $Test::env{'relative-2'} = ['TZ=UTC+1'];

  return @tv;
}

1;
