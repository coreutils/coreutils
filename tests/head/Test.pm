package Test;
require 5.002;
use strict;

my @tv = (
# test name, options, input, expected output, expected return code
#
['idem-0', '', "", "", 0],
['idem-1', '', "a", "a", 0],
['idem-2', '', "\n", "\n", 0],
['idem-3', '', "a\n", "a\n", 0],

['basic-0-10', '',
 "1\n2\n3\n4\n5\n6\n7\n8\n9\na\n",
 "1\n2\n3\n4\n5\n6\n7\n8\n9\na\n", 0],

['basic-0-09', '',
 "1\n2\n3\n4\n5\n6\n7\n8\n9\n",
 "1\n2\n3\n4\n5\n6\n7\n8\n9\n", 0],

['basic-0-11', '',
 "1\n2\n3\n4\n5\n6\n7\n8\n9\na\nb\n",
 "1\n2\n3\n4\n5\n6\n7\n8\n9\na\n", 0],


);

sub test_vector
{
  my $t;
  foreach $t (@tv)
    {
      my ($test_name, $flags, $in, $exp, $ret) = @$t;
      $Test::input_via{$test_name} = {REDIR => 0, FILE => 0, PIPE => 0}
    }

  return @tv;
}

1;
