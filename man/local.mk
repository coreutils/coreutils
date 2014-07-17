# Make coreutils man pages.				-*-Makefile-*-
# This is included by the top-level Makefile.am.

# Copyright (C) 2002-2014 Free Software Foundation, Inc.

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

EXTRA_DIST += man/help2man man/dummy-man

## Graceful degradation for systems lacking perl.
if HAVE_PERL
run_help2man = $(PERL) -- $(srcdir)/man/help2man
else
run_help2man = $(SHELL) $(srcdir)/man/dummy-man
endif

man1_MANS = @man1_MANS@
EXTRA_DIST += $(man1_MANS:.1=.x)

EXTRA_MANS = @EXTRA_MANS@
EXTRA_DIST += $(EXTRA_MANS:.1=.x)

ALL_MANS = $(man1_MANS) $(EXTRA_MANS)

CLEANFILES += $(ALL_MANS)

# This is a kludge to remove generated 'man/*.1' from a non-srcdir build.
# Without this, "make distcheck" might fail.
distclean-local:
	test x$(srcdir) = x$(builddir) || rm -f $(ALL_MANS)

# Dependencies common to all man pages.  Updated below.
mandeps =

# Depend on this to get version number changes.
mandeps += .version

# This is required so that changes to e.g., emit_bug_reporting_address
# provoke regeneration of all the manpages.
mandeps += $(top_srcdir)/src/system.h

$(ALL_MANS): $(mandeps)

# Most prog.1 man pages depend on src/$prog, except when they are part of a
# single binary, in which case they depend on src/coreutils. The exceptions
# are handled by converting $name to $prog on the following code.
# $(ALL_MANS) includes the $(EXTRA_MANS) so even the programs that are not
# being installed will have the right dependency for the manpages.
DISTCLEANFILES += man/dynamic-deps.mk
man/dynamic-deps.mk: Makefile
	$(AM_V_GEN)rm -f $@ $@-t
	$(AM_V_at)for man in $(ALL_MANS); do				\
		name=`echo "$$man"|sed 's|.*/||; s|\.1$$||'` || exit 1;	\
		case $$name in						\
		     arch) prog='uname';;				\
		  install) prog='ginstall';;				\
		     test) prog='[';;					\
			*) prog=$$name;;				\
		esac;							\
		case " $(single_binary_progs) " in			\
			*" $$prog "*)					\
				echo $$man: src/coreutils$(EXEEXT);;	\
			*)						\
				echo $$man: src/$$prog$(EXEEXT);;	\
		esac							\
	done > $@-t							\
	&& mv $@-t $@

# Include the generated man dependencies.
@AMDEP_TRUE@@am__include@ man/dynamic-deps.mk

.x.1:
	$(AM_V_GEN)name=`echo $@ | sed 's|.*/||; s|\.1$$||'` || exit 1;	\
## Ensure that help2man runs the 'src/ginstall' binary as 'install' when
## creating 'install.1'.  Similarly, ensure that it uses the 'src/[' binary
## to create 'test.1'.
	case $$name in							\
	  install) prog='ginstall'; argv=$$name;;			\
	     test) prog='['; argv='[';;					\
		*) prog=$$name; argv=$$prog;;				\
	esac;								\
## Note the use of $$t/$*, rather than just '$*' as in other packages.
## That is necessary to avoid failures for programs that are also shell
## built-in functions like echo, false, printf, pwd.
	rm -f $@ $@-t							\
	  && t=$*.td							\
	  && rm -rf $$t							\
	  && $(MKDIR_P) $$t						\
	  && (cd $$t && $(LN_S) '$(abs_top_builddir)/src/'$$prog $$argv) \
	  && $(run_help2man)						\
		     --source='$(PACKAGE_STRING)'			\
		     --include=$(srcdir)/man/$$name.x			\
		     --output=$$t/$$name.1 $$t/$$argv			\
		     --info-page='coreutils \(aq'$$name' invocation\(aq' \
	  && sed \
	       -e 's|$*\.td/||g' \
	       -e '/For complete documentation/d' \
	       $$t/$$name.1 > $@-t			\
	  && rm -rf $$t							\
	  && chmod a-w $@-t						\
	  && mv $@-t $@
