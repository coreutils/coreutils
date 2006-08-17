# Test "ls".

# Copyright (C) 1997 Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
# 02110-1301, USA.

package Test;
require 5.002;
use strict;

$Test::input_via_default = {FILE => 0};

my @tv = (
# test name, options, input, expected output, expected return code
#
['q1', '-b', sub{('""""" "', 'b')}, '"""""\\ " b', 0],
['q2', '-b -Q', sub{('""""" "', 'b')}, '\\"\\"\\"\\"\\"\\ \\" b', 0],
);

sub test_vector
{
  my $t;
  foreach $t (@tv)
    {
      # FIXME: append newline
      my ($test_name, $flags, $in, $exp, $ret) = @$t;

    }

  return @tv;
}

1;
