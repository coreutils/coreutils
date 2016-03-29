#!/usr/bin/perl
# Exercise expand.

# Copyright (C) 2004-2016 Free Software Foundation, Inc.

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

use strict;

(my $program_name = $0) =~ s|.*/||;

# Turn off localization of executable's output.
@ENV{qw(LANGUAGE LANG LC_ALL)} = ('C') x 3;

my @Tests =
  (
   ['t1', '--tabs=3',     {IN=>"a\tb"}, {OUT=>"a  b"}],
   ['t2', '--tabs=3,6,9', {IN=>"a\tb\tc\td\te"}, {OUT=>"a  b  c  d e"}],
   ['i1', '--tabs=3 -i', {IN=>"\ta\tb"}, {OUT=>"   a\tb"}],
   ['i2', '--tabs=3 -i', {IN=>" \ta\tb"}, {OUT=>"   a\tb"}],
  );

my $save_temps = $ENV{DEBUG};
my $verbose = $ENV{VERBOSE};

my $prog = 'expand';
my $fail = run_tests ($program_name, $prog, \@Tests, $save_temps, $verbose);
exit $fail;
