#!/usr/bin/perl
# Basic tests for "wc".

# Copyright (C) 1997-2026 Free Software Foundation, Inc.

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

my $prog = 'wc';

# Turn off localization of executable's output.
@ENV{qw(LANGUAGE LANG LC_ALL)} = ('C') x 3;

my $mb_locale = $ENV{LOCALE_FR_UTF8};
my $single_byte_locale = 'C';

{
  my $codeset = qx(LC_ALL=C locale charmap 2>/dev/null);
  chomp $codeset;
  if ($codeset eq 'UTF-8')
    {
      my $fr_locale = $ENV{LOCALE_FR};
      $single_byte_locale
        = defined $fr_locale && $fr_locale ne 'none' ? $fr_locale : undef;
    }
}

my @Tests =
    (
     ['a0', '-c',  {IN_PIPE=>''},            {OUT=>"0\n"}],
     ['a1', '-l',  {IN_PIPE=>''},            {OUT=>"0\n"}],
     ['a2', '-w',  {IN_PIPE=>''},            {OUT=>"0\n"}],
     ['a3', '-c',  {IN_PIPE=>'x'},           {OUT=>"1\n"}],
     ['a4', '-w',  {IN_PIPE=>'x'},           {OUT=>"1\n"}],
     ['a5', '-w',  {IN_PIPE=>"x y\n"},       {OUT=>"2\n"}],
     ['a6', '-w',  {IN_PIPE=>"x y\nz"},      {OUT=>"3\n"}],
     # Remember, -l counts *newline* bytes, not logical lines.
     ['a7', '-l',  {IN_PIPE=>"x y"},         {OUT=>"0\n"}],
     ['a8', '-l',  {IN_PIPE=>"x y\n"},       {OUT=>"1\n"}],
     ['a9', '-l',  {IN_PIPE=>"x\ny\n"},      {OUT=>"2\n"}],
     ['b0', '',    {IN_PIPE=>""},         {OUT=>"      0       0       0\n"}],
     ['b1', '',    {IN_PIPE=>"a b\nc\n"}, {OUT=>"      2       3       6\n"}],
     ['c0', '-L',  {IN_PIPE=>"1\n12\n"},     {OUT=>"2\n"}],
     ['c1', '-L',  {IN_PIPE=>"1\n123\n1\n"}, {OUT=>"3\n"}],
     ['c2', '-L',  {IN_PIPE=>"\n123456"},    {OUT=>"6\n"}],
     ['d1', '-w',  {IN_PIPE=>"\1\n"},        {OUT=>"1\n"}],
    );

# Test the behavior of -m with a multi-byte locale.
my @mbTests =
    (
     # LATIN SMALL LETTER A
     ['mb-m-1byte-1', '-m', {IN_PIPE=>"\x61"}, {OUT=>"1\n"},
      {ENV=>"LC_ALL=$mb_locale"}],
     # NO-BREAK SPACE
     ['mb-m-2byte-1', '-m', {IN_PIPE=>"\xC2\xA0"}, {OUT=>"1\n"},
      {ENV=>"LC_ALL=$mb_locale"}],
     # SAMARITAN LETTER ALAF
     ['mb-m-3byte-1', '-m', {IN_PIPE=>"\xE0\xA0\x80"}, {OUT=>"1\n"},
      {ENV=>"LC_ALL=$mb_locale"}],
     # LINEAR B SYLLABLE B008 A
     ['mb-m-4byte-1', '-m', {IN_PIPE=>"\xF0\x90\x80\x80"}, {OUT=>"1\n"},
      {ENV=>"LC_ALL=$mb_locale"}],
    );

defined $mb_locale && $mb_locale ne 'none' and push @Tests, @mbTests;

# Make sure -m counts bytes in single-byte locales.
if (defined $single_byte_locale)
  {
    my @new;
    my $in_len;
    for my $t (@mbTests)
      {
        my @new_t = map { ref $_ eq 'HASH' ? { %$_ } : $_ } @$t;
        (my $test_name = shift @new_t) =~ s/^mb/c/;
        for my $e (@new_t)
          {
            next unless ref $e eq 'HASH';
            $e->{ENV} = "LC_ALL=$single_byte_locale" if exists $e->{ENV};
            $in_len = length $e->{IN_PIPE} if exists $e->{IN_PIPE};
          }
        for my $e (@new_t)
          {
            next unless ref $e eq 'HASH';
            $e->{OUT} = "$in_len\n" if exists $e->{OUT};
          }
        unshift @new_t, $test_name;
        push @new, \@new_t;
      }
    push @Tests, @new;
  };

my $save_temps = $ENV{DEBUG};
my $verbose = $ENV{VERBOSE};

my $fail = run_tests ($prog, $prog, \@Tests, $save_temps, $verbose);
exit $fail;
