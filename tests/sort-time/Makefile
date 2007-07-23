# Copyright (C) 1997 Free Software Foundation, Inc.

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

N = 1000000

dot30 = ..............................
dot31 = $(dot30).

safe_redirect_prologue = rm -f $@ $@-tmp
safe_redirect = $@-tmp && mv $@-tmp $@

time = /p/bin/time --verbose

all: t30 t31

t3%: in-3%
	@$(safe_redirect_prologue)
	$(time) sort $< > $(safe_redirect)

rand-digits: rand-gen
	@$(safe_redirect_prologue)
	./$< --integer $(N) |tr -d '\012' > $(safe_redirect)

in-30: rand-digits
	@$(safe_redirect_prologue)
	perl -pe 's/($(dot30))/$$1\n/g' $< > $(safe_redirect)

in-31: rand-digits
	@$(safe_redirect_prologue)
	perl -pe 's/($(dot31))/$$1\n/g' $< > $(safe_redirect)

clean:
	rm -f t30 t31 in-30 in-31

realclean: clean
	rm -f rand-digits
