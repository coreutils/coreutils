package Test;
require 5.002;
use strict;

my $delim = chr 0247;
sub t_subst ($)
{
  (my $s = $_[0]) =~ s/:/$delim/g;
  return $s;
}

my @tv = (
# test name
#     flags       file-1 file-2    expected output   expected return code
#
['1a', '-a1',      ["a 1\n", "b\n"],   "a 1\n",          0],
['1b', '-a2',      ["a 1\n", "b\n"],   "b\n",            0], # Got "\n"
['1c', '-a1 -a2',  ["a 1\n", "b\n"],   "a 1\nb\n",       0], # Got "a 1\n\n"
['1d', '-a1',      ["a 1\nb\n", "b\n"],   "a 1\nb\n",    0],
['1e', '-a2',      ["a 1\nb\n", "b\n"],   "b\n",         0],
['1f', '-a2',      ["b\n", "a\nb\n"], "a\nb\n",          0],

['2a', '-a1 -e .',  ["a\nb\nc\n", "a x y\nb\nc\n"], "a x y\nb\nc\n", 0],
['2b', '-a1 -e . -o 2.1,2.2,2.3',  ["a\nb\nc\n", "a x y\nb\nc\n"],
 "a x y\nb . .\nc . .\n", 0],
['2c', '-a1 -e . -o 2.1,2.2,2.3',  ["a\nb\nc\nd\n", "a x y\nb\nc\n"],
 "a x y\nb . .\nc . .\n. . .\n", 0],

['3a', '-t:', ["a:1\nb:1\n", "a:2:\nb:2:\n"], "a:1:2:\nb:1:2:\n", 0],

# Just like -a1 and -a2 when there are no pairable lines
['4a', '-v 1', ["a 1\n", "b\n"],   "a 1\n",          0],
['4b', '-v 2', ["a 1\n", "b\n"],   "b\n",            0],

['4c', '-v 1', ["a 1\nb\n", "b\n"],        "a 1\n",       0],
['4d', '-v 2', ["a 1\nb\n", "b\n"],        "",            0],
['4e', '-v 2', ["b\n",      "a 1\nb\n"],   "a 1\n",       0],
['5a', '-a1 -e - -o 1.1 2.2',
 ["a 1\nb 2\n",  "a 11\nb\n"], "a 11\nb -\n", 0],
['5b', '-a1 -e - -o 1.1 2.2',
 ["apr 15\naug 20\ndec 18\nfeb 05\n", "apr 06\naug 14\ndate\nfeb 15"],
 "apr 06\naug 14\ndec -\nfeb 15\n", 0],
['5c', '-a1 -e - -o 1.1 2.2',
 ["aug 20\ndec 18\n", "aug 14\ndate\nfeb 15"],
 "aug 14\ndec -\n", 0],
['5d', '-a1 -e - -o 1.1 2.2',
 ["dec 18\n",  ""], "dec -\n", 0],
['5e', '-a2 -e - -o 1.1 2.2',
 ["apr 15\naug 20\ndec 18\nfeb 05\n", "apr 06\naug 14\ndate\nfeb 15\n"],
 "apr 06\naug 14\n- -\nfeb 15\n", 0],
['5f', '-a2 -e - -o 2.2 1.1',
 ["apr 15\naug 20\ndec 18\nfeb 05\n", "apr 06\naug 14\ndate\nfeb 15\n"],
 "06 apr\n14 aug\n- -\n15 feb\n", 0],
['5g', '-a1 -e - -o 2.2 1.1',
 ["apr 15\naug 20\ndec 18\nfeb 05\n", "apr 06\naug 14\ndate\nfeb 15\n"],
 "06 apr\n14 aug\n- dec\n15 feb\n", 0],

['5h', '-a1 -e - -o 2.2 1.1',
 ["apr 15\naug 20\ndec 18\nfeb 05\n", "apr 06\naug 14\ndate\n"],
 "06 apr\n14 aug\n- dec\n- feb\n", 0],
['5i', '-a1 -e - -o 1.1 2.2',
 ["apr 15\naug 20\ndec 18\nfeb 05\n", "apr 06\naug 14\ndate\n"],
 "apr 06\naug 14\ndec -\nfeb -\n", 0],

['5j', '-a2 -e - -o 2.2 1.1',
 ["apr 15\naug 20\ndec 18\nfeb 05\n", "apr 06\naug 14\ndate\n"],
 "06 apr\n14 aug\n- -\n", 0],
['5k', '-a2 -e - -o 2.2 1.1',
 ["apr 15\naug 20\ndec 18\nfeb 05\n", "apr 06\naug 14\ndate\n"],
  "06 apr\n14 aug\n- -\n", 0],

['5l', '-a1 -e - -o 2.2 1.1',
 ["apr 15\naug 20\ndec 18\n", "apr 06\naug 14\ndate\nfeb 15\n"],
  "06 apr\n14 aug\n- dec\n", 0],
['5m', '-a2 -e - -o 2.2 1.1',
 ["apr 15\naug 20\ndec 18\n", "apr 06\naug 14\ndate\nfeb 15\n"],
  "06 apr\n14 aug\n- -\n15 -\n", 0],

['6a', '-e -',
 ["a 1\nb 2\nd 4\n", "a 21\nb 22\nc 23\nf 26\n"],
 "a 1 21\nb 2 22\n", 0],
['6b', '-a1 -e -',
 ["a 1\nb 2\nd 4\n", "a 21\nb 22\nc 23\nf 26\n"],
 "a 1 21\nb 2 22\nd 4\n", 0],
['6c', '-a1 -e -',
 ["a 21\nb 22\nc 23\nf 26\n", "a 1\nb 2\nd 4\n"],
 "a 21 1\nb 22 2\nc 23\nf 26\n", 0],

['7a', '-a1 -e . -o 2.7',
 ["a\nb\nc\n", "a x y\nb\nc\n"], ".\n.\n.\n", 0],

['8a', '-a1 -e . -o 0,1.2',
 ["a\nb\nc\nd G\n", "a x y\nb\nc\ne\n"],
 "a .\nb .\nc .\nd G\n", 0],
['8b', '-a1 -a2 -e . -o 0,1.2',
 ["a\nb\nc\nd G\n", "a x y\nb\nc\ne\n"],
 "a .\nb .\nc .\nd G\ne .\n", 0],

# From David Dyck
['9a', '', [" a 1\n b 2\n", " a Y\n b Z\n"], "a 1 Y\nb 2 Z\n", 0],

# From Tim Smithers: fixed in 1.22l
['trailing-sp', '-t: -1 1 -2 1', ["a:x \n", "a:y \n"], "a:x :y \n", 0],

# From Paul Eggert: fixed in 1.22n
['sp-vs-blank', '', ["\f 1\n", "\f 2\n"], "\f 1 2\n", 0],

# From Paul Eggert: fixed in 1.22n (this would fail on Solaris7,
# with LC_ALL set to en_US).
# Unfortunately, that Solaris7's en_US local folds case (making
# the first input file sorted) is not portable, so this test would
# fail on e.g. Linux systems, because the input to join isn't sorted.
# ['lc-collate', '', ["a 1a\nB 1B\n", "B 2B\n"], "B 1B 2B\n", 0],

# Based on a report from Antonio Rendas.  Fixed in 2.0.9.
['8-bit-t', t_subst "-t:",
 [t_subst "a:1\nb:1\n", t_subst "a:2:\nb:2:\n"],
 t_subst "a:1:2:\nb:1:2:\n", 0],

);


sub test_vector
{
  return @tv;
}

1;
