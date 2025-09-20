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

my $prog = 'fold';

# Turn off localization of executable's output.
@ENV{qw(LANGUAGE LANG LC_ALL)} = ('C') x 3;

# uncommented to enable multibyte paths
my $mb_locale = $ENV{LOCALE_FR_UTF8};
! defined $mb_locale || $mb_locale eq 'none'
 and $mb_locale = 'C';

my @Tests =
  (
   ['s1', '-w2 -s', {IN=>"a\t"}, {OUT=>"a\n\t"}],
   ['s2', '-w4 -s', {IN=>"abcdef d\n"}, {OUT=>"abcd\nef d\n"}],
   ['s3', '-w4 -s', {IN=>"a cd fgh\n"}, {OUT=>"a \ncd \nfgh\n"}],
   ['s4', '-w4 -s', {IN=>"abc ef\n"}, {OUT=>"abc \nef\n"}],

   # The downstream I18N patch made fold(1) exit with success for non-existing
   # files since v5.2.1-1158-g3d3030da6 (2004) changed int to bool for booleans.
   # The I18N patch was fixed only in July 2024.  (rhbz#2296201).
   ['enoent', 'enoent', {EXIT => 1}, {ERR_SUBST=>"s/.*//msg"}],

   # The downstream I18N patch made 'fold -b' mishandle '\n' in UTF locales.
   # The I18N patch was fixed only in Sep 2024.  (RHEL-60295)
   ['bw1', '-b -w 4', {IN=>"abcdef\nghijkl"}, {OUT=>"abcd\nef\nghij\nkl"}],
   ['bw2', '-b -w 6', {IN=>"1234567890\nabcdefghij\n1234567890"},
     {OUT=>"123456\n7890\nabcdef\nghij\n123456\n7890"}],
  );

# define UTF-8 encoded multi-byte characters for tests
my $eaC = "\xC3\xA9";      # e acute NFC form
my $eaD = "\x65\xCC\x81";  # e acute NFD form (zero width combining)
my $eFW = "\xEF\xBD\x85";  # e fullwidth

my @mbTests =
  (
     ['smb1', '-w2', {IN=>$eaC x 3}, {OUT=>$eaC x 2 . "\n" . $eaC}],
     ['smb2', '-w2', {IN=>$eaD x 3}, {OUT=>$eaD x 2 . "\n" . $eaD}],
     ['smb3', '-w2', {IN=>$eFW x 2}, {OUT=>$eFW . "\n" . $eFW}],
  );

if ($mb_locale ne 'C')
  {
    # Duplicate each test vector, appending "-mb" to the test name and
    # inserting {ENV => "LC_ALL=$mb_locale"} in the copy, so that we
    # provide coverage for multi-byte code paths.
    my @new;

    foreach my $t (@Tests)
      {
        my @new_t = @$t;
        my $test_name = shift @new_t;

        push @new, ["$test_name-mb", @new_t, {ENV => "LC_ALL=$mb_locale"}];
      }
    push @Tests, @new;

    @new = ();
    foreach my $t (@mbTests)
      {
        my @new_t = @$t;

        push @new, [@new_t, {ENV => "LC_ALL=$mb_locale"}];
      }
    push @Tests, @new;
  }

@Tests = triple_test \@Tests;

# Remember that triple_test creates from each test with exactly one "IN"
# file two more tests (.p and .r suffix on name) corresponding to reading
# input from a file and from a pipe.  The pipe-reading test would fail
# due to a race condition about 1 in 20 times.
# Remove the IN_PIPE version of the "output-is-input" test above.
# The others aren't susceptible because they have three inputs each.
@Tests = grep {$_->[0] ne 'output-is-input.p'} @Tests;

my $save_temps = $ENV{DEBUG};
my $verbose = $ENV{VERBOSE};

my $fail = run_tests ($program_name, $prog, \@Tests, $save_temps, $verbose);
exit $fail;
