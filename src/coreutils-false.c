/* false -- wrapper to true with the right true_mode.
   Copyright (C) 2026 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

#include <config.h>
#include "system.h"

#include "true.h"
/* Ensure that the main for true is declared even if the tool is not being
   built in this single-binary. */
int single_binary_main_true (int argc, char **argv);
int single_binary_main_false (int argc, char **argv);

int
single_binary_main_false (int argc, char **argv)
{
  true_mode = TRUE_FALSE;
  return single_binary_main_true (argc, argv);
}
