package T;

require 5.004;
use strict;
use vars qw($VERSION @ISA @EXPORT);

use FileHandle;
use File::Compare qw(compare);

@ISA = qw(Exporter);
$VERSION = '0.5';
@EXPORT = qw (run_tests);

my @Types = qw (IN OUT ERR EXIT);
my %Types = map {$_ => 1} @Types;

my $count = 1;

sub _create_file ($$$)
{
  my ($program_name, $test_name, $data) = @_;
  my $file = "$test_name-$$.$count";
  ++$count;

  # The test spec gave a string.
  # Write it to a temp file and return tempfile name.
  my $fh = new FileHandle "> $file";
  die "$program_name: $file: $!\n" if ! $fh;
  print $fh $data;
  $fh->close || die "$program_name: $file: $!\n";

  return $file;
}

# FIXME: having to include $program_name here is an expedient kludge.
# Library code doesn't `die'.
sub run_tests ($$$$$)
{
  my ($program_name, $prog, $t_spec, $save_temps, $verbose) = @_;

  # Warn about empty t_spec.
  # FIXME

  # Remove all temp files upon interrupt.
  # FIXME

  # Verify that test names are distinct.
  my $found_duplicate = 0;
  my %seen;
  my $t;
  foreach $t (@$t_spec)
    {
      my $test_name = $t->[0];
      if ($seen{$test_name})
	{
	  warn "$program_name: $test_name: duplicate test name\n";
	  $found_duplicate = 1;
	}
      $seen{$test_name} = 1;
    }
  return 1 if $found_duplicate;

  # FIXME check exit status
  system ($prog, '--version');

  my @junk_files;
  my $fail = 0;
  foreach $t (@$t_spec)
    {
      my $test_name = shift @$t;

      my $expout_file;
      my $experr_file;
      my $exit_status;

      my @args;
      my $io_spec;
      foreach $io_spec (@$t)
	{
	  if (!ref $io_spec)
	    {
	      push @args, $io_spec;
	      next;
	    }

	  die "$program_name: $test_name: invalid test spec\n"
	    if ref $io_spec ne 'HASH';

	  my $n = keys %$io_spec;
	  die "$program_name: $test_name: spec has $n elements --"
	    . " expected 1\n"
	      if $n != 1;
	  my ($type, $val) = each %$io_spec;
	  die "$program_name: $test_name: invalid key `$type' in test spec\n"
	      if ! $Types{$type};

	  if ($type eq 'EXIT')
	    {
	      # FIXME: make sure there's only one of these
	      # FIXME: make sure $data is numeric
	      $exit_status = $val;
	      next;
	    }

	  my $file_spec = $val;
	  my ($filename, $contents) = each %$file_spec;

	  if ($type =~ /_FILE$/ || $type =~ /_DATA$/)
	    {
	      my $file;
	      if ($type =~ /_FILE$/)
		{
		  $file = $data;
		  warn "$program_name: $test_name: specified file `$file' does"
		    . " not exist\n"
		      if ! -f $file;
		}
	      else
		{
		  $file = _create_file ($program_name, $test_name, $data);
		  push @junk_files, $file;
		}

	      if ($type =~ /IN_/)
		{
		  push (@args, $file)
		}
	      elsif ($type =~ /OUT_/)
		{
		  die "$program_name: $test_name: duplicate OUT_* spec\n"
		    if defined $expout_file;
		  $expout_file = $file;
		}
	      elsif ($type =~ /ERR_/)
		{
		  die "$program_name: $test_name: duplicate ERR_* spec\n"
		    if defined $experr_file;
		  $experr_file = $file;
		}
	    }
	  elsif ($type eq 'EXIT_STATUS')
	    {
	      $exit_status = $data;
	    }
	  else
	    {
	      die "$program_name: $test_name: internal error\n";
	    }
	}

      $exit_status = 0 if !defined $exit_status;

      # Allow ERR_* to be omitted -- in that case, expect no error output.
      if (!defined $experr_file)
	{
	  $experr_file = _create_file ($program_name, $test_name, '');
	  push @junk_files, $experr_file;
	}

      # FIXME: Require at least one of OUT_DATA, OUT_FILE.

      warn "$test_name...\n";
      my $t_out = "$test_name-out";
      my $t_err = "$test_name-err";
      push (@junk_files, $t_out, $t_err);
      my @cmd = ($prog, @args, "> $t_out", "2> $t_err");
      my $cmd_str = join (' ', @cmd);
      warn "Running command: `$cmd_str'\n" if $verbose;
      my $rc = 0xffff & system $cmd_str;
      if ($rc == 0xff00)
	{
	  warn "$program_name: test $test_name failed: command failed:\n"
	    . "  `$cmd_str': $!\n";
	  $fail = 1;
	  next;
	}
      $rc >>= 8 if $rc > 0x80;
      if ($exit_status != $rc)
	{
	  warn "$program_name: test $test_name failed: exit status mismatch:"
	    . "  expected $exit_status, got $rc\n";
	  $fail = 1;
	  next;
	}

      if (compare ($expout_file, $t_out))
	{
	  warn "$program_name: stdout mismatch, comparing"
	    . " $expout_file and $t_out\n";
	  $fail = 1;
	  next;
	}
      if (compare ($experr_file, $t_err))
	{
	  warn "$program_name: stderr mismatch, comparing"
	    . " $experr_file and $t_err\n";
	  $fail = 1;
	  next;
	}
    }

  unlink @junk_files if ! $save_temps;

  return $fail;
}

## package return
1;
