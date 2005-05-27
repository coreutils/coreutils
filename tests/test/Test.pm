# -*-perl-*-
package Test;
require 5.002;
use strict;

sub test_vector
{
  my @tvec =
    (
     # test-name options input expected-output expected-return-code
     #
     ['1a', '', {}, '', 1],
     ['1b', "-z ''", {}, '', 0],
     ['1c', 'any-string', {}, '', 0],
     ['1d', '-n any-string', {}, '', 0],
     ['1e', "''", {}, '', 1],
     ['1f', '-', {}, '', 0],
     ['1g', '--', {}, '', 0],
     ['1h', '-0', {}, '', 0],
     ['1i', '-f', {}, '', 0],
     ['1j', '--help', {}, '', 0],
     ['1k', '--version', {}, '', 0],

     ['streq-1', 't = t', {}, '', 0],
     ['streq-2', 't = f', {}, '', 1],
     ['streq-3', '! = !', {}, '', 0],
     ['streq-4', '= = =', {}, '', 0],
     ['streq-5', "'(' = '('", {}, '', 0],
     ['streq-6', "'(' = ')'", {}, '', 1],
     ['strne-1', 't != t', {}, '', 1],
     ['strne-2', 't != f', {}, '', 0],
     ['strne-3', '! != !', {}, '', 1],
     ['strne-4', '= != =', {}, '', 1],
     ['strne-5', "'(' != '('", {}, '', 1],
     ['strne-6', "'(' != ')'", {}, '', 0],

     ['and-1', 't -a t', {}, '', 0],
     ['and-2', "'' -a t", {}, '', 1],
     ['and-3', "t -a ''", {}, '', 1],
     ['and-4', "'' -a ''", {}, '', 1],

     ['or-1', 't -o t', {}, '', 0],
     ['or-2', "'' -o t", {}, '', 0],
     ['or-3', "t -o ''", {}, '', 0],
     ['or-4', "'' -o ''", {}, '', 1],

     ['eq-1', '9 -eq 9', {}, '', 0],
     ['eq-2', '0 -eq 0', {}, '', 0],
     ['eq-3', '0 -eq 00', {}, '', 0],
     ['eq-4', '8 -eq 9', {}, '', 1],
     ['eq-5', '1 -eq 0', {}, '', 1],
     ['eq-6', '340282366920938463463374607431768211456 -eq 0', {}, '', 1],

     ['gt-1', '5 -gt 5', {}, '', 1],
     ['gt-2', '5 -gt 4', {}, '', 0],
     ['gt-3', '4 -gt 5', {}, '', 1],
     ['gt-4', '-1 -gt -2', {}, '', 0],
     ['gt-5', '18446744073709551616 -gt -18446744073709551616', {}, '', 0],

     ['lt-1', '5 -lt 5', {}, '', 1],
     ['lt-2', '5 -lt 4', {}, '', 1],
     ['lt-3', '4 -lt 5', {}, '', 0],
     ['lt-4', '-1 -lt -2', {}, '', 1],
     ['lt-5', '-18446744073709551616 -lt 18446744073709551616', {}, '', 0],

     # This evokes `test: 0x0: integer expression expected'.
     ['inv-1', '0x0 -eq 00', {}, '', 2],

     ['t1', "-t", {}, '', 0],
     ['t2', "-t 1", {}, '', 1],

     ['paren-1', "'(' '' ')'", {}, '', 1],
     ['paren-2', "'(' '(' ')'", {}, '', 0],
     ['paren-3', "'(' ')' ')'", {}, '', 0],
     ['paren-4', "'(' ! ')'", {}, '', 0],
     ['paren-5', "'(' -a ')'", {}, '', 0],
    );

  my %inverse_op =
    (
     eq => 'ne',
     lt => 'ge',
     gt => 'le',
    );

  # Generate corresponding tests of inverse ops.
  # E.g. generate tests of `-ge' from those of `-lt'.
  my @tv;
  my $t;
  foreach $t (@tvec)
    {
      my ($test_name, $flags, $in, $exp, $ret) = @$t;

      my $op;
      for $op (qw(gt lt eq))
	{
	  if ($test_name =~ /$op-/ && $flags =~ / -$op /)
	    {
	      my $inv = $inverse_op{$op};
	      $test_name =~ s/$op/$inv/;
	      $flags =~ s/-$op/-$inv/;
	      $ret = 1 - $ret;
	      push (@tv, [$test_name, $flags, $in, $exp, $ret]);
	    }
	}
    }

  # Generate parenthesized and negated versions of each test.
  # There are a few exceptions.
  my %not_N   = map {$_ => 1} qw (1a);
  my %not_P   = map {$_ => 1} qw (1a
				  streq-6 strne-6
				  paren-1 paren-2 paren-3 paren-4 paren-5);
  foreach $t (@tvec)
    {
      my ($test_name, $flags, $in, $exp, $ret) = @$t;
      next if $ret == 2;
      push (@tv, ["N-$test_name", "! $flags", $in, $exp, 1 - $ret])
	  unless $not_N{$test_name};
      push (@tv, ["P-$test_name", "'(' $flags ')'", $in, $exp, $ret])
	  unless $not_P{$test_name};
      push (@tv, ["NP-$test_name", "! '(' $flags ')'", $in, $exp, 1 - $ret])
	  unless $not_P{$test_name};
      push (@tv, ["NNP-$test_name", "! ! '(' $flags ')'", $in, $exp, $ret])
	  unless $not_P{$test_name};
    }

  return (@tv, @tvec);
}

1;
