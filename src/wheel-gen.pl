#! /sw/tools/bin/perl -w
# FIXME: one line description
# Generated automatically from @FROM@.
# remember to add to perl_exe in Makefile.am

use strict;
use Getopt::Long;

(my $VERSION = '$Revision: 1.1 $ ') =~ tr/[0-9].//cd;
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
  my $is_prime =
    {
     2 => 1,
     3 => 1,
     5 => 1,
     7 => 1,
     11 => 1,
    };
  $w = 1;

  sub is_prime ($)
  {
    my ($n0) = @_;
    # FIXME
    die;
    use integer;
    my $n = $n0;
    my $d = 2;
    while (1)
      {
	my $q = $n / $d;
	($n == $q * $d)
	  and return 0;
	$d += $w;
	$d < $q
	  and last;
	$w = 2;
      }
    return 1;
  }

  my $prod = 2 * 3 * 5;
  my $last_prime = 2;
  for ($i = 3; $i < $prod; $i++)
    {
      if (is_prime $i)
	{
	  print $i - $last_prime, "\n";
	  $last_prime = $i;
	}
    }

  exit 0;
}
