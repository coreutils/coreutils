#test   options   input   expected-output   return-code
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
