# TODO: make sure all test_names are unique!!!!
# test name
#     flags       file-1 file-2    expected output   expected return code
#
('1a', '-a1',      "a 1\n", "b\n",   "a 1\n",          0);
('1b', '-a2',      "a 1\n", "b\n",   "b\n",            0); # Got "\n"
('1c', '-a1 -a2',  "a 1\n", "b\n",   "a 1\nb\n",       0); # Got "a 1\n\n"

('2a', '-a1 -e .',  "a\nb\nc\n", "a x y\nb\nc\n", "a x y\nb\nc\n", 0);
('2b', '-a1 -e . -o 2.1,2.2,2.3',  "a\nb\nc\n", "a x y\nb\nc\n", "a x y\nb . .\nc . .\n", 0);
('2c', '-a1 -e . -o 2.1,2.2,2.3',  "a\nb\nc\nd\n", "a x y\nb\nc\n", "a x y\nb . .\nc . .\nd\n", 0);
