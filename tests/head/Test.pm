package Test;
require 5.002;
use strict;

# Tell head to accept old-style options like `-1'.
$Test::env_default = ['_POSIX2_VERSION=199209'];

# This should match the definition in head.c.
my $READ_BUFSIZE = 4096;

my @tv = (
# test name, options, input, expected output, expected return code
#
['idem-0', '', "", "", 0],
['idem-1', '', "a", "a", 0],
['idem-2', '', "\n", "\n", 0],
['idem-3', '', "a\n", "a\n", 0],

['basic-0-10', '',
 "1\n2\n3\n4\n5\n6\n7\n8\n9\n0\n",
 "1\n2\n3\n4\n5\n6\n7\n8\n9\n0\n", 0],

['basic-0-09', '',
 "1\n2\n3\n4\n5\n6\n7\n8\n9\n",
 "1\n2\n3\n4\n5\n6\n7\n8\n9\n", 0],

['basic-0-11', '',
 "1\n2\n3\n4\n5\n6\n7\n8\n9\n0\nb\n",
 "1\n2\n3\n4\n5\n6\n7\n8\n9\n0\n", 0],

['obs-0', '-1', "1\n2\n", "1\n", 0],
['obs-1', '-1c', "", "", 0],
['obs-2', '-1c', "12", "1", 0],
['obs-3', '-14c', "1234567890abcdefg", "1234567890abcd", 0],
['obs-4', '-2b', [\'in'], [\'in-1024'], 0],
['obs-5', '-1k', [\'in'], [\'in-1024'], 0],

# This test fails for textutils-1.22, because head let 4096m overflow to 0
# and did not fail.  Now head fails with a diagnostic.

# Disable this test because it fails on systems with 64-bit longs.
# ['fail-0', '-n 4096m', "a\n", "", 1],

# In spite of its name, this test passes -- just to contrast with the above.
['fail-1', '-n 2048m', "a\n", "a\n", 0],

# Make sure we don't break like AIX 4.3.1 on files with \0 in them.
['null-1', '', "a\0a\n", "a\0a\n", 0],

# Make sure counts are interpreted as decimal.
# Before 2.0f, these would have been interpreted as octal
['no-oct-1', '-08',  "\n"x12, "\n"x8, 0],
['no-oct-2', '-010', "\n"x12, "\n"x10, 0],
['no-oct-3', '-n 08', "\n"x12, "\n"x8, 0],
['no-oct-4', '-c 08', "\n"x12, "\n"x8, 0],

# Elide the exact size of the file.
['elide-1', "--bytes=-2", "a\n", '', 0],
# Elide more than the size of the file.
['elide-2', "--bytes=-2", "a", '', 0],
# Leave just one byte.
['elide-3', "--bytes=-2", "abc", 'a', 0],
# Make it so the elided bytes straddle the end of the first $READ_BUFSIZE block.
['elide-4', "--bytes=-2",
 'a' x ($READ_BUFSIZE-3) . "\nbcd",
 'a' x ($READ_BUFSIZE-3) . "\nb", 0],
# Make it so the elided bytes straddle the end of the 2nd $READ_BUFSIZE block.
['elide-5', "--bytes=-2",
 'a' x (2 * $READ_BUFSIZE - 2) . 'bcd',
 'a' x (2 * $READ_BUFSIZE - 2) . 'b', 0],
);

# Brute force: use all combinations of file sizes [0..20] and
# number of bytes to elide [0..20].
my $s = "abcdefghijklmnopqrst";
for my $file_size (0..20)
  {
    for my $n_elide (0..20)
      {
	my $input = substr $s, 0, $file_size;
	my $out_len = $n_elide < $file_size ? $file_size - $n_elide : 0;
	my $output = substr $input, 0, $out_len;
	push @tv, ["elide-$file_size-$n_elide", "--bytes=-$n_elide",
		   $input, $output, 0];

      }
  }

sub test_vector
{
  my @derived_tests;
  foreach my $t (@tv)
    {
      my ($test_name, $flags, $in, $exp, $ret) = @$t;

      # Derive equivalent, posix-style tests from the obsolescent ones.
      next if $test_name !~ /^obs-/;

      $test_name =~ s/^obs-/posix-/;
      if ($flags =~ /-(\d+)$/)
	{
	  $flags = "-n $1";
	}
      elsif ($flags =~ /-(\d+)([cbk])$/)
	{
	  my $suffix = $2;
	  $suffix = '' if $suffix eq 'c';
	  $flags = "-c $1$suffix";
	}
      else
	{
	  $flags = "-l $`";
	}
      push (@derived_tests, [$test_name, $flags, $in, $exp, $ret]);
    }

  foreach my $t (@tv, @derived_tests)
    {
      my ($test_name) = @$t;
      $Test::input_via{$test_name} = {REDIR => 0, FILE => 0, PIPE => 0}
    }

  return (@tv, @derived_tests);
}

1;
