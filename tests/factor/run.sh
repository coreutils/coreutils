#!/bin/sh
# Test the factor rewrite.
# Expect to be invoked via a file whose basename matches
# /^(\d+)\-(\d+)\-([\da-f]{40})\.sh$/
# The test is to run this command
# seq $1 $2 | factor | shasum -c --status <(echo $3  -)
# I.e., to ensure that the factorizations of integers $1..$2
# match what we expect.

# Copyright (C) 2012 Free Software Foundation, Inc.

. "${srcdir=.}/tests/init.sh"; path_prepend_ ./src

# Don't run these tests by default.
very_expensive_

print_ver_ factor seq

# Remove the ".sh" suffix:
t=${ME_%.sh}

# Make IFS include "-", so that a simple "set" will separate the args:
IFS=-$IFS
set $t
echo "$3  -" > exp

f=1
seq $1 $2 | factor | shasum -c --status exp && f=0

Exit $f
