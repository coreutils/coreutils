# input flags  1 or 2 strings      expected output   expected return code
#
("1", "abcd",   '',    'abcd','[]*]',   "]]]]",              0);
("2", "abc",    '',    'abc','[%*]xyz',  "xyz",              0);
("3", "abc",    '',    '','[.*]',        "abc",              0);
# Test --truncate-set1 behavior when string1 is longer than string2
("4", "abcde", '-t',   'abcd','xy',     "xycde",            0);
# Test bsd behavior (the default) when string1 is longer than string2
("5", "abcde", '',      'abcd','xy',     "xyyye",            0);
# Do it the posix way
("6", "abcde", '',      'abcd','x[y*]',  "xyyye",            0);
#
("7", "abcdefghijklmnop", '-s', 'a-p','%[.*]$', "%.$",       0);
("8", "abcdefghijklmnop", '-s', 'a-p','[.*]$', ".$",         0);
("9", "abcdefghijklmnop", '-s', 'a-p','%[.*]', "%.",         0);
("a", "aabbcc", '-s', '[a-z]','',        "abc",              0);
("b", "aabbcc", '-s', '[a-c]','',        "abc",              0);
("c", "aabbcc", '-s', '[a-b]','',        "abcc",             0);
("d", "aabbcc", '-s', '[b-c]','',        "aabc",             0);
("e", "\0\0a\1\1b\2\2\2c\3\3\3d\4\4\4\4e\5\5", \
	   '-s',    '[\0-\5]','', "\0a\1b\2c\3d\4e\5",  0);
# tests of delete
("f", "[[[[[[[]]]]]]]]", '-d', '[=[=]','', "]]]]]]]]", 0);
("g", "[[[[[[[]]]]]]]]", '-d', '[=]=]','', "[[[[[[[", 0);
("h", "0123456789acbdefABCDEF", '-d', '[:xdigit:]','', "", 0);
("i", "w0x1y2z3456789acbdefABCDEFz", '-d', '[:xdigit:]','', "wxyzz", 0);
("j", "0123456789", '-d', '[:digit:]','', "", 0);
("k", "a0b1c2d3e4f5g6h7i8j9k", '-d', '[:digit:]','', "abcdefghijk", 0);
("l", "abcdefghijklmnopqrstuvwxyz", '-d', '[:lower:]','', "", 0);
("m", "ABCDEFGHIJKLMNOPQRSTUVWXYZ", '-d', '[:upper:]','', "", 0);
("n", "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ", \
    '-d', '[:lower:][:upper:]','', "", 0);
("o", "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ", \
    '-d', '[:alpha:]','', "", 0);
("p", "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789", \
    '-d', '[:alnum:]','', "", 0);
("q", ".abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.", \
    '-d', '[:alnum:]','', "..", 0);
("r", ".abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.", \
    '-ds', '[:alnum:]','.', ".", 0);
# The classic example, with string2 BSD-style
("s", "The big black fox jumped over the fence.", \
    '-cs', '[:alnum:]','\n', \
    "The\nbig\nblack\nfox\njumped\nover\nthe\nfence\n", 0);
# The classic example, POSIX-style
("t", "The big black fox jumped over the fence.", \
    '-cs', '[:alnum:]','[\n*]', \
    "The\nbig\nblack\nfox\njumped\nover\nthe\nfence\n", 0);
("u", "aabbaa", '-ds', 'b','a',          "a",             0);
("v", "ZZ0123456789acbdefABCDEFZZ", \
	   '-ds', '[:xdigit:]','Z', "Z", 0);
# Try some data with 8th bit set in case something is mistakenly sign-extended. 
("w", "\300\301\377\345\345\350\345", \
	   '-ds', '\350','\345', "\300\301\377\345", 0);
("x", "abcdefghijklmnop", '-s', 'abcdefghijklmn','[:*016]', ":op", 0);
("y", "abc \$code", '-d', 'a-z','', " \$", 0);
("z", "a.b.c \$\$\$\$code\\", '-ds', 'a-z','$.', ". \$\\", 0);
# Make sure that a-a is accepted, even though POSIX 1001.2 says it is illegal.
("A", "abc",    '',  'a-a','z',         "zbc",               0);
#
("B", "",       '',  'a',"''",          "",                  1);
("C", "abcxyzABCXYZ", '', '[:lower:]', '[:upper:]', "ABCXYZABCXYZ", 0);
("D", "abcxyzABCXYZ", '', '[:upper:]', '[:lower:]', "abcxyzabcxyz", 0);
