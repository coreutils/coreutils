#! /sw/tools/bin/perl -w
# FIXME: one line description
# Generated automatically from @FROM@.
# remember to add to perl_exe in Makefile.am

use strict;
use Getopt::Long;

(my $VERSION = '$Revision: 1.4 $ ') =~ tr/[0-9].//cd;
(my $program_name = $0) =~ s|.*/||;

my $debug = 0;
my $verbose = 0;

sub usage ($)
{
  my ($exit_code) = @_;
  my $STREAM = ($exit_code == 0 ? *STDOUT : *STDERR);
  if ($exit_code != 0)
    {
      print $STREAM "Try `$program_name --help' for more information.\n";
    }
  else
    {
      # FIXME: add new option descriptions here
      print $STREAM <<EOF;
Usage: $program_name [OPTIONS] FIXME: FILE ?

OPTIONS:

   --help             display this help and exit
   --version          output version information and exit
   --debug            output debugging information
   --verbose	      generate verbose output

EOF
    }
  exit $exit_code;
}

sub END
{
  use POSIX qw (_exit);
  # This is required if the code might send any output to stdout
  # E.g., even --version or --help.  So it's best to do it unconditionally.
  close STDOUT
    or (warn "$program_name: closing standard output: $!\n"), _exit (1);
}

{
  GetOptions
    (
     # FIXME: add new options here
     debug => \$debug,
     verbose => \$verbose,
     help => sub { usage 0 },
     version => sub { print "$program_name version $VERSION\n"; exit },
    ) or usage 1;

  # FIXME: provide default here if necessary
  # unshift (@ARGV, ".") if !@ARGV;
#  print "$#ARGV ; $ARGV[0], $ARGV[1]\n" if $debug;
#  foreach my $arg (@ARGV)
#    {
#      die "$program_name: FIXME - process $arg here"
#    }

  my $wheel_size = $ARGV[0];

  my @primes = (2);
  my $product = $primes[0];
  my $n_primes = 1;
  for (my $i = 3; ; $i += 2)
    {
      if (is_prime $i)
	{
	  push @primes, $i;
	  $product *= $i;
	  ++$n_primes == $wheel_size
	    and last;
	}
    }

  my $prev = 2;
  for (my $i = 3; ; $i += 2)
  while (1)
    {
      my $rel_prime = 1;
      foreach my $d (@primes)
	{
	  FIXME
	}
      if ($rel_prime)
	{
	  print $i, ' ', $i - $prev, "\n";
	  $prev = $i;

	  $product < $i
	    and last;
	}
    }

  exit 0;
}
