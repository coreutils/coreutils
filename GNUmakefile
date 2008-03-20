# Having a separate GNUmakefile lets me `include' the dynamically
# generated rules created via cfg.mk (package-local configuration)
# as well as maint.mk (generic maintainer rules).
# This makefile is used only if you run GNU Make.
# It is necessary if you want to build targets usually of interest
# only to the maintainer.

# Copyright (C) 2001, 2003, 2006-2008 Free Software Foundation, Inc.

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

# If the user runs GNU make but has not yet run ./configure,
# give them a diagnostic.
_have-Makefile := $(shell test -f Makefile && echo yes)
ifeq ($(_have-Makefile),yes)

# Make tar archive easier to reproduce.
export TAR_OPTIONS = --owner=0 --group=0 --numeric-owner

include Makefile

# Some projects override e.g., _autoreconf here.
-include $(srcdir)/cfg.mk
include $(srcdir)/maint.mk

# Allow cfg.mk to override these.
_build-aux ?= build-aux
_autoreconf ?= autoreconf

# Ensure that $(VERSION) is up to date for dist-related targets, but not
# for others: rerunning autoreconf and recompiling everything isn't cheap.
_have-git-version-gen := \
  $(shell test -f $(srcdir)/$(_build-aux)/git-version-gen && echo yes)
ifeq ($(_have-git-version-gen)0,yes$(MAKELEVEL))
  _is-dist-target = $(filter-out %clean, \
    $(filter dist% alpha beta major,$(MAKECMDGOALS)))
  ifneq (,$(_is-dist-target))
    _curr-ver := $(shell cd $(srcdir) && ./$(_build-aux)/git-version-gen \
                   $(srcdir)/.tarball-version)
    ifneq ($(_curr-ver),$(VERSION))
      $(info INFO: running autoreconf for new version string: $(_curr-ver))
      _dummy := $(shell cd $(srcdir) && rm -rf autom4te.cache && $(_autoreconf)))
    endif
  endif
endif

else

.DEFAULT_GOAL := abort-due-to-no-makefile

# The package can override .DEFAULT_GOAL to run actions like autoreconf.
-include ./cfg.mk
include ./maint.mk

abort-due-to-no-makefile:
	@echo There seems to be no Makefile in this directory.   1>&2
	@echo "You must run ./configure before running \`make'." 1>&2
	@exit 1

endif

# Tell version 3.79 and up of GNU make to not build goals in this
# directory in parallel.  This is necessary in case someone tries to
# build multiple targets on one command line.
.NOTPARALLEL:
