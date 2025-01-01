#!/usr/bin/perl
# Exercise expr with multibyte input

# Copyright (C) 2017-2025 Free Software Foundation, Inc.

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

(my $ME = $0) =~ s|.*/||;

my $limits = getlimits ();
my $UINTMAX_OFLOW = $limits->{UINTMAX_OFLOW};

(my $program_name = $0) =~ s|.*/||;
my $prog = 'expr';

my $locale = $ENV{LOCALE_FR_UTF8};
! defined $locale || $locale eq 'none'
  and CuSkip::skip "$ME: this test requires FR-UTF8 locale\n";


=pod
ἔκφρασις (ekphrasis) - "expression" in Ancient Greek.
=cut
my $expression = "\x{1F14}\x{3BA}\x{3C6}\x{3C1}\x{3B1}\x{3C3}\x{3B9}\x{3C2}";


## NOTE about tests locales:
## Tests starting with 'mb' will have {ENV=>"LC_ALL=$locale"}
## added to them automatically - results are multibyte-aware.
## Tests starting with 'sb' have the same input but will be
## run under C locale and will be treated as single-bytes.
## This enables interleaving C/UTF8 tests
## (for easier comparison of expected results).

my @Tests =
  (
   ### length expressions ###

   # sanity check
   ['mb-l1', 'length abcdef',   {OUT=>"6"}],
   ['st-l1', 'length abcdef',   {OUT=>"6"}],

   # A single multibyte character in the beginning of the string
   # \xCE\xB1 is UTF-8 for "U+03B1 GREEK SMALL LETTER ALPHA"
   ['mb-l2', "length \xCE\xB1bcdef",   {OUT=>"6"}],
   ['st-l2', "length \xCE\xB1bcdef",   {OUT=>"7"}],

   # A single multibyte character in the middle of the string
   # \xCE\xB4 is UTF-8 for "U+03B4 GREEK SMALL LETTER DELTA"
   ['mb-l3', "length abc\xCE\xB4ef",   {OUT=>"6"}],
   ['st-l3', "length abc\xCE\xB4ef",   {OUT=>"7"}],

   # A single multibyte character in the end of the string
   ['mb-l4', "length fedcb\xCE\xB1",   {OUT=>"6"}],
   ['st-l4', "length fedcb\xCE\xB1",   {OUT=>"7"}],

   # A invalid multibyte sequence
   ['mb-l5', "length \xB1aaa",   {OUT=>"4"}],
   ['st-l5', "length \xB1aaa",   {OUT=>"4"}],

   # An incomplete multibyte sequence at the end of the string
   ['mb-l6', "length aaa\xCE",   {OUT=>"4"}],
   ['st-l6', "length aaa\xCE",   {OUT=>"4"}],

   # An incomplete multibyte sequence at the end of the string
   ['mb-l7', "length $expression",   {OUT=>"8"}],
   ['st-l7', "length $expression",   {OUT=>"17"}],



   ### index expressions ###

   # sanity check
   ['mb-i1', 'index abcdef fb',   {OUT=>"2"}],
   ['st-i1', 'index abcdef fb',   {OUT=>"2"}],

   # Search for a single-octet
   ['mb-i2', "index \xCE\xB1bc\xCE\xB4ef b",   {OUT=>"2"}],
   ['st-i2', "index \xCE\xB1bc\xCE\xB4ef b",   {OUT=>"3"}],
   ['mb-i3', "index \xCE\xB1bc\xCE\xB4ef f",   {OUT=>"6"}],
   ['st-i3', "index \xCE\xB1bc\xCE\xB4ef f",   {OUT=>"8"}],

   # Search for multibyte character.
   # In the C locale, the search string is treated as two octets.
   # the first of them (\xCE) matches the first octet of the input string.
   ['mb-i4', "index \xCE\xB1bc\xCE\xB4ef \xCE\xB4",   {OUT=>"4"}],
   ['st-i4', "index \xCE\xB1bc\xCE\xB4ef \xCE\xB4",   {OUT=>"1"}],

   # Invalid multibyte sequence in the input string, treated as a single octet.
   ['mb-i5', "index \xCEbc\xCE\xB4ef \xCE\xB4",   {OUT=>"4"}],
   ['st-i5', "index \xCEbc\xCE\xB4ef \xCE\xB4",   {OUT=>"1"}],

   # Invalid multibyte sequence in the search string, treated as a single octet.
   # In multibyte locale, there should be no match, expr returns and prints
   # zero, and terminates with exit-code 1 (as per POSIX).
   ['mb-i6', "index \xCE\xB1bc\xCE\xB4ef \xB4",   {OUT=>"0"}, {EXIT=>1}],
   ['st-i6', "index \xCE\xB1bc\xCE\xB4ef \xB4",   {OUT=>"6"}],

   # Edge-case: invalid multibyte sequence BOTH in the input string
   # and in the search string: expr should find a match.
   ['mb-i7', "index \xCE\xB1bc\xB4ef \xB4",       {OUT=>"4"}],


   ### substr expressions ###

   # sanity check
   ['mb-s1', 'substr abcdef 2 3',   {OUT=>"bcd"}],
   ['st-s1', 'substr abcdef 2 3',   {OUT=>"bcd"}],

   ['mb-s2', "substr \xCE\xB1bc\xCE\xB4ef 1 1",   {OUT=>"\xCE\xB1"}],
   ['st-s2', "substr \xCE\xB1bc\xCE\xB4ef 1 1",   {OUT=>"\xCE"}],

   ['mb-s3', "substr \xCE\xB1bc\xCE\xB4ef 3 2",   {OUT=>"c\xCE\xB4"}],
   ['st-s3', "substr \xCE\xB1bc\xCE\xB4ef 3 2",   {OUT=>"bc"}],

   ['mb-s4', "substr \xCE\xB1bc\xCE\xB4ef 4 1",   {OUT=>"\xCE\xB4"}],
   ['st-s4', "substr \xCE\xB1bc\xCE\xB4ef 4 1",   {OUT=>"c"}],

   ['mb-s5', "substr \xCE\xB1bc\xCE\xB4ef 4 2",   {OUT=>"\xCE\xB4e"}],
   ['st-s5', "substr \xCE\xB1bc\xCE\xB4ef 4 2",   {OUT=>"c\xCE"}],

   ['mb-s6', "substr \xCE\xB1bc\xCE\xB4ef 6 1",   {OUT=>"f"}],
   ['st-s6', "substr \xCE\xB1bc\xCE\xB4ef 6 1",   {OUT=>"\xB4"}],

   ['mb-s7', "substr \xCE\xB1bc\xCE\xB4ef 7 1",   {OUT=>""}, {EXIT=>1}],
   ['st-s7', "substr \xCE\xB1bc\xCE\xB4ef 7 1",   {OUT=>"e"}],

   # Invalid multibyte sequences
   ['mb-s8', "substr \xCE\xB1bc\xB4ef 3 3",   {OUT=>"c\xB4e"}],
   ['st-s8', "substr \xCE\xB1bc\xB4ef 3 3",   {OUT=>"bc\xB4"}],


   ### match expressions ###

   # sanity check
   ['mb-m1', 'match abcdef ab',   {OUT=>"2"}],
   ['st-m1', 'match abcdef ab',   {OUT=>"2"}],
   ['mb-m2', 'match abcdef "\(ab\)"',   {OUT=>"ab"}],
   ['st-m2', 'match abcdef "\(ab\)"',   {OUT=>"ab"}],

   # The regex engine should match the '.' to the first multibyte character.
   ['mb-m3', "match \xCE\xB1bc\xCE\xB4ef .bc", {OUT=>"3"}],
   ['st-m3', "match \xCE\xB1bc\xCE\xB4ef .bc", {OUT=>"0"}, {EXIT=>1}],

   # The opposite of the previous test: two dots should only match
   # the two octets in single-byte locale.
   ['mb-m4', "match \xCE\xB1bc\xCE\xB4ef ..bc", {OUT=>"0"}, {EXIT=>1}],
   ['st-m4', "match \xCE\xB1bc\xCE\xB4ef ..bc", {OUT=>"4"}],

   # Match with grouping - a single dot should return the two octets
   ['mb-m5', "match \xCE\xB1bc\xCE\xB4ef '\\(.b\\)c'", {OUT=>"\xCE\xB1b"}],
   ['st-m5', "match \xCE\xB1bc\xCE\xB4ef '\\(.b\\)c'", {OUT=>""}, {EXIT=>1}],

   # Invalid multibyte sequences - regex should not match in multibyte locale
   # (POSIX requirement)
   ['mb-m6', "match \xCEbc\xCE\xB4ef '\\(.\\)'", {OUT=>""}, {EXIT=>1}],
   ['st-m6', "match \xCEbc\xCE\xB4ef '\\(.\\)'", {OUT=>"\xCE"}],


   # Character classes: in the multibyte case, the regex engine understands
   # there is a single multibyte character in the brackets.
   # In the single byte case, the regex engine sees two octets in the character
   # class ('\xCE' and '\xB1') - and it matches the first one.
   ['mb-m7', "match \xCE\xB1bc\xCE\xB4e '\\([\xCE\xB1]\\)'", {OUT=>"\xCE\xB1"}],
   ['st-m7', "match \xCE\xB1bc\xCE\xB4e '\\([\xCE\xB1]\\)'", {OUT=>"\xCE"}],

  );


# Append a newline to end of each expected 'OUT' string.
my $t;
foreach $t (@Tests)
  {
    my $arg1 = $t->[1];
    my $e;
    foreach $e (@$t)
      {
        $e->{OUT} .= "\n"
          if ref $e eq 'HASH' and exists $e->{OUT};
      }
  }


# Force multibyte locale in all tests.
#
# NOTE about the ERR_SUBST:
# The error tests above (e1/e2/e3/e4) expect error messages in C locale
# having single-quote character (ASCII 0x27).
# In UTF-8 locale, the error messages will use:
#  'LEFT SINGLE QUOTATION MARK'  (U+2018) (UTF8: 0xE2 0x80 0x98)
#  'RIGHT SINGLE QUOTATION MARK' (U+2019) (UTF8: 0xE2 0x80 0x99)
# So we replace them with ascii single-quote and the results will
# match the expected error string.
if ($locale ne 'C')
  {
    my @new;
    foreach my $t (@Tests)
      {
        my ($tname) = @$t;
        if ($tname =~ /^mb/)
          {
            push @$t, ({ENV => "LC_ALL=$locale"},
                       {ERR_SUBST => "s/\xe2\x80[\x98\x99]/'/g"});
          }
      }
  }


my $save_temps = $ENV{DEBUG};
my $verbose = $ENV{VERBOSE};

my $fail = run_tests ($program_name, $prog, \@Tests, $save_temps, $verbose);
exit $fail;
