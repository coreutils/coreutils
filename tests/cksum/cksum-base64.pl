#!/usr/bin/perl
# Exercise cksum's --base64 option.

# Copyright (C) 2023-2025 Free Software Foundation, Inc.

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

# Pairs of hash,degenerate_output, given file name of "f":
my @pairs =
  (
   ['sysv', "0 0 f"],
   ['bsd', "00000     0 f"],
   ['crc', "4294967295 0 f"],
   ['crc32b', "0 0 f"],
   ['md5', "1B2M2Y8AsgTpgAmY7PhCfg=="],
   ['sha1', "2jmj7l5rSw0yVb/vlWAYkK/YBwk="],
   ['sha2', "z4PhNX7vuL3xVChQ1m2AB9Yg5AULVxXcg/SpIdNs6c5H0NE8XYXysP+DGNKHfuwvY7kxvUdBeoGlODJ6+SfaPg=="],
   ['sha3', "pp9zzKI6msXItWfcGFp1bpfJghZP4lhZ4NHcwUdcgKYVshI68fX5TBHj6UAsOsVY9QAZnZW20+MBdYWGKB3NJg=="],
   ['blake2b', "eGoC90IBWQPGxv2FJVLScpEvR0DhWEdhiobiF/cfVBnSXhAxr+5YUxOJZESTTrBLkDpoWxRIt1XVb3Aa/pvizg=="],
   ['sm3', "GrIdg1XPoX+OYRlIMegajyK+yMco/vt0ftA161CCqis="],
  );

# Return the formatted output for a given hash name/value pair.
# Use the hard-coded "f" as file name.
sub fmt ($$) {
  my ($h, $v) = @_;
  $h !~ m{^(sysv|bsd|crc|crc32b)$} and $v = uc($h). " (f) = $v";
  # BLAKE2b is inconsistent:
  $v =~ s{BLAKE2B}{BLAKE2b};
  # Our tests use 'cksum -a sha{2,3} --length=512'.
  $v =~ s/^SHA3\b/SHA3-512/;
  $v =~ s/^SHA2\b/SHA512/;
  return "$v"
}

my @Tests =
  (
   # Ensure that each of the above works with --base64:
   (map {my ($h,$v)= @$_; my $o=fmt $h,$v;
         (my $opts = $h) =~ s/^sha3$/sha3 --length=512/;
         $opts =~ s/^sha2$/sha512/;
         [$h, "--base64 -a $opts", {IN=>{f=>''}}, {OUT=>"$o\n"}]} @pairs),

   # For each that accepts --check, ensure that works with base64 digests:
   (map {my ($h,$v)= @$_; my $o=fmt $h,$v;
         ["chk-".$h, "--check --strict", {IN=>$o},
          {AUX=>{f=>''}}, {OUT=>"f: OK\n"}]}
      grep { $_->[0] !~ m{^(sysv|bsd|crc|crc32b)$} } @pairs),

   # For digests ending in "=", ensure --check fails if any "=" is removed.
   (map {my ($h,$v)= @$_; my $o=fmt $h,$v;
         ["chk-eq1-".$h, "--check", {IN=>$o}, {AUX=>{f=>''}},
          {ERR_SUBST=>"s/.*: //"}, {OUT=>''}, {EXIT=>1},
          {ERR=>"no properly formatted checksum lines found\n"}]}
      ( map {my ($h,$v)=@$_; $v =~ s/=$//; [$h,$v] }
        grep { $_->[1] =~ m{=$} } @pairs)),

   # Same as above, but for those ending in "==":
   (map {my ($h,$v)= @$_; my $o=fmt $h,$v;
         ["chk-eq2-".$h, "--check", {IN=>$o}, {AUX=>{f=>''}},
          {ERR_SUBST=>"s/.*: //"}, {OUT=>''}, {EXIT=>1},
          {ERR=>"no properly formatted checksum lines found\n"}]}
      ( map {my ($h,$v)=@$_; $v =~ s/==$//; [$h,$v] }
        grep { $_->[1] =~ m{==$} } @pairs)),

   # Trigger a read-buffer-overrun error in an early (not committed)
   # version of the --base64-adding patch.
   ['nul', '-a sha1 --check', {IN=>'\0\0\0'},
    {ERR=>"no properly formatted checksum lines found\n"},
    {ERR_SUBST=>"s/.*: //"}, {OUT=>''}, {EXIT=>1}],
  );

my $save_temps = $ENV{DEBUG};
my $verbose = $ENV{VERBOSE};

my $prog = 'cksum';
my $fail = run_tests ($program_name, $prog, \@Tests, $save_temps, $verbose);

# Ensure hash names from cksum --help match those in @pairs above.
my $help_algs = join ' ', map { m{^  ([[:alpha:]]\S+)} }
  grep { m{^  ([[:alpha:]]\S+)} } split ('\n', `cksum --help`);
my $test_algs = join ' ', map {$_->[0]} @pairs;
$help_algs eq $test_algs or die "$help_algs not equal to\n$test_algs";

exit $fail;
