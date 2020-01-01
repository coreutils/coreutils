#!/usr/bin/perl
# Exercise basenc.

# Copyright (C) 2006-2020 Free Software Foundation, Inc.

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

# This test exercises the various encoding (other than base64/32).
# It also does not test the general options (e.g. --wrap), as that code is
# shared and tested in base64.

use strict;

(my $program_name = $0) =~ s|.*/||;
my $prog = 'basenc';

# Turn off localization of executable's output.
@ENV{qw(LANGUAGE LANG LC_ALL)} = ('C') x 3;


my $base64_in = "\x54\x0f\xdc\xf0\x0f\xaf\x4a";
my $base64_out = "VA/c8A+vSg==";
my $base64url_out = $base64_out;
$base64url_out =~ y|+/|-_|;
my $base64url_out_nl = $base64url_out;
$base64url_out_nl =~ s/(..)/\1\n/g; # add newline every two characters


my $base32_in = "\xfd\xd8\x07\xd1\xa5";
my $base32_out = "7XMAPUNF";
my $x = $base32_out;
$x =~ y|ABCDEFGHIJKLMNOPQRSTUVWXYZ234567|0123456789ABCDEFGHIJKLMNOPQRSTUV|;
my $base32hex_out = $x;

# base32 with padding and newline
my $base32_in2 = "\xFF\x00";
my $base32_out2 = "74AA====";
$x = $base32_out2;
$x =~ y|ABCDEFGHIJKLMNOPQRSTUVWXYZ234567|0123456789ABCDEFGHIJKLMNOPQRSTUV|;
my $base32hex_out2 = $x;
my $base32hex_out2_nl = $x;
$base32hex_out2_nl =~ s/(...)/\1\n/g; # Add newline every 3 characters

my $base16_in = "\xfd\xd8\x07\xd1\xa5";
my $base16_out = "FDD807D1A5";

my $z85_in = "\x86\x4F\xD2\x6F\xB5\x59\xF7\x5B";
my $z85_out = 'HelloWorld';

my $base2lsbf_ab = "1000011001000110";
my $base2lsbf_ab_nl = $base2lsbf_ab;
$base2lsbf_ab_nl =~ s/(...)/\1\n/g; # Add newline every 3 characters
my $base2msbf_ab = "0110000101100010";
my $base2msbf_ab_nl = $base2msbf_ab;
$base2msbf_ab_nl =~ s/(...)/\1\n/g; # Add newline every 3 characters

my $try_help = "Try '$prog --help' for more information.\n";

my @Tests =
(
 # These are mainly for higher coverage
 ['help', '--help',        {IN=>''}, {OUT=>""}, {OUT_SUBST=>'s/.*//sm'}],

 # Typical message is " unrecognized option '--foobar'", but on
 # Open/NetBSD it is  " unknown option -- foobar".
 ['error', '--foobar',     {IN=>''}, {OUT=>""}, {EXIT=>1},
  {ERR=>"$prog: foobar\n" . $try_help },
  {ERR_SUBST=>"s/(unrecognized|unknown) option [-' ]*foobar[' ]*/foobar/"}],

 ['noenc', '',    {IN=>''}, {EXIT=>1},
  {ERR=>"$prog: missing encoding type\n" . $try_help }],

 ['extra', '--base64 A B',  {IN=>''}, {EXIT=>1},
  {ERR=>"$prog: extra operand 'B'\n" . $try_help}],


 ['empty1', '--base64',    {IN=>''}, {OUT=>""}],
 ['empty2', '--base64url', {IN=>''}, {OUT=>""}],
 ['empty3', '--base32',    {IN=>''}, {OUT=>""}],
 ['empty4', '--base32hex', {IN=>''}, {OUT=>""}],
 ['empty5', '--base16',    {IN=>''}, {OUT=>""}],
 ['empty6', '--base2msbf', {IN=>''}, {OUT=>""}],
 ['empty7', '--base2lsbf', {IN=>''}, {OUT=>""}],
 ['empty8', '--z85',       {IN=>''}, {OUT=>""}],




 ['b64_1',  '--base64',       {IN=>$base64_in},     {OUT=>$base64_out}],
 ['b64_2',  '--base64 -d',    {IN=>$base64_out},    {OUT=>$base64_in}],
 ['b64_3',  '--base64 -d -i', {IN=>'&'.$base64_out},{OUT=>$base64_in}],

 ['b64u_1', '--base64url',       {IN=>$base64_in},       {OUT=>$base64url_out}],
 ['b64u_2', '--base64url -d',  {IN=>$base64url_out},        {OUT=>$base64_in}],
 ['b64u_3', '--base64url -di', {IN=>'&'.$base64url_out}   , {OUT=>$base64_in}],
 ['b64u_4', '--base64url -di', {IN=>'/'.$base64url_out.'+'},{OUT=>$base64_in}],
 ['b64u_5', '--base64url -d',  {IN=>$base64url_out_nl},     {OUT=>$base64_in}],
 ['b64u_6', '--base64url -di', {IN=>$base64url_out_nl},     {OUT=>$base64_in}],
 # ensure base64url fails to decode base64 input with "+" and "/"
 ['b64u_7', '--base64url -d',  {IN=>$base64_out},
  {EXIT=>1},  {ERR=>"$prog: invalid input\n"}],




 ['b32_1',  '--base32',       {IN=>$base32_in},     {OUT=>$base32_out}],
 ['b32_2',  '--base32 -d',    {IN=>$base32_out},    {OUT=>$base32_in}],
 ['b32_3',  '--base32 -d -i', {IN=>'&'.$base32_out},{OUT=>$base32_in}],
 ['b32_4',  '--base32',       {IN=>$base32_in2},    {OUT=>$base32_out2}],
 ['b32_5',  '--base32 -d',    {IN=>$base32_out2},   {OUT=>$base32_in2}],
 ['b32_6',  '--base32 -d -i', {IN=>$base32_out2},   {OUT=>$base32_in2}],



 ['b32h_1', '--base32hex',       {IN=>$base32_in},      {OUT=>$base32hex_out}],
 ['b32h_2', '--base32hex -d',    {IN=>$base32hex_out}, {OUT=>$base32_in}],
 ['b32h_3', '--base32hex -d -i', {IN=>'/'.$base32hex_out}, {OUT=>$base32_in}],
 ['b32h_4', '--base32hex -d -i', {IN=>'W'.$base32hex_out}, {OUT=>$base32_in}],
 ['b32h_5', '--base32hex -d',    {IN=>$base32hex_out.'W'}, , {OUT=>$base32_in},
  {EXIT=>1},  {ERR=>"$prog: invalid input\n"}],
 ['b32h_6', '--base32hex -d',    {IN=>$base32hex_out.'/'}, {OUT=>$base32_in},
  {EXIT=>1},  {ERR=>"$prog: invalid input\n"}],
 ['b32h_7',  '--base32hex',      {IN=>$base32_in2},     {OUT=>$base32hex_out2}],
 ['b32h_8',  '--base32hex -d',   {IN=>$base32hex_out2},     {OUT=>$base32_in2}],
 ['b32h_9',  '--base32hex -di',  {IN=>$base32hex_out2},     {OUT=>$base32_in2}],
 ['b32h_10', '--base32hex -d',   {IN=>$base32hex_out2_nl},  {OUT=>$base32_in2}],
 ['b32h_11', '--base32hex -di',  {IN=>$base32hex_out2_nl},  {OUT=>$base32_in2}],



 ['b16_1', '--base16',        {IN=>$base16_in},       {OUT=>$base16_out}],
 ['b16_2', '--base16 -d',     {IN=>$base16_out},      {OUT=>$base16_in}],
 ['b16_3', '--base16 -d -i',  {IN=>'&'. $base16_out}, {OUT=>$base16_in}],
 ['b16_4', '--base16 -d -i',  {IN=>$base16_out.'G'},  {OUT=>$base16_in}],
 ['b16_5', '--base16 -d',     {IN=>'.'}, {EXIT=>1},
  {ERR=>"$prog: invalid input\n"}],
 ['b16_6', '--base16 -d',     {IN=>'='}, {EXIT=>1},
  {ERR=>"$prog: invalid input\n"}],
 ['b16_7', '--base16 -d',     {IN=>'G'}, {EXIT=>1},
  {ERR=>"$prog: invalid input\n"}],
 ['b16_8', '--base16 -d',     {IN=>"AB\nCD"}, {OUT=>"\xAB\xCD"}],



 ['b2m_1', '--base2m',        {IN=>"\xC1"},       {OUT=>"11000001"}],
 ['b2m_2', '--base2m -d',     {IN=>'11000001'},   {OUT=>"\xC1"}],
 ['b2m_3', '--base2m -d',     {IN=>"110\n00001"}, {OUT=>"\xC1"}],
 ['b2m_4', '--base2m -di',    {IN=>"110x00001"},  {OUT=>"\xC1"}],
 ['b2m_5', '--base2m -d',     {IN=>"110x00001"},  {EXIT=>1},
  {ERR=>"$prog: invalid input\n"}],
 ['b2m_6', '--base2m -d',     {IN=>"11000001x"},  {OUT=>"\xC1"}, {EXIT=>1},
  {ERR=>"$prog: invalid input\n"}],
 ['b2m_7', '--base2m -d',     {IN=>"1"},      {EXIT=>1},
  {ERR=>"$prog: invalid input\n"}],
 ['b2m_8', '--base2m -d',     {IN=>"1000100"}, {EXIT=>1},
  {ERR=>"$prog: invalid input\n"}],
 ['b2m_9', '--base2m -d',     {IN=>"100010000000000"}, {OUT=>"\x88"}, {EXIT=>1},
  {ERR=>"$prog: invalid input\n"}],
 ['b2m_10','--base2m',        {IN=>"ab"},       {OUT=>$base2msbf_ab}],
 ['b2m_11','--base2m -d',     {IN=>$base2msbf_ab},     {OUT=>"ab"}],
 ['b2m_12','--base2m -d',     {IN=>$base2msbf_ab_nl},  {OUT=>"ab"}],


 ['b2l_1', '--base2l',        {IN=>"\x83"},       {OUT=>"11000001"}],
 ['b2l_2', '--base2l -d',     {IN=>'11000001'},   {OUT=>"\x83"}],
 ['b2l_3', '--base2l -d',     {IN=>"110\n00001"}, {OUT=>"\x83"}],
 ['b2l_4', '--base2l -di',    {IN=>"110x00001"},  {OUT=>"\x83"}],
 ['b2l_5', '--base2l -d',     {IN=>"110x00001"},  {EXIT=>1},
  {ERR=>"$prog: invalid input\n"}],
 ['b2l_6', '--base2l -d',     {IN=>"11000001x"},  {OUT=>"\x83"}, {EXIT=>1},
  {ERR=>"$prog: invalid input\n"}],
 ['b2l_7', '--base2l -d',     {IN=>"1"},      {EXIT=>1},
  {ERR=>"$prog: invalid input\n"}],
 ['b2l_8', '--base2l -d',     {IN=>"1000100"}, {EXIT=>1},
  {ERR=>"$prog: invalid input\n"}],
 ['b2l_9', '--base2l -d',     {IN=>"100010000000000"}, {OUT=>"\x11"}, {EXIT=>1},
  {ERR=>"$prog: invalid input\n"}],
 ['b2l_10','--base2l',        {IN=>"ab"},       {OUT=>$base2lsbf_ab}],
 ['b2l_11','--base2l -d',     {IN=>$base2lsbf_ab},     {OUT=>"ab"}],
 ['b2l_12','--base2l -d',     {IN=>$base2lsbf_ab_nl},  {OUT=>"ab"}],





 ['z85_1', '--z85',        {IN=>$z85_in},        {OUT=>$z85_out}],
 ['z85_2', '--z85 -d',     {IN=>$z85_out},       {OUT=>$z85_in}],
 ['z85_3', '--z85 -d -i',  {IN=>'~'. $z85_out},  {OUT=>$z85_in}],
 ['z85_4', '--z85 -d -i',  {IN=>' '. $z85_out},  {OUT=>$z85_in}],
 ['z85_5', '--z85 -d',     {IN=>'%j$qP'},  {OUT=>"\xFF\xDD\xBB\x99"}],
 ['z85_6', '--z85 -d -i',  {IN=>'%j~$qP'}, {OUT=>"\xFF\xDD\xBB\x99"}],

 # z85 encoding require input to be multiple of 5 octets
 ['z85_7', '--z85 -d',  {IN=>'hello'},   {OUT=>"5jXu"}],
 ['z85_8', '--z85 -d',  {IN=>'helloX'},  {OUT=>"5jXu"},  {EXIT=>1},
  {ERR=>"$prog: invalid input\n"}],
 ['z85_9', '--z85 -d',  {IN=>"he\nl\nlo"},   {OUT=>"5jXu"}],

 # Invalid input characters (space ~ ")
 ['z85_10', '--z85 -d',  {IN=>' j$qP'}, {EXIT=>1},
  {ERR=>"$prog: invalid input\n"}],
 ['z85_11', '--z85 -d',  {IN=>'%j$q~'}, {EXIT=>1},
  {ERR=>"$prog: invalid input\n"}],
 ['z85_12', '--z85 -d',  {IN=>'%j$"P'}, {EXIT=>1},
  {ERR=>"$prog: invalid input\n"}],

 # Invalid length (binary input must be a multiple of 4 octets,
 # z85-encoded input must be a multiple of 5 octets)
 ['z85_20', '--z85',  {IN=>'A'}, {EXIT=>1},
  {ERR=>"$prog: invalid input (length must be multiple of 4 characters)\n"}],
 ['z85_21', '--z85',  {IN=>'AB'}, {EXIT=>1},
  {ERR=>"$prog: invalid input (length must be multiple of 4 characters)\n"}],
 ['z85_22', '--z85',  {IN=>'ABC'}, {EXIT=>1},
  {ERR=>"$prog: invalid input (length must be multiple of 4 characters)\n"}],
 ['z85_23', '--z85',  {IN=>'ABCD'}, {OUT=>'k%^}b'}],
 ['z85_24', '--z85',  {IN=>'ABCDE'}, {EXIT=>1},
  {ERR=>"$prog: invalid input (length must be multiple of 4 characters)\n"}],

 ['z85_30', '--z85 -d',  {IN=>'A'}, {EXIT=>1},
  {ERR=>"$prog: invalid input\n"}],
 ['z85_31', '--z85 -d',  {IN=>'AB'}, {EXIT=>1},
  {ERR=>"$prog: invalid input\n"}],
 ['z85_32', '--z85 -d',  {IN=>'ABC'}, {EXIT=>1},
  {ERR=>"$prog: invalid input\n"}],
 ['z85_33', '--z85 -d',  {IN=>'ABCD'}, {EXIT=>1},
  {ERR=>"$prog: invalid input\n"}],
 ['z85_34', '--z85 -d',  {IN=>'ABCDE'}, {OUT=>"\x71\x61\x9e\xb6"}],
 ['z85_35', '--z85 -d',  {IN=>'ABCDEF'},{OUT=>"\x71\x61\x9e\xb6"},
  {EXIT=>1}, {ERR=>"$prog: invalid input\n"}],

 # largest possible value
 ['z85_40', '--z85',    {IN=>"\xFF\xFF\xFF\xFF"},{OUT=>"%nSc0"}],
 ['z85_41', '--z85 -d', {IN=>"%nSc0"}, {OUT=>"\xFF\xFF\xFF\xFF"}],
 # Invalid encoded data - will decode to more than 0xFFFFFFFF
 ['z85_42', '--z85 -d', {IN=>"%nSc1"}, {EXIT=>1},
  {ERR=>"$prog: invalid input\n"}],
 ['z85_43', '--z85 -d', {IN=>"%nSd0"}, {EXIT=>1},
  {ERR=>"$prog: invalid input\n"}],
 ['z85_44', '--z85 -d', {IN=>"%nTc0"}, {EXIT=>1},
  {ERR=>"$prog: invalid input\n"}],
 ['z85_45', '--z85 -d', {IN=>"%oSc0"}, {EXIT=>1},
  {ERR=>"$prog: invalid input\n"}],
 ['z85_46', '--z85 -d', {IN=>'$nSc0'}, {EXIT=>1},
  {ERR=>"$prog: invalid input\n"}],
 ['z85_47', '--z85 -d', {IN=>'#0000'}, {EXIT=>1},
  {ERR=>"$prog: invalid input\n"}],
);

# Prepend the command line argument and append a newline to end
# of each expected 'OUT' string.
my $t;

Test:
foreach $t (@Tests)
  {
      foreach my $e (@$t)
      {
          ref $e && ref $e eq 'HASH' && defined $e->{OUT_SUBST}
          and next Test;
      }

      push @$t, {OUT_SUBST=>'s/\n$//s'};
  }



my $save_temps = $ENV{DEBUG};
my $verbose = $ENV{VERBOSE};

my $fail = run_tests ($program_name, $prog, \@Tests, $save_temps, $verbose);

exit $fail;
