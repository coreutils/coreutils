# -*-perl-*-
package Test;
require 5.002;
use strict;

# Always copy filename arguments from $srcdir to current directory
# before running ls on them.  Otherwise, we'd have to factor out
# differences in $srcdir from the expected output.
$Test::copy_fileargs_default = 1;

sub test_vector
{
  my @tv =
    (
     # test-name options input expected-output expected-return-code
     #
     ['q-', '', {"q\a" => 'contents'}, "q\a\n", 0],
     ['q-N', '-N', {"q\a" => ''}, "q\a\n", 0],
     ['q-q', '-q', {"q\a" => ''}, "q?\n", 0],
     ['q-Q', '-Q', {"q\a" => ''}, "\"q\\a\"\n", 0],
     ['q-qs-lit', '--quoting=literal', {"q\a" => ''}, "q\a\n", 0],
     ['q-qs-sh', '--quoting=shell', {"q\a" => ''}, "q\a\n", 0],
     ['q-qs-sh-a', '--quoting=shell-always', {"q\a" => ''}, "'q\a'\n", 0],
     ['q-qs-c', '--quoting=c', {"q\a" => ''}, "\"q\\a\"\n", 0],
     ['q-qs-esc', '--quoting=escape', {"q\a" => ''}, "q\\a\n", 0],
     );

  return @tv;
}

1;
