package Test;
require 5.002;
use strict;

$Test::input_via_default = {REDIR => 0, FILE => 0, PIPE => 0};

my @tv = (
# test flags		input		expected output	expected return code
#
['1', '-d: -f1,3-',	"a:b:c\n",		"a:c\n",		0],
['2', '-d: -f1,3-',	"a:b:c\n",		"a:c\n",		0],
['3', '-d: -f2-',	"a:b:c\n",		"b:c\n",		0],
['4', '-d: -f4',	"a:b:c\n",		"\n",			0],
['5', '-d: -f4',	"",			"",			0],
['6', '-c4',		"123\n",		"\n",			0],
['7', '-c4',		"123",			"\n",			0],
['8', '-c4',		"123\n1",		"\n\n",			0],
['9', '-c4',		"",			"",			0],
['a', '-s -d: -f3-',	"a:b:c\n",		"c\n",			0],
['b', '-s -d: -f2,3',	"a:b:c\n",		"b:c\n",		0],
['c', '-s -d: -f1,3',	"a:b:c\n",		"a:c\n",		0],
# Trailing colon should not be output
['d', '-s -d: -f1,3',	"a:b:c:\n",		"a:c\n",		0],
['e', '-s -d: -f3-',	"a:b:c:\n",		"c:\n",			0],
['f', '-s -d: -f3-4',	"a:b:c:\n",		"c:\n",			0],
['g', '-s -d: -f3,4',	"a:b:c:\n",		"c:\n",			0],
# Make sure -s suppresses non-delimited lines
['h', '-s -d: -f2,3',	"abc\n",		"",			0],
#
['i', '-d: -f1-3',	":::\n",		"::\n",			0],
['j', '-d: -f1-4',	":::\n",		":::\n",		0],
['k', '-d: -f2-3',	":::\n",		":\n",			0],
['l', '-d: -f2-4',	":::\n",		"::\n",			0],
['m', '-s -d: -f1-3',	":::\n",		"::\n",			0],
['n', '-s -d: -f1-4',	":::\n",		":::\n",		0],
['o', '-s -d: -f2-3',	":::\n",		":\n",			0],
['p', '-s -d: -f2-4',	":::\n",		"::\n",			0],
['q', '-s -d: -f2-4',	":::\n:\n",		"::\n\n",		0],
['r', '-s -d: -f2-4',	":::\n:1\n",		"::\n1\n",		0],
['s', '-s -d: -f1-4',	":::\n:a\n",		":::\n:a\n",		0],
['t', '-s -d: -f3-',	":::\n:1\n",		":\n\n",		0],
# Make sure it handles empty input properly, with and without -s.
['u', '-s -f3-',	"",			"",			0],
['v', '-f3-',		"",			"",			0],
# Make sure it handles empty input properly.
['w', '-b 1',		"",			"",			0],
['x', '-s -d: -f2-4',	":\n",			"\n",			0],
# Errors
# -s may be used only with -f
['y', '-s -b4',		":\n",			"",			1],
# You must specify bytes or fields (or chars)
['z', '',		":\n",			"",			1],
# Empty field list
['empty-fl', '-f \'\'',	":\n",			"",			1],
# Missing field list
['missing-fl', '-f',	":\n",			"",			1],
# Empty byte list
['empty-bl', '-b \'\'',	":\n",			"",			1],
# Missing byte list
['missing-bl', '-b',	":\n",			"",			1],

# This test fails with cut from textutils-1.22.
['empty-f1', '-f1',	"",			"",			0],

['empty-f2', '-f2',	"",			"",			0],

['o-delim', '-d: -f2,3 --out=_', "a:b:c\n",	"b_c\n",		0],
['nul-idelim', "-d '' -f2,3 --out=_", "a\0b\0c\n", "b_c\n",		0],
['nul-odelim', "-d: -f2,3 --out=", "a:b:c\n",	"b\0c\n",		0],
['multichar-od', "-d: -f2,3 --out=_._", "a:b:c\n", "b_._c\n",		0],

# Prior to 1.22i, you couldn't use a delimiter that would sign-extend.
['8bit-delim', "'-d\255' -f2,3 --out=_", "a\255b\255c\n", "b_c\n",	0],

# New functionality:
['out-delim1', '-c1-3,5- --output-d=:', "abcdefg\n", "abc:efg\n",	0],
# A totally overlapped field shouldn't change anything:
['out-delim2', '-c1-3,2,5- --output-d=:', "abcdefg\n", "abc:efg\n",	0],
# Partial overlap: index `2' is not at the start of a range.
['out-delim3', '-c1-3,2-4,6 --output-d=:', "abcdefg\n", "abcd:f\n",	0],
['out-delim3a', '-c1-3,2-4,6- --output-d=:', "abcdefg\n", "abcd:fg\n",	0],
# Ensure that the following two commands produce the same output.
# Before an off-by-one fix, the output from the former would not contain a `:'.
['out-delim4', '-c4-,2-3 --output-d=:', "abcdefg\n", "bc:defg\n",	0],
['out-delim5', '-c2-3,4- --output-d=:', "abcdefg\n", "bc:defg\n",	0],
# This test would fail for cut from coreutils-5.0.1 and earlier.
['out-delim6', '-c2,1-3 --output-d=:', "abc\n", "abc\n",	0],
#
['od-abut',    '-b1-2,3-4 --output-d=:', "abcd\n", "ab:cd\n",	0],
['od-overlap', '-b1-2,2   --output-d=:', "abc\n",  "ab\n",	0],
['od-overlap2', '-b1-2,2- --output-d=:', "abc\n",  "abc\n",	0],
['od-overlap3', '-b1-3,2- --output-d=:', "abcd\n",  "abcd\n",	0],
['od-overlap4', '-b1-3,2-3 --output-d=:', "abcd\n",  "abc\n",	0],
['od-overlap5', '-b1-3,1-4 --output-d=:', "abcde\n",  "abcd\n",	0],

);

# Don't use a pipe for failing tests.  Otherwise, sometimes they
# fail so early they'd evoke the `Broken pipe' message.
my $t;
foreach $t (@tv)
  {
    my ($test_name, $flags, $in, $exp, $ret) = @$t;
    $Test::input_via{$test_name} = {REDIR => 0, FILE => 0} if $ret;
  }

sub test_vector
{
  return @tv;
}

1;
