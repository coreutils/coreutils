#!/usr/bin/perl
# Exercise fold.

# Copyright (C) 2003-2025 Free Software Foundation, Inc.

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

use strict;

(my $program_name = $0) =~ s|.*/||;

# Turn off localization of executable's output.
@ENV{qw(LANGUAGE LANG LC_ALL)} = ('C') x 3;
my $prog = 'fold';

my @Tests =
  (
   ['s1', '-w2 -s', {IN=>"a\t"}, {OUT=>"a\n\t"}],
   ['s2', '-w4 -s', {IN=>"abcdef d\n"}, {OUT=>"abcd\nef d\n"}],
   ['s3', '-w4 -s', {IN=>"a cd fgh\n"}, {OUT=>"a \ncd \nfgh\n"}],
   ['s4', '-w4 -s', {IN=>"abc ef\n"}, {OUT=>"abc \nef\n"}],

   # The downstream I18N patch made fold(1) exit with success for non-existing
   # files since v5.2.1-1158-g3d3030da6 (2004) changed int to bool for booleans.
   # The I18N patch was fixed only in July 2024.  (rhbz#2296201).
   ['enoent', 'enoent', {EXIT => 1},
     {ERR=>"$prog: enoent: No such file or directory\n"}],

   # The downstream I18N patch made 'fold -b' mishandled '\n' in UTF locales.
   # The I18N patch was fixed only in Sep 2024.  (RHEL-60295)
   ['bw1', '-b -w 4', {IN=>"abcdef\nghijkl"}, {OUT=>"abcd\nef\nghij\nkl"}],
   ['bw2', '-b -w 6', {IN=>"1234567890\nabcdefghij\n1234567890"},
     {OUT=>"123456\n7890\nabcdef\nghij\n123456\n7890"}],
  );

my $save_temps = $ENV{DEBUG};
my $verbose = $ENV{VERBOSE};

my $fail = run_tests ($program_name, $prog, \@Tests, $save_temps, $verbose);
exit $fail;
