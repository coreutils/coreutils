package Test;
require 5.002;
use strict;

my $nl = "\n";
@Test::t = (
# test input		flags		expected output	expected return code
#
['1', "a:b:c$nl",	'-d: -f1,3-',		"a:c\n",		0],
['2', "a:b:c\n",	'-d: -f1,3-',		"a:c\n",		0],
['3', "a:b:c$nl",	'-d: -f2-',		"b:c\n",		0],
['4', "a:b:c$nl",	'-d: -f4',		"\n",			0],
['5', "",		'-d: -f4',		"",			0],
['6', "123$nl",	'-c4',			"\n",				0],
['7', "123",		'-c4',			"\n",			0],
['8', "123\n1",	'-c4',			"\n\n",				0],
['9', "",		'-c4',			"",			0],
['a', "a:b:c$nl",	'-s -d: -f3-',		"c\n",			0],
['b', "a:b:c$nl",	'-s -d: -f2,3',		"b:c\n",		0],
['c', "a:b:c$nl",	'-s -d: -f1,3',		"a:c\n",		0],
# Trailing colon should not be output
['d', "a:b:c:$nl",	'-s -d: -f1,3',		"a:c\n",		0],
['e', "a:b:c:$nl",	'-s -d: -f3-',		"c:\n",			0],
['f', "a:b:c:$nl",	'-s -d: -f3-4',		"c:\n",			0],
['g', "a:b:c:$nl",	'-s -d: -f3,4',		"c:\n",			0],
# Make sure -s suppresses non-delimited lines
['h', "abc\n",	'-s -d: -f2,3',		"",				0],
#
['i', ":::\n",	'-d: -f1-3',		"::\n",				0],
['j', ":::\n",	'-d: -f1-4',		":::\n",			0],
['k', ":::\n",	'-d: -f2-3',		":\n",				0],
['l', ":::\n",	'-d: -f2-4',		"::\n",				0],
['m', ":::\n",	'-s -d: -f1-3',		"::\n",				0],
['n', ":::\n",	'-s -d: -f1-4',		":::\n",			0],
['o', ":::\n",	'-s -d: -f2-3',		":\n",				0],
['p', ":::\n",	'-s -d: -f2-4',		"::\n",				0],
['q', ":::\n:\n",	'-s -d: -f2-4',		"::\n\n",		0],
['r', ":::\n:1\n",	'-s -d: -f2-4',		"::\n1\n",		0],
['s', ":::\n:a\n",	'-s -d: -f1-4',		":::\n:a\n",		0],
['t', ":::\n:1\n",	'-s -d: -f3-',		":\n\n",		0],
# Make sure it handles empty input properly, with and without -s.
['u', "",		'-s -f3-',		"",			0],
['v', "",		'-f3-',			"",			0],
# Make sure it handles empty input properly.
['w', "",		'-b 1',			"",			0],
['x', ":\n",		'-s -d: -f2-4',		"\n",			0],
# Errors
# -s may be used only with -f
['y', ":\n",		'-s -b4',		"",			1],
# You must specify bytes or fields (or chars)
['z', ":\n",		'',			"",			1],
# Empty field list
['A', ":\n",		'-f \'\'',		"",			1],
# Missing field list
['B', ":\n",		'-f',			"",			1],
# Empty byte list
['C', ":\n",		'-b \'\'',		"",			1],
# Missing byte list
['D', ":\n",		'-b',			"",			1],
);

1;
