#!/bin/sh
# Test 'date --debug' option.

# Copyright (C) 2016 Free Software Foundation, Inc.

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

. "${srcdir=.}/tests/init.sh"; path_prepend_ ./src
print_ver_ date

export LC_ALL=C

## Ensure timezones are supported.
## (NOTE: America/Belize timezone does not change on DST)
test "$(TZ=America/Belize date +%z)" = '-0600' \
    || skip_ 'Timezones database not found'


##
## Test 1: complex date string
##
in1='TZ="Asia/Tokyo" Sun, 90-12-11 + 3 days - 90 minutes'

cat<<EOF>exp1
date: parsed day part: Sun (day ordinal=0 number=0)
date: parsed date part: (Y-M-D) 0090-12-11
date: parsed relative part: +3 day(s)
date: parsed relative part: +3 day(s) -90 minutes
date: input timezone: +09:00 (set from TZ="Asia/Tokyo" in date string)
date: warning: adjusting year value 90 to 1990
date: warning: using midnight as starting time: 00:00:00
date: warning: day (Sun) ignored when explicit dates are given
date: starting date/time: '(Y-M-D) 1990-12-11 00:00:00 TZ=+09:00'
date: warning: when adding relative days, it is recommended to specify 12:00pm
date: after date adjustment (+0 years, +0 months, +3 days),
date:     new date/time = '(Y-M-D) 1990-12-14 00:00:00 TZ=+09:00'
date: '(Y-M-D) 1990-12-14 00:00:00 TZ=+09:00' = 661100400 epoch-seconds
date: after time adjustment (+0 hours, -90 minutes, +0 seconds, +0 ns),
date:     new time = 661095000 epoch-seconds
date: output timezone: -06:00 (set from TZ="America/Belize" environment value)
date: final: 661095000.000000000 (epoch-seconds)
date: final: (Y-M-D) 1990-12-13 13:30:00 (UTC0)
date: final: (Y-M-D) 1990-12-13 07:30:00 (output timezone TZ=-06:00)
Thu Dec 13 07:30:00 CST 1990
EOF

TZ=America/Belize date --debug -d "$in1" >out1 2>&1 || fail=1

compare exp1 out1 || fail=1

##
## Test 2: Invalid date from Coreutils' FAQ
##         (with explicit timezone added)
in2='TZ="America/Edmonton" 2006-04-02 02:30:00'
cat<<EOF>exp2
date: parsed date part: (Y-M-D) 2006-04-02
date: parsed time part: 02:30:00
date: input timezone: -07:00 (set from TZ="America/Edmonton" in date string)
date: using specified time as starting value: '02:30:00'
date: error: invalid date/time value:
date:     user provided time: '(Y-M-D) 2006-04-02 02:30:00 TZ=-07:00'
date:        normalized time: '(Y-M-D) 2006-04-02 03:30:00 TZ=-07:00'
date:                                             --
date:      possible reasons:
date:        non-existing due to daylight-saving time;
date:        numeric values overflow;
date:        missing timezone
date: invalid date 'TZ="America/Edmonton" 2006-04-02 02:30:00'
EOF

# date should return 1 (error) for invalid date
returns_ 1 date --debug -d "$in2" >out2 2>&1 || fail=1
compare exp2 out2 || fail=1

##
## Test 3: timespec (input always UTC, output is TZ-dependent)
##
in3='@1'
cat<<EOF>exp3
date: parsed number of seconds part: number of seconds: 1
date: input timezone: +00:00 (set from '@timespec' - always UTC0)
date: output timezone: -05:00 (set from TZ="America/Lima" environment value)
date: final: 1.000000000 (epoch-seconds)
date: final: (Y-M-D) 1970-01-01 00:00:01 (UTC0)
date: final: (Y-M-D) 1969-12-31 19:00:01 (output timezone TZ=-05:00)
Wed Dec 31 19:00:01 PET 1969
EOF

TZ=America/Lima date --debug -d "$in3" >out3 2>&1 || fail=1
compare exp3 out3 || fail=1

Exit $fail
