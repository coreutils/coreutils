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

     # FIXME: add new date-related tests in ../misc/date.
     );

  my @tv;
  my $t;
  foreach $t (@tvec)
    {
      my ($test_name, $flags, $in, $exp, $ret) = @$t;

      print "['$test_name', "{OUT=$exp}" ],\n";

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
