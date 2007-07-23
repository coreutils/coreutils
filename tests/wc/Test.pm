# Test "wc".

# Copyright (C) 1997, 2003, 2006 Free Software Foundation, Inc.

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

package Test;
require 5.002;
use strict;

$Test::input_via_stdin = 1;

my @tv = (
# test flags  input                 expected output        expected return code
['a0', '-c',  '',                   "0\n",                          0],
['a1', '-l',  '',                   "0\n",                          0],
['a2', '-w',  '',                   "0\n",                          0],
['a3', '-c',  'x',                  "1\n",                          0],
['a4', '-w',  'x',                  "1\n",                          0],
['a5', '-w',  "x y\n",              "2\n",                          0],
['a6', '-w',  "x y\nz",             "3\n",                          0],
# Remember, -l counts *newline* bytes, not logical lines.
['a7', '-l',  "x y",                "0\n",                          0],
['a8', '-l',  "x y\n",              "1\n",                          0],
['a9', '-l',  "x\ny\n",             "2\n",                          0],
['b0', '',    "",                   "0 0 0\n",                      0],
['b1', '',    "a b\nc\n",           "2 3 6\n",                      0],
['c0', '-L',  "1\n12\n",            "2\n",                          0],
['c1', '-L',  "1\n123\n1\n",        "3\n",                          0],
['c2', '-L',  "\n123456",           "6\n",                          0],
);

sub test_vector
{
  my $t;
  foreach $t (@tv)
    {
      my ($test_name, $flags, $in, $exp, $ret) = @$t;
      # By default, test both stdin-redirection and input from a pipe.
      $Test::input_via{$test_name} = {REDIR => 0, PIPE => 0};

      # But if test name ends with `-file', test only with file arg(s).
      # FIXME: unfortunately, invoking wc like `wc FILE' makes it put
      # FILE in the ouput -- and FILE is different depending on $srcdir.
      $Test::input_via{$test_name} = {FILE => 0}
        if $test_name =~ /-file$/;

      # Now that `wc FILE' (note, with no options) produces results
      # different from `cat FILE|wc', disable those two `PIPE' tests.
      $flags eq ''
	and delete $Test::input_via{$test_name}->{PIPE};
    }

  return @tv;
}

1;
