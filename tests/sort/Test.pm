# -*-perl-*-
package Test;
require 5.002;
use strict;

@Test::t = (
#test   options   input   expected-output   expected-return-code
#
["01a", '', "A\nB\nC\n", "A\nB\nC\n", 0],
#
["02a", '-c', "A\nB\nC\n", '', 0],
["02b", '-c', "A\nC\nB\n", '', 1],
# This should fail because there are duplicate keys
["02c", '-cu', "A\nA\n", '', 1],
["02d", '-cu', "A\nB\n", '', 0],
["02e", '-cu', "A\nB\nB\n", '', 1],
["02f", '-cu', "B\nA\nB\n", '', 1],
#
["03a", '-k1', "B\nA\n", "A\nB\n",  0],
["03b", '-k1,1', "B\nA\n", "A\nB\n",  0],
["03c", '-k1 -k2', "A b\nA a\n", "A a\nA b\n",  0],
# FIXME: fail with a diagnostic when -k specifies field == 0
["03d", '-k0', "", "",  2],
# FIXME: fail with a diagnostic when -k specifies character == 0
["03e", '-k1.0', "", "",  2],
["03f", '-k1.1,-k0', "", "",  2],
# This is ok.
["03g", '-k1.1,1.0', "", "",  0],
# This is equivalent to 3f.
["03h", '-k1.1,1', "", "",  0],
# This too, is equivalent to 3f.
["03i", '-k1,1', "", "",  0],
#
["04a", '-nc', "2\n11\n", "",  0],
["04b", '-n', "11\n2\n", "2\n11\n", 0],
["04c", '-k1n', "11\n2\n", "2\n11\n", 0],
["04d", '-k1', "11\n2\n", "11\n2\n", 0],
["04e", '-k2', "ignored B\nz-ig A\n", "z-ig A\nignored B\n", 0],
#
["05a", '-k1,2', "A B\nA A\n", "A A\nA B\n", 0],
["05b", '-k1,2', "A B A\nA A Z\n", "A A Z\nA B A\n", 0],
["05c", '-k1 -k2', "A B A\nA A Z\n", "A A Z\nA B A\n", 0],
["05d", '-k2,2', "A B A\nA A Z\n", "A A Z\nA B A\n", 0],
["05e", '-k2,2', "A B Z\nA A A\n", "A A A\nA B Z\n", 0],
["05f", '-k2,2', "A B A\nA A Z\n", "A A Z\nA B A\n", 0],
#
["06a", '-k 1,2', "A B\nA A\n", "A A\nA B\n", 0],
["06b", '-k 1,2', "A B A\nA A Z\n", "A A Z\nA B A\n", 0],
["06c", '-k 1 -k 2', "A B A\nA A Z\n", "A A Z\nA B A\n", 0],
["06d", '-k 2,2', "A B A\nA A Z\n", "A A Z\nA B A\n", 0],
["06e", '-k 2,2', "A B Z\nA A A\n", "A A A\nA B Z\n", 0],
["06f", '-k 2,2', "A B A\nA A Z\n", "A A Z\nA B A\n", 0],
#
["07a", '-k 2,3', "9 a b\n7 a a\n", "7 a a\n9 a b\n", 0],
["07b", '-k 2,3', "a a b\nz a a\n", "z a a\na a b\n", 0],
["07c", '-k 2,3', "y k b\nz k a\n", "z k a\ny k b\n", 0],
["07d", '+1 -3', "y k b\nz k a\n", "z k a\ny k b\n", 0],
#
# report an error for `.' without following char spec
["08a", '-k 2.,3', "", "", 2],
# report an error for `,' without following POS2
["08b", '-k 2,', "", "", 2],
#
# Test new -g option.
["09a", '-g', "1e2\n2e1\n", "2e1\n1e2\n", 0],
# Make sure -n works how we expect.
["09b", '-n', "1e2\n2e1\n", "1e2\n2e1\n", 0],
["09c", '-n', "2e1\n1e2\n", "1e2\n2e1\n", 0],
["09d", '-k2g', "a 1e2\nb 2e1\n", "b 2e1\na 1e2\n", 0],
#
# Bug reported by Roger Peel" <R.Peel@ee.surrey.ac.uk>
["10a", '-t : -k 2.2,2.2', ":ba\n:ab\n", ":ba\n:ab\n", 0],
# Equivalent to above, but using obsolescent `+pos -pos' option syntax.
["10b", '-t : +1.1 -1.2', ":ba\n:ab\n", ":ba\n:ab\n", 0],
#
# The same as the preceding two, but with input lines reversed.
["10c", '-t : -k 2.2,2.2', ":ab\n:ba\n", ":ba\n:ab\n", 0],
# Equivalent to above, but using obsolescent `+pos -pos' option syntax.
["10d", '-t : +1.1 -1.2', ":ab\n:ba\n", ":ba\n:ab\n", 0],
# Try without -t...
# But note that we have to count the delimiting space at the beginning
# of each field that has it.
["10a0", '-k 2.3,2.3', "z ba\nz ab\n", "z ba\nz ab\n", 0],
["10a1", '-k 1.2,1.2', "ba\nab\n", "ba\nab\n", 0],
["10a2", '-b -k 2.2,2.2', "z ba\nz ab\n", "z ba\nz ab\n", 0],
#
# An even simpler example demonstrating the bug.
["10e", '-k 1.2,1.2', "ab\nba\n", "ba\nab\n", 0],
#
["10f", '-t : -k 1.3,1.3', ":ab\n:ba\n", ":ab\n:ba\n", 0],
["10g", '-k 1.4,1.4', "a ab\nb ba\n", "a ab\nb ba\n", 0],
#
# Exercise bug re using -b to skip trailing blanks.
["11a", '-t: -k1,1b -k2,2', "a\t:a\na :b\n", "a\t:a\na :b\n", 0],
["11b", '-t: -k1,1b -k2,2', "a :b\na\t:a\n", "a\t:a\na :b\n", 0],
["11c", '-t: -k2,2b -k3,3', "z:a\t:a\na :b\n", "z:a\t:a\na :b\n", 0],
["11d", '-t: -k2,2b -k3,3', "z:a :b\na\t:a\n", "a\t:a\nz:a :b\n", 0],
#
# Exercise bug re comparing `-' and integers.
["12a", '-n -t: +1', "a:1\nb:-\n", "b:-\na:1\n", 0],
["12b", '-n -t: +1', "b:-\na:1\n", "b:-\na:1\n", 0],
# Try some other (e.g. `X') invalid character.
["12c", '-n -t: +1', "a:1\nb:X\n", "b:X\na:1\n", 0],
["12d", '-n -t: +1', "b:X\na:1\n", "b:X\na:1\n", 0],
# From Karl Heuer
["13a", '+0.1n', "axx\nb-1\n", "b-1\naxx\n", 0],
["13b", '+0.1n', "b-1\naxx\n", "b-1\naxx\n", 0],
#
# From Carl Johnson <carlj@cjlinux.home.org>
["14a", '-d -u', "mal\nmal-\nmala\n", "mal\nmala\n", 0],
# Be sure to fix the (translate && ignore) case in keycompare.
["14b", '-f -d -u', "mal\nmal-\nmala\n", "mal\nmala\n", 0],
#
# Experiment with -i.
["15a", '-i -u', "a\na\1\n", "a\n", 0],
["15b", '-i -u', "a\n\1a\n", "a\n", 0],
["15c", '-i -u', "a\1\na\n", "a\1\n", 0],
["15d", '-i -u', "\1a\na\n", "\1a\n", 0],
["15e", '-i -u', "a\n\1\1\1\1\1a\1\1\1\1\n", "a\n", 0],
);

1;
