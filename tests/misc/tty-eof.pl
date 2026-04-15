#!/usr/bin/perl
# Test whether programs exit upon a single EOF from a tty.
# Ensure that e.g., cat exits upon a single configured EOF from a tty.
# Do the same for all programs that can read stdin,
# require no arguments and that write to standard output.

# Copyright (C) 2003-2026 Free Software Foundation, Inc.

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

use strict;
(my $ME = $0) =~ s|.*/||;

eval { require POSIX; };
$@
  and CuSkip::skip "$ME: this script requires Perl's POSIX module\n";

use POSIX qw(:termios_h);

# Some older versions of Expect.pm (e.g. 1.07) lack the log_user method,
# so check for that, too.
eval { require Expect; Expect->require_version('1.11') };
$@
  and CuSkip::skip "$ME: this script requires Perl's Expect package >=1.11\n";

# Try to explicitly set eof character in case it's not the usual Ctrl-d
sub
set_tty_eof_char ($$)
{
  my ($tty, $eof_char) = @_;

  return eval
    {
      my $termios = POSIX::Termios->new;
      my $fd = fileno ($tty);
      defined $fd
        or die "no file descriptor for tty";
      $termios->getattr ($fd)
        or die "tcgetattr failed: $!";
      $termios->setcc (VEOF, ord $eof_char);
      $termios->setattr ($fd, TCSANOW)
        or die "tcsetattr failed: $!";
      1;
    };
}

sub
normalize_tty_output ($)
{
  my ($output) = @_;

  $output =~ s/\r\n/\n/g;
  $output =~ s/\r/\n/g;
  return $output;
}

{
  my $fail = 0;
  my $eof_char = "\cD";
  my $eof_name = '^D';
  my @stdin_reading_commands = qw(
    b2sum
    base32
    base64
    cat
    cksum
    expand
    fmt
    fold
    head
    md5sum
    nl
    od
    paste
    pr
    ptx
    sha1sum
    sha224sum
    sha256sum
    sha384sum
    sha512sum
    shuf
    sort
    sum
    tac
    tail
    tee
    tsort
    unexpand
    uniq
    wc
  );
  my @commands = (@stdin_reading_commands,
   'basenc --z85',
   'cut -d " " -f2',
   'cut -b1-3',
   'dd status=none',
   'numfmt --invalid=ignore'
  );
  my $stderr = 'tty-eof.err';
  foreach my $with_input (1, 0)
    {
      foreach my $cmd (@commands)
        {
          my $exp = new Expect;
          $exp->log_user(0);
          my $cmd_name = (split(' ', $cmd))[0];
          my $mode = $with_input ? 'with input' : 'without input';
          $ENV{built_programs} =~ /\b$cmd_name\b/ || next;
          $exp->spawn("$cmd 2> $stderr")
            or (warn "$ME: cannot run '$cmd' ($mode): $!\n"),
               $fail=1, next;
          set_tty_eof_char ($exp->slave, $eof_char);

          my $input = "a b\n";
          if ($with_input)
            {
              $exp->send($input);
            }
          $exp->send($eof_char);

          $exp->expect(10, 'eof');
          if (! defined $exp->exitstatus)
            {
              warn "$ME: $cmd didn't exit after $eof_name from standard input"
                   . " ($mode)\n";
              $fail = 1;
            }
          else
            {
              my $output = normalize_tty_output ($exp->before ());

              if ($with_input)
                {
                  $output =~ s/\Q$input\E//
                   or (warn "$ME: $cmd ($mode) didn't echo expected input\n"),
                       $fail = 1;
                  $output =~ /\S/
                   or (warn "$ME: $cmd ($mode) didn't write expected output\n"),
                       $fail = 1;
                }

              my $s = $exp->exitstatus;
              $s == 0
                or (warn "$ME: $cmd exited with status $s (expected 0)"
                         . " ($mode)\n"),
                   $fail = 1;
            }

          $exp->hard_close();

          if (-s $stderr)
            {
              warn "$ME: $cmd wrote to stderr ($mode):\n";
              system "cat $stderr";
              $fail = 1;
            }
        }
    }
  continue
    {
      unlink $stderr
        or warn "$ME: failed to remove stderr file $stderr: $!\n";
    }

  exit $fail
}
