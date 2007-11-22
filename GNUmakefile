# Having a separate GNUmakefile lets me `include' the dynamically
# generated rules created via Makefile.maint as well as Makefile.maint itself.
# This makefile is used only if you run GNU Make.
# It is necessary if you want to build targets usually of interest
# only to the maintainer.

# Copyright (C) 2001, 2003, 2006-2007 Free Software Foundation, Inc.
#
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

# Systems where /bin/sh is not the default shell need this.  The $(shell)
# command below won't work with e.g. stock DOS/Windows shells.
ifeq ($(wildcard /bin/s[h]),/bin/sh)
SHELL = /bin/sh
else
# will be used only with the next shell-test line, then overwritten
# by a configured-in value
SHELL = sh
endif

have-Makefile := $(shell test -f Makefile && echo yes)

# If the user runs GNU make but has not yet run ./configure,
# give them a diagnostic.
ifeq ($(have-Makefile),yes)

# Make tar archive easier to reproduce.
export TAR_OPTIONS = --owner=0 --group=0 --numeric-owner

include Makefile

# Ensure that $(VERSION) is up to date for dist-related targets, but not
# for others: running autoreconf and recompiling everything isn't cheap.
ifeq (0,$(MAKELEVEL))
  _is-dist-target = $(filter dist% alpha beta major,$(MAKECMDGOALS))
  ifneq (,$(_is-dist-target))
    _curr-ver := $(shell build-aux/git-version-gen .version)
    ifneq ($(_curr-ver),$(VERSION))
      $(info INFO: running autoreconf for new version string: $(_curr-ver))
      dummy := $(shell rm -rf autom4te.cache; autoreconf)
    endif
  endif
endif

include $(srcdir)/Makefile.cfg
include $(srcdir)/Makefile.maint

else

all:
	@echo There seems to be no Makefile in this directory.   1>&2
	@echo "You must run ./configure before running \`make'." 1>&2
	@exit 1

endif

# Tell version 3.79 and up of GNU make to not build goals in this
# directory in parallel.  This is necessary in case someone tries to
# build multiple targets on one command line.
.NOTPARALLEL:
