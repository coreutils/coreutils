package Coreutils;
# This is a testing framework.

require 5.003;
use strict;
use vars qw($VERSION @ISA @EXPORT);

use FileHandle;
use File::Compare qw(compare);

@ISA = qw(Exporter);
($VERSION = '$Revision: 1.2 $ ') =~ tr/[0-9].//cd;
@EXPORT = qw (run_tests);

my $debug = $ENV{DEBUG};

my @Types = qw (IN OUT ERR AUX CMP EXIT PRE POST OUT_SUBST ERR_SUBST ENV ENV_DEL);
my %Types = map {$_ => 1} @Types;
my %Zero_one_type = map {$_ => 1}
   qw (OUT ERR EXIT PRE POST OUT_SUBST ERR_SUBST ENV);
my $srcdir = $ENV{srcdir};
my $Global_count = 1;

# When running in a DJGPP environment, make $ENV{SHELL} point to bash.
# Otherwise, a bad shell might be used (e.g. command.com) and many
# tests would fail.
defined $ENV{DJDIR}
  and $ENV{SHELL} = "$ENV{DJDIR}/bin/bash.exe";

# A file spec: a scalar or a reference to a single-keyed hash
# ================
# 'contents'               contents only (file name is derived from test name)
# {filename => 'contents'} filename and contents
# {filename => undef}      filename only -- $(srcdir)/filename must exist
#
# FIXME: If there is more than one input file, then you can't specify `REDIR'.
# PIPE is still ok.
#
# I/O spec: a hash ref with the following properties
# ================
# - one key/value pair
# - the key must be one of these strings: IN, OUT, ERR, AUX, CMP, EXIT
# - the value must be a file spec
# {OUT => 'data'}    put data in a temp file and compare it to stdout from cmd
# {OUT => {'filename'=>undef}} compare contents of existing filename to
#           stdout from cmd
# {OUT => {'filename'=>[$CTOR, $DTOR]}} $CTOR and $DTOR are references to
#           functions, each which is passed the single argument `filename'.
#           $CTOR must create `filename'.
#           DTOR may be omitted in which case `sub{unlink @_[0]}' is used.
#           FIXME: implement this
# {ERR => ...}
#           Same as for OUT, but compare with stderr, not stdout.
# {OUT_SUBST => 's/variable_output/expected_output/'}
#   Transform actual standard output before comparing it against expected output.
#   This is useful e.g. for programs like du that produce output that
#   varies a lot from system.  E.g., an empty file may consume zero file
#   blocks, or more, depending on the OS and on the file system type.
# {ERR_SUBST => 's/variable_output/expected_output/'}
#   Transform actual stderr output before comparing it against expected.
#   This is useful when verifying that we get a meaningful diagnostic.
#   For example, in rm/fail-2eperm, we have to account for three different
#   diagnostics: Operation not permitted, Not owner, and Permission denied.
# {EXIT => N} expect exit status of cmd to be N
# {ENV => 'VAR=val ...'}
#   Prepend 'VAR=val ...' to the command that we execute via `system'.
# {ENV_DEL => 'VAR'}
#   Remove VAR from the environment just before running the corresponding
#   command, and restore any value just afterwards.
#
# There may be many input file specs.  File names from the input specs
# are concatenated in order on the command line.
# There may be at most one of the OUT-, ERR-, and EXIT-keyed specs.
# If the OUT-(or ERR)-keyed hash ref is omitted, then expect no output
#   on stdout (or stderr).
# If the EXIT-keyed one is omitted, then expect the exit status to be zero.

# FIXME: Make sure that no junkfile is also listed as a
# non-junkfile (i.e. with undef for contents)

sub _shell_quote ($)
{
  my ($string) = @_;
  $string =~ s/\'/\'\\\'\'/g;
  return "'$string'";
}

sub _create_file ($$$$)
{
  my ($program_name, $test_name, $file_name, $data) = @_;
  my $file;
  if (defined $file_name)
    {
      $file = $file_name;
    }
  else
    {
      $file = "$test_name.$Global_count";
      ++$Global_count;
    }

  warn "creating file `$file' with contents `$data'\n" if $debug;

  # The test spec gave a string.
  # Write it to a temp file and return tempfile name.
  my $fh = new FileHandle "> $file";
  die "$program_name: $file: $!\n" if ! $fh;
  print $fh $data;
  $fh->close || die "$program_name: $file: $!\n";

  return $file;
}

sub _compare_files ($$$$$)
{
  my ($program_name, $test_name, $in_or_out, $actual, $expected) = @_;

  my $differ = compare ($expected, $actual);
  if ($differ)
    {
      my $info = (defined $in_or_out ? "std$in_or_out " : '');
      warn "$program_name: test $test_name: ${info}mismatch, comparing "
	. "$actual (actual) and $expected (expected)\n";
      # Ignore any failure, discard stderr.
      system "diff -c $actual $expected 2>/dev/null";
    }

  return $differ;
}

sub _process_file_spec ($$$$$)
{
  my ($program_name, $test_name, $file_spec, $type, $junk_files) = @_;

  my ($file_name, $contents);
  if (!ref $file_spec)
    {
      ($file_name, $contents) = (undef, $file_spec);
    }
  elsif (ref $file_spec eq 'HASH')
    {
      my $n = keys %$file_spec;
      die "$program_name: $test_name: $type spec has $n elements --"
	. " expected 1\n"
	  if $n != 1;
      ($file_name, $contents) = each %$file_spec;

      # This happens for the AUX hash in an io_spec like this:
      # {CMP=> ['zy123utsrqponmlkji', {'@AUX@'=> undef}]},
      defined $contents
	or return $file_name;
    }
  else
    {
      die "$program_name: $test_name: invalid RHS in $type-spec\n"
    }

  my $is_junk_file = (! defined $file_name
		      || (($type eq 'IN' || $type eq 'AUX' || $type eq 'CMP')
			  && defined $contents));
  my $file = _create_file ($program_name, $test_name,
			   $file_name, $contents);

  if ($is_junk_file)
    {
      push @$junk_files, $file
    }
  else
    {
      # FIXME: put $srcdir in here somewhere
      warn "$program_name: $test_name: specified file `$file' does"
	. " not exist\n"
	  if ! -f "$srcdir/$file";
    }

  return $file;
}

sub _at_replace ($$)
{
  my ($map, $s) = @_;
  foreach my $eo (qw (AUX OUT ERR))
    {
      my $f = $map->{$eo};
      $f
	and $s =~ /\@$eo\@/
	  and $s =~ s/\@$eo\@/$f/g;
    }
  return $s;
}

# FIXME: cleanup on interrupt
# FIXME: extract `do_1_test' function

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
  my $bad_test_name = 0;
  my %seen;
  my %seen_8dot3;
  my $t;
  foreach $t (@$t_spec)
    {
      my $test_name = $t->[0];
      if ($seen{$test_name})
	{
	  warn "$program_name: $test_name: duplicate test name\n";
	  $bad_test_name = 1;
	}
      $seen{$test_name} = 1;

      if (0)
	{
	  my $t8 = lc substr $test_name, 0, 8;
	  if ($seen_8dot3{$t8})
	    {
	      warn "$program_name: 8.3 test name conflict: "
		. "$test_name, $seen_8dot3{$t8}\n";
	      $bad_test_name = 1;
	    }
	  $seen_8dot3{$t8} = $test_name;
	}

      # The test name may be no longer than 12 bytes,
      # so that we can add a two-byte suffix without exceeding
      # the maximum of 14 imposed on some old file systems.
      if (14 < (length $test_name) + 2)
	{
	  warn "$program_name: $test_name: test name is too long (> 12)\n";
	  $bad_test_name = 1;
	}
    }
  return 1 if $bad_test_name;

  # FIXME check exit status
  system ($prog, '--version') if $verbose;

  my @junk_files;
  my $fail = 0;
  foreach $t (@$t_spec)
    {
      my @post_compare;
      my $test_name = shift @$t;
      my $expect = {};
      my ($pre, $post);

      # FIXME: maybe don't reset this.
      $Global_count = 1;
      my @args;
      my $io_spec;
      my %seen_type;
      my @env_delete;
      my $env_prefix = '';
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

	  # Make sure there's no more than one of OUT, ERR, EXIT, etc.
	  die "$program_name: $test_name: more than one $type spec\n"
	    if $Zero_one_type{$type} and $seen_type{$type}++;

	  if ($type eq 'PRE' or $type eq 'POST')
	    {
	      $expect->{$type} = $val;
	      next;
	    }

	  if ($type eq 'CMP')
	    {
	      my $t = ref $val;
	      $t && $t eq 'ARRAY'
		or die "$program_name: $test_name: invalid CMP spec\n";
	      @$val == 2
		or die "$program_name: $test_name: invalid CMP list;  must have"
		  . " exactly 2 elements\n";
	      my @cmp_files;
	      foreach my $e (@$val)
		{
		  my $r = ref $e;
		  $r && $r ne 'HASH'
		    and die "$program_name: $test_name: invalid element ($r)"
		      . " in CMP list;  only scalars and hash references "
			. "are allowed\n";
		  if ($r && $r eq 'HASH')
		    {
		      my $n = keys %$e;
		      $n == 1
			or die "$program_name: $test_name: CMP spec has $n "
			  . "elements -- expected 1\n";

		      # Replace any `@AUX@' in the key of %$e.
		      my ($ff, $val) = each %$e;
		      my $new_ff = _at_replace $expect, $ff;
		      if ($new_ff ne $ff)
			{
			  $e->{$new_ff} = $val;
			  delete $e->{$ff};
			}
		    }
		  my $cmp_file = _process_file_spec ($program_name, $test_name,
						     $e, $type, \@junk_files);
		  push @cmp_files, $cmp_file;
		}
	      push @post_compare, [@cmp_files];

	      $expect->{$type} = $val;
	      next;
	    }

	  if ($type eq 'EXIT')
	    {
	      die "$program_name: $test_name: invalid EXIT code\n"
		if $val !~ /^\d+$/;
	      # FIXME: make sure $data is numeric
	      $expect->{EXIT} = $val;
	      next;
	    }

	  if ($type =~ /^(OUT|ERR)_SUBST$/)
	    {
	      $expect->{RESULT_SUBST} ||= {};
	      $expect->{RESULT_SUBST}->{$1} = $val;
	      next;
	    }

	  if ($type eq 'ENV')
	    {
	      $env_prefix = "$val ";
	      next;
	    }

	  if ($type eq 'ENV_DEL')
	    {
	      push @env_delete, $val;
	      next;
	    }

	  my $file = _process_file_spec ($program_name, $test_name, $val,
					 $type, \@junk_files);

	  if ($type eq 'IN')
	    {
	      push @args, _shell_quote $file;
	    }
	  elsif ($type eq 'AUX' || $type eq 'OUT' || $type eq 'ERR')
	    {
	      $expect->{$type} = $file;
	    }
	  else
	    {
	      die "$program_name: $test_name: invalid type: $type\n"
	    }
	}

      # Expect an exit status of zero if it's not specified.
      $expect->{EXIT} ||= 0;

      # Allow ERR to be omitted -- in that case, expect no error output.
      foreach my $eo (qw (OUT ERR))
	{
	  if (!exists $expect->{$eo})
	    {
	      $expect->{$eo} = _create_file ($program_name, $test_name,
					     undef, '');
	      push @junk_files, $expect->{$eo};
	    }
	}

      # FIXME: Does it ever make sense to specify a filename *and* contents
      # in OUT or ERR spec?

      # FIXME: this is really suboptimal...
      my @new_args;
      foreach my $a (@args)
	{
	  $a = _at_replace $expect, $a;
	  push @new_args, $a;
	}
      @args = @new_args;

      warn "$test_name...\n" if $verbose;
      &{$expect->{PRE}} if $expect->{PRE};
      my %actual;
      $actual{OUT} = "$test_name.O";
      $actual{ERR} = "$test_name.E";
      push @junk_files, $actual{OUT}, $actual{ERR};
      my @cmd = ($prog, @args, "> $actual{OUT}", "2> $actual{ERR}");
      my $cmd_str = $env_prefix . join (' ', @cmd);

      # Delete from the environment any symbols specified by syntax
      # like this: {ENV_DEL => 'TZ'}.
      my %pushed_env;
      foreach my $env_sym (@env_delete)
	{
	  my $val = delete $ENV{$env_sym};
	  defined $val
	    and $pushed_env{$env_sym} = $val;
	}

      warn "Running command: `$cmd_str'\n" if $debug;
      my $rc = 0xffff & system $cmd_str;

      # Restore any environment setting we changed via a deletion.
      foreach my $env_sym (keys %pushed_env)
	{
	  $ENV{$env_sym} = $pushed_env{$env_sym};
	}

      if ($rc == 0xff00)
	{
	  warn "$program_name: test $test_name failed: command failed:\n"
	    . "  `$cmd_str': $!\n";
	  $fail = 1;
	  goto cleanup;
	}
      $rc >>= 8 if $rc > 0x80;
      if ($expect->{EXIT} != $rc)
	{
	  warn "$program_name: test $test_name failed: exit status mismatch:"
	    . "  expected $expect->{EXIT}, got $rc\n";
	  $fail = 1;
	  goto cleanup;
	}

      foreach my $eo (qw (OUT ERR))
	{
	  my $subst_expr = $expect->{RESULT_SUBST}->{$eo};
	  if (defined $subst_expr)
	    {
	      my $out = $actual{$eo};
	      my $orig = "$out.orig";

	      # Move $out aside (to $orig), then then recreate $out
	      # by transforming each line of $orig via $subst_expr.
	      rename $out, $orig
		or (warn "$program_name: cannot rename $out to $orig: $!\n"),
		  $fail = 1, next;
	      open IN, $orig
		or (warn "$program_name: cannot open $orig for reading: $!\n"),
		  $fail = 1, (unlink $orig), next;
	      unlink $orig
		or (warn "$program_name: cannot unlink $orig: $!\n"),
		  $fail = 1;
	      open OUT, ">$out"
		or (warn "$program_name: cannot open $out for writing: $!\n"),
		  $fail = 1, next;
	      while (defined (my $line = <IN>))
		{
		  eval "\$_ = \$line; $subst_expr; \$line = \$_";
		  print OUT $line;
		}
	      close IN;
	      close OUT
		or (warn "$program_name: failed to write $out: $!\n"),
		  $fail = 1, next;
	    }

	  my $eo_lower = lc $eo;
	  _compare_files ($program_name, $test_name, $eo_lower,
			  $actual{$eo}, $expect->{$eo})
	    and $fail = 1;
	}

      foreach my $pair (@post_compare)
	{
	  my ($expected, $actual) = @$pair;
	  _compare_files $program_name, $test_name, undef, $actual, $expected
	    and $fail = 1;
	}

    cleanup:
      &{$expect->{POST}} if $expect->{POST};

    }

  # FIXME: maybe unlink files inside the big foreach loop?
  unlink @junk_files if ! $save_temps;

  return $fail;
}

## package return
1;
