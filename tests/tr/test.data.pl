# input flags  1 or 2 strings      expected output   expected return code
#
("abcd",   '',    'abcd','[]*]',   "]]]]",              0);
("abc",    '',    'abc','[%*]xyz',  "xyz",              0);
("abc",    '',    '','[.*]',        "abc",              0);
# Test --truncate-set1 behavior when string1 is longer than string2
("abcde", '-t',   'abcd','xy',     "xycde",            0);
# Test bsd behavior (the default) when string1 is longer than string2
("abcde", '',      'abcd','xy',     "xyyye",            0);
# Do it the posix way
("abcde", '',      'abcd','x[y*]',  "xyyye",            0);
#
("abcdefghijklmnop", '-s', 'a-p','%[.*]$', "%.$",       0);
("abcdefghijklmnop", '-s', 'a-p','[.*]$', ".$",         0);
("abcdefghijklmnop", '-s', 'a-p','%[.*]', "%.",         0);
("aabbcc", '-s', '[a-z]','',        "abc",              0);
("aabbcc", '-s', '[a-c]','',        "abc",              0);
("aabbcc", '-s', '[a-b]','',        "abcc",             0);
("aabbcc", '-s', '[b-c]','',        "aabc",             0);
("\0\0a\1\1b\2\2\2c\3\3\3d\4\4\4\4e\5\5", \
	   '-s',    '[\0-\5]','', "\0a\1b\2c\3d\4e\5",  0);
# tests of delete
("[[[[[[[]]]]]]]]", '-d', '[=[=]','', "]]]]]]]]", 0);
("[[[[[[[]]]]]]]]", '-d', '[=]=]','', "[[[[[[[", 0);
("0123456789acbdefABCDEF", '-d', '[:xdigit:]','', "", 0);
("w0x1y2z3456789acbdefABCDEFz", '-d', '[:xdigit:]','', "wxyzz", 0);
("0123456789", '-d', '[:digit:]','', "", 0);
("a0b1c2d3e4f5g6h7i8j9k", '-d', '[:digit:]','', "abcdefghijk", 0);
("abcdefghijklmnopqrstuvwxyz", '-d', '[:lower:]','', "", 0);
("ABCDEFGHIJKLMNOPQRSTUVWXYZ", '-d', '[:upper:]','', "", 0);
("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ", \
    '-d', '[:lower:][:upper:]','', "", 0);
("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ", \
    '-d', '[:alpha:]','', "", 0);
("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789", \
    '-d', '[:alnum:]','', "", 0);
(".abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.", \
    '-d', '[:alnum:]','', "..", 0);
(".abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.", \
    '-ds', '[:alnum:]','.', ".", 0);
# The classic example, with string2 BSD-style
("The big black fox jumped over the fence.", \
    '-cs', '[:alnum:]','\n', \
    "The\nbig\nblack\nfox\njumped\nover\nthe\nfence\n", 0);
# The classic example, POSIX-style
("The big black fox jumped over the fence.", \
    '-cs', '[:alnum:]','[\n*]', \
    "The\nbig\nblack\nfox\njumped\nover\nthe\nfence\n", 0);
("aabbaa", '-ds', 'b','a',          "a",             0);
("ZZ0123456789acbdefABCDEFZZ", \
	   '-ds', '[:xdigit:]','Z', "Z", 0);
# Try some data with 8th bit set in case something is mistakenly sign-extended. 
("\300\301\377\345\345\350\345", \
	   '-ds', '\350','\345', "\300\301\377\345", 0);
("abcdefghijklmnop", '-s', 'abcdefghijklmn','[:*016]', ":op", 0);
("abc \$code", '-d', 'a-z','', " \$", 0);
("a.b.c \$\$\$\$code\\", '-ds', 'a-z','$.', ". \$\\", 0);
# Make sure that a-a is accepted, even though POSIX 1001.2 says it is illegal.
("abc",    '',  'a-a','z',         "zbc",               0);
#
("",       '',  'a',"''",          "",                  1);
