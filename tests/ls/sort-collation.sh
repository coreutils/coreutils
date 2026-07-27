#!/bin/sh
# Ensure ls sorts filenames according to the LC_COLLATE locale category.

# Copyright (C) 2026 Free Software Foundation, Inc.

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

. "${srcdir=.}/tests/init.sh"; path_prepend_ ./src
print_ver_ ls

test "$LOCALE_FR_UTF8" != "none" \
  || skip_ "no UTF-8 locale available for collation comparison"

# Skip if the available locale's collation order matches C
# (we need a locale that interleaves upper- and lower-case to expose
# whether LC_COLLATE is honored).
locale_sorted=$(printf 'Cherry\nbanana\n' | LC_ALL=$LOCALE_FR_UTF8 sort)
case $locale_sorted in
  'banana'*) ;;
  *) skip_ "locale $LOCALE_FR_UTF8 sorts like C; cannot distinguish" ;;
esac

mkdir dir || framework_failure_
touch dir/Apple dir/banana dir/Cherry dir/date_fruit || framework_failure_

# Under LC_ALL=C, uppercase sorts before lowercase.
cat <<\EOF > exp_c || framework_failure_
Apple
Cherry
banana
date_fruit
EOF

# Under a UTF-8 locale, case is folded for collation.
cat <<\EOF > exp_locale || framework_failure_
Apple
banana
Cherry
date_fruit
EOF

LC_ALL=C ls dir > out_c || fail=1
compare exp_c out_c || fail=1

LC_ALL=$LOCALE_FR_UTF8 ls dir > out_lc_all || fail=1
compare exp_locale out_lc_all || fail=1

# LC_COLLATE alone (with LC_ALL unset) must also be honored.
(
  unset LC_ALL
  LANG=C LC_COLLATE=$LOCALE_FR_UTF8 ls dir
) > out_lc_collate || fail=1
compare exp_locale out_lc_collate || fail=1

Exit $fail
