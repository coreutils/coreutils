# -*-perl-*-
#test   options   input   expected-output   expected-return-code
#
("1a", '', "A\nB\nC\n", "A\nB\nC\n", 0);
#
("2a", '-c', "A\nB\nC\n", '', 0);
("2b", '-c', "A\nC\nB\n", '', 1);
# This should fail because there are duplicate keys
("2c", '-cu', "A\nA\n", '', 1);
("2d", '-cu', "A\nB\n", '', 0);
("2e", '-cu', "A\nB\nB\n", '', 1);
("2f", '-cu', "B\nA\nB\n", '', 1);
#
("3a", '-k1', "B\nA\n", "A\nB\n",  0);
("3b", '-k1,1', "B\nA\n", "A\nB\n",  0);
("3c", '-k1 -k2', "A b\nA a\n", "A a\nA b\n",  0);
# FIXME: fail with a diagnostic when -k specifies field == 0
("3d", '-k0', "", "",  1);
# FIXME: fail with a diagnostic when -k specifies character == 0
("3e", '-k1.0', "", "",  1);
#
("4a", '-nc', "2\n11\n", "",  0);
("4b", '-n', "11\n2\n", "2\n11\n", 0);
("4c", '-k1n', "11\n2\n", "2\n11\n", 0);
("4d", '-k1', "11\n2\n", "11\n2\n", 0);
("4e", '-k2', "ignored B\nz-ig A\n", "z-ig A\nignored B\n", 0);
#
("5a", '-k1,2', "A B\nA A\n", "A A\nA B\n", 0);
("5b", '-k1,2', "A B A\nA A Z\n", "A A Z\nA B A\n", 0);
("5c", '-k1 -k2', "A B A\nA A Z\n", "A A Z\nA B A\n", 0);
("5d", '-k2,2', "A B A\nA A Z\n", "A A Z\nA B A\n", 0);
("5e", '-k2,2', "A B Z\nA A A\n", "A A A\nA B Z\n", 0);
("5f", '-k2,2', "A B A\nA A Z\n", "A A Z\nA B A\n", 0);
#
("6a", '-k 1,2', "A B\nA A\n", "A A\nA B\n", 0);
("6b", '-k 1,2', "A B A\nA A Z\n", "A A Z\nA B A\n", 0);
("6c", '-k 1 -k 2', "A B A\nA A Z\n", "A A Z\nA B A\n", 0);
("6d", '-k 2,2', "A B A\nA A Z\n", "A A Z\nA B A\n", 0);
("6e", '-k 2,2', "A B Z\nA A A\n", "A A A\nA B Z\n", 0);
("6f", '-k 2,2', "A B A\nA A Z\n", "A A Z\nA B A\n", 0);
#
("7a", '-k 2,3', "9 a b\n7 a a\n", "7 a a\n9 a b\n", 0);
("7b", '-k 2,3', "a a b\nz a a\n", "z a a\na a b\n", 0);
("7c", '-k 2,3', "y k b\nz k a\n", "z k a\ny k b\n", 0);
("7d", '+1 -3', "y k b\nz k a\n", "z k a\ny k b\n", 0);
#
# FIXME: report an error for `.' but missing char spec
("8a", '-k 2.,3', "", "", 1);
# FIXME: report an error for `,' but missing POS2
("8b", '-k 2,', "", "", 1);
