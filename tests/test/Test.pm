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

     ['streq-1', 't = t', {}, '', 0],
     ['streq-2', 't = f', {}, '', 1],
     ['strne-1', 't != t', {}, '', 1],
     ['strne-2', 't != f', {}, '', 0],

     ['and-1', 't -a t', {}, '', 0],
     ['and-2', '"" -a t', {}, '', 1],
     ['and-3', 't -a ""', {}, '', 1],
     ['and-4', '"" -a ""', {}, '', 1],

     ['or-1', 't -o t', {}, '', 0],
     ['or-2', '"" -o t', {}, '', 0],
     ['or-3', 't -o ""', {}, '', 0],
     ['or-4', '"" -o ""', {}, '', 1],

     ['eq-1', '9 -eq 9', {}, '', 0],
     ['eq-2', '0 -eq 0', {}, '', 0],
     ['eq-3', '0 -eq 00', {}, '', 0],
     ['eq-4', '8 -eq 9', {}, '', 1],
     ['eq-5', '1 -eq 0', {}, '', 1],

     ['gt-1', '5 -gt 5', {}, '', 1],
     ['gt-2', '5 -gt 4', {}, '', 0],
     ['gt-3', '4 -gt 5', {}, '', 1],
     ['gt-4', '-1 -gt -2', {}, '', 0],

     ['lt-1', '5 -lt 5', {}, '', 1],
     ['lt-2', '5 -lt 4', {}, '', 1],
     ['lt-3', '4 -lt 5', {}, '', 0],
     ['lt-4', '-1 -lt -2', {}, '', 1],

     # This evokes `test: 0x0: integer expression expected'.
     ['inv-1', '0x0 -eq 00', {}, '', 1],

     ['t1', "-t", {}, '', 1],
     ['t2', "-t 1", {}, '', 1],
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

  # Generate a negated and a double-negated version of each test.
  # There are a few exceptions.
  my %not_invertible = map {$_ => 1} qw (1a inv-1 t1);
  foreach $t (@tvec)
    {
      my ($test_name, $flags, $in, $exp, $ret) = @$t;
      next if $not_invertible{$test_name};
      push (@tv, ["N-$test_name", "! '(' $flags ')'", $in, $exp, 1 - $ret]);
      push (@tv, ["NN-$test_name", "! ! '(' $flags ')'", $in, $exp, $ret]);
    }

  return (@tv, @tvec);
}

1;
