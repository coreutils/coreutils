package CuTmpdir;
# create, then chdir into a temporary sub-directory

# Copyright (C) 2007 Free Software Foundation, Inc.

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

use strict;
use warnings;

use File::Temp;
use File::Find;

our $ME = $0 || "<???>";

my $dir;

sub skip_test
{
  warn "$ME: skipping test: unsafe working directory name\n";
  exit 77;
}

sub import {
  my $prefix = $_[1];

  $ME eq '-' && defined $prefix
    and $ME = $prefix;

  if ($prefix !~ /^\//)
    {
      eval 'use Cwd';
      my $cwd = $@ ? '.' : Cwd::getcwd();
      $prefix = "$cwd/$prefix";
    }

  # Untaint for the upcoming mkdir.
  $prefix =~ m!^([-+\@\w./]+)$!
    or skip_test;
  $prefix = $1;

  $dir = File::Temp::tempdir("$prefix.tmp-XXXX", CLEANUP => 1 );
  chdir $dir
    or warn "$ME: failed to chdir to $dir: $!\n";
}

sub wanted
{
  my $name = $_;

  # Skip symlinks and non-directories.
  -l $name || !-d _
    and return;

  chmod 0700, $name;
}

END {
  my $saved_errno = $?;
  if (defined $dir)
    {
      chdir $dir
	or warn "$ME: failed to chdir to $dir: $!\n";
      # Perform the equivalent of find . -type d -print0|xargs -0 chmod -R 700.
      my $options = {untaint => 1, wanted => \&wanted};
      find ($options, '.');
    }
  $? = $saved_errno;
}

1;
