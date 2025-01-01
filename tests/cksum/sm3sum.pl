#!/usr/bin/perl
# Test "cksum -a sm3".

# Copyright (C) 2021-2025 Free Software Foundation, Inc.

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

my $sha_degenerate =
  "1ab21d8355cfa17f8e61194831e81a8f22bec8c728fefb747ed035eb5082aa2b";

my @Tests =
    (
     ['s1', {IN=> {f=> ''}},
{OUT=>"$sha_degenerate  f\n"}],
     ['s2', {IN=> {f=> 'a'}},
{OUT=>"623476ac18f65a2909e43c7fec61b49c7e764a91a18ccb82f1917a29c86c5e88  f\n"}],
     ['s3', {IN=> {f=> 'abc'}},
{OUT=>"66c7f0f462eeedd9d1f2d46bdc10e4e24167c4875cf2f7a2297da02b8f4ba8e0  f\n"}],
     ['s4',
      {IN=> {f=> 'abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq'}},
{OUT=>"639b6cc5e64d9e37a390b192df4fa1ea0720ab747ff692b9f38c4e66ad7b8c05  f\n"}],
     ['s8', {IN=> {f=> 'a' x 1000000}},
{OUT=>"c8aaf89429554029e231941a2acc0ad61ff2a5acd8fadd25847a3a732b3b02c3  f\n"}],
    );

# Insert the arguments for each test.
my $t;
foreach $t (@Tests)
  {
    splice @$t, 1, 0, '--untagged --text -a sm3'
  }

my $save_temps = $ENV{DEBUG};
my $verbose = $ENV{VERBOSE};

my $prog = 'cksum';
my $fail = run_tests ($program_name, $prog, \@Tests, $save_temps, $verbose);
exit $fail;
