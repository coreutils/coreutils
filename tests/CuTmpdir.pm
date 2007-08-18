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

our $ME = $0 || "<???>";

my $dir;

sub import {
  my $prefix = $_[1];
  if ($prefix !~ /^\//)
    {
      eval 'use Cwd';
      my $cwd = $@ ? '.' : Cwd::getcwd();
      $prefix = "$cwd/$prefix";
    }
  $dir = File::Temp::tempdir("$prefix.tmp-XXXX", CLEANUP => 1 );
  chdir $dir
    or warn "$ME: failed to chdir to $dir: $!\n";
}

END {
  my $saved_errno = $?;
  # FIXME: use File::Find
  system qw (chmod -R 700), $dir;
  $? = $saved_errno;
}

1;
