# TODO: make sure all test_names are unique!!!!
# test name
#     flags       file-1 file-2    expected output   expected return code
#
('1a', '-a1',      "a 1\n", "b\n",   "a 1\n",          0);
('1b', '-a2',      "a 1\n", "b\n",   "b\n",            0); # Got "\n"
('1c', '-a1 -a2',  "a 1\n", "b\n",   "a 1\nb\n",       0); # Got "a 1\n\n"
('1d', '-a1',      "a 1\nb\n", "b\n",   "a 1\nb\n",    0);
('1e', '-a2',      "a 1\nb\n", "b\n",   "b\n",         0);
('1f', '-a2',      "b\n", "a\nb\n", "a\nb\n",          0);

('2a', '-a1 -e .',  "a\nb\nc\n", "a x y\nb\nc\n", "a x y\nb\nc\n", 0);
('2b', '-a1 -e . -o 2.1,2.2,2.3',  "a\nb\nc\n", "a x y\nb\nc\n", "a x y\nb . .\nc . .\n", 0);
('2c', '-a1 -e . -o 2.1,2.2,2.3',  "a\nb\nc\nd\n", "a x y\nb\nc\n", "a x y\nb . .\nc . .\nd\n", 0);

('3a', '-t:', "a:1\nb:1\n", "a:2:\nb:2:\n", "a:1:2:\nb:1:2:\n", 0);

# Just like -a1 and -a2 when there are no pairable lines
('4a', '-v 1', "a 1\n", "b\n",   "a 1\n",          0);
('4b', '-v 2', "a 1\n", "b\n",   "b\n",            0);

('4c', '-v 1', "a 1\nb\n", "b\n",        "a 1\n",       0);
('4d', '-v 2', "a 1\nb\n", "b\n",        "",            0);
('4e', '-v 2', "b\n",      "a 1\nb\n",   "a 1\n",       0);
