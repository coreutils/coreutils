# -*-perl-*-
package Test;
require 5.002;
use strict;

# Tell head to accept old-style options like `-1'.
$Test::env_default = ['_POSIX2_VERSION=199209'];

my @tv = (
#test   options   input   expected-output   expected-return-code
#
["n1", '-n', ".01\n0\n", "0\n.01\n", 0],
["n2", '-n', ".02\n.01\n", ".01\n.02\n", 0],
["n3", '-n', ".02\n.00\n", ".00\n.02\n", 0],
["n4", '-n', ".02\n.000\n", ".000\n.02\n", 0],
["n5", '-n', ".021\n.029\n", ".021\n.029\n", 0],

["n6", '-n', ".02\n.0*\n", ".0*\n.02\n", 0],
["n7", '-n', ".02\n.*\n", ".*\n.02\n", 0],
["n8a", '-s -n -k1,1', ".0a\n.0b\n", ".0a\n.0b\n", 0],
["n8b", '-s -n -k1,1', ".0b\n.0a\n", ".0b\n.0a\n", 0],
["n9a", '-s -n -k1,1', ".000a\n.000b\n", ".000a\n.000b\n", 0],
["n9b", '-s -n -k1,1', ".000b\n.000a\n", ".000b\n.000a\n", 0],
["n10a", '-s -n -k1,1', ".00a\n.000b\n", ".00a\n.000b\n", 0],
["n10b", '-s -n -k1,1', ".00b\n.000a\n", ".00b\n.000a\n", 0],
["n11a", '-s -n -k1,1', ".01a\n.010\n", ".01a\n.010\n", 0],
["n11b", '-s -n -k1,1', ".010\n.01a\n", ".010\n.01a\n", 0],

["01a", '', "A\nB\nC\n", "A\nB\nC\n", 0],
#
["02a", '-c', "A\nB\nC\n", '', 0],
["02b", '-c', "A\nC\nB\n", '', 1],
["02c", '-c -k1,1', "a\na b\n", '', 0],
# This should fail because there are duplicate keys
["02m", '-cu', "A\nA\n", '', 1],
["02n", '-cu', "A\nB\n", '', 0],
["02o", '-cu', "A\nB\nB\n", '', 1],
["02p", '-cu', "B\nA\nB\n", '', 1],
#
["03a", '-k1', "B\nA\n", "A\nB\n",  0],
["03b", '-k1,1', "B\nA\n", "A\nB\n",  0],
["03c", '-k1 -k2', "A b\nA a\n", "A a\nA b\n",  0],
# Fail with a diagnostic when -k specifies field == 0.
["03d", '-k0', "", "",  2],
# Fail with a diagnostic when -k specifies character == 0.
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
# Bug reported by Roger Peel <R.Peel@ee.surrey.ac.uk>
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
# The way sort works on these inputs (10f and 10g) seems wrong to me.
# See May 30 ChangeLog entry.  POSIX doesn't seem to say one way or
# the other, but that's the way all other sort implementations work.
["10f", '-t : -k 1.3,1.3', ":ab\n:ba\n", ":ba\n:ab\n", 0],
["10g", '-k 1.4,1.4', "a ab\nb ba\n", "b ba\na ab\n", 0],
#
# Exercise bug re using -b to skip trailing blanks.
["11a", '-t: -k1,1b -k2,2', "a\t:a\na :b\n", "a\t:a\na :b\n", 0],
["11b", '-t: -k1,1b -k2,2', "a :b\na\t:a\n", "a\t:a\na :b\n", 0],
["11c", '-t: -k2,2b -k3,3', "z:a\t:a\na :b\n", "z:a\t:a\na :b\n", 0],
# Before 1.22m, the first key comparison reported equality.
# With 1.22m, they compare different: "a" sorts before "a\n",
# and the second key spec isn't even used.
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

# From Erick Branderhorst -- fixed around 1.19e
["16a", '-f',
 "éminence\nüberhaupt\n's-Gravenhage\naëroclub\nAag\naagtappels\n",
 "'s-Gravenhage\nAag\naagtappels\naëroclub\néminence\nüberhaupt\n",
 0],

# This provokes a one-byte memory overrun of a malloc'd block for versions
# of sort from textutils-1.19p and before.
["17", '-c', "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n", "", 0],

# POSIX says -n no longer implies -b, so here we're comparing ` 9' and `10'.
["18a", '-k1.1,1.2n', " 901\n100\n", " 901\n100\n", 0],

# Just like above, because the global `-b' has no effect on the
# key specifier when a key-specific option (`n' in this case) is used.
["18b", '-b -k1.1,1.2n', " 901\n100\n", " 901\n100\n", 0],

# No change from above because the `b' on the key-end part of the
# key specifier makes sort ignore only trailing blanks
["18c", '-k1.1,1.2nb', " 901\n100\n", " 901\n100\n", 0],

# Here we're comparing `90' and `10', because the `b' on the key-start
# specifier makes sort ignore *leading* blanks on that key.
["18d", '-k1.1b,1.2n', " 901\n100\n", "100\n 901\n", 0],

# Equivalent to above, except it ignores both leading and trailing blanks.
["18e", '-nb -k1.1,1.2', " 901\n100\n", "100\n 901\n", 0],

# This looks odd, but works properly -- 2nd keyspec is never
# used because all lines are different.
["19a", '+0 +1nr', "b 2\nb 1\nb 3\n", "b 1\nb 2\nb 3\n", 0],

# The test *intended* by the author of the above, but using the
# more-intuitive POSIX-style -k options.
["19b", '-k1,1 -k2nr', "b 2\nb 1\nb 3\n", "b 3\nb 2\nb 1\n", 0],

# This test failed when sort-1.22 was compiled on a Next x86 system
# without optimization.  Without optimization gcc uses the buggy version
# of memcmp in the Next C library.  With optimization, gcc uses its
# (working) builtin version.  Test case form William Lewis.
["20a", '',
 "_________U__free\n_________U__malloc\n_________U__abort\n_________U__memcpy\n_________U__memset\n_________U_dyld_stub_binding_helper\n_________U__malloc\n_________U___iob\n_________U__abort\n_________U__fprintf\n",
 "_________U___iob\n_________U__abort\n_________U__abort\n_________U__fprintf\n_________U__free\n_________U__malloc\n_________U__malloc\n_________U__memcpy\n_________U__memset\n_________U_dyld_stub_binding_helper\n",
 0],

# Demonstrate that folding changes the ordering of e.g. A, a, and _
# because while they normally (in the C locale) collate like A, _, a,
# when using -f, `a' is compared as if it were `A'.
["21a", '',    "A\na\n_\n", "A\n_\na\n", 0],
["21b", '-f',  "A\na\n_\n", "A\na\n_\n", 0],
["21c", '-f',  "a\nA\n_\n", "A\na\n_\n", 0],
["21d", '-f',  "_\na\nA\n", "A\na\n_\n", 0],
["21e", '-f',  "a\n_\nA\n", "A\na\n_\n", 0],
["21f", '-fs', "A\na\n_\n", "A\na\n_\n", 0],
["21g", '-fu', "a\n_\n", "a\n_\n", 0],

# This test failed until 1.22f.  From Zvi Har'El.
["22a", '-k 2,2fd -k 1,1r', "3 b\n4 B\n", "4 B\n3 b\n", 0],
["22b", '-k 2,2d  -k 1,1r', "3 b\n4 b\n", "4 b\n3 b\n", 0],

["no-file1", 'no-file', {}, '', 2],
# This test failed until 1.22f.  Sort didn't give an error.
# From Will Edgington.
["o-no-file1", '-o no-such-file no-such-file', {}, '', 2],

["create-empty", '-o no/such/file /dev/null', {}, '', 2],

# From Paul Eggert.  This was fixed in textutils-1.22k.
["neg-nls", '-n', "-1\n-9\n", "-9\n-1\n", 0],

# From Paul Eggert.  This was fixed in textutils-1.22m.
# The bug was visible only when using the internationalized sorting code
# (i.e., not when configured with --disable-nls).
["nul-nls", '', "\0b\n\0a\n", "\0a\n\0b\n", 0],

# Paul Eggert wrote:
# I tested the revised `sort' against Solaris `sort', and found a
# discrepancy that turns out to be a longstanding bug in GNU sort.
# POSIX.2 specifies that a newline is part of the input line, and should
# be significant during comparison; but with GNU sort the newline is
# insignificant.  Here is an example of the bug:
#
# 	$ od -c t
# 	0000000  \n  \t  \n
# 	0000003
# 	$ sort t | od -c
# 	0000000  \n  \t  \n
# 	0000003
#
# The correct output of the latter command should be
#
# 	0000000  \t  \n  \n
# 	0000003
#
# because \t comes before \n in the collating sequence, and the trailing
# \n's are part of the input line.
["use-nl", '', "\n\t\n", "\n\t\n", 0],

);

sub test_vector
{
  return @tv;
}

1;
