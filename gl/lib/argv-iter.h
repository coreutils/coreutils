/* Iterate over arguments from argv or --files0-from=FILE
   Copyright (C) 2008-2009 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include <stdio.h>
#include <stdbool.h>

struct argv_iterator;
enum argv_iter_err;

#undef _ATTRIBUTE_NONNULL_
#if __GNUC__ == 3 && __GNUC_MINOR__ >= 3 || 3 < __GNUC__
# define _ATTRIBUTE_NONNULL_(m) __attribute__ ((__nonnull__ (m)))
#else
# define _ATTRIBUTE_NONNULL_(m)
#endif

enum argv_iter_err
{
  AI_ERR_OK = 1,
  AI_ERR_EOF,
  AI_ERR_MEM,
  AI_ERR_READ
};

struct argv_iterator *argv_iter_init_argv (char **argv)
  _ATTRIBUTE_NONNULL_ (1);
struct argv_iterator *argv_iter_init_stream (FILE *fp)
  _ATTRIBUTE_NONNULL_ (1);
char *argv_iter (struct argv_iterator *, enum argv_iter_err *)
  _ATTRIBUTE_NONNULL_ (1) _ATTRIBUTE_NONNULL_ (2);
size_t argv_iter_n_args (struct argv_iterator const *)
  _ATTRIBUTE_NONNULL_ (1);
void argv_iter_free (struct argv_iterator *)
  _ATTRIBUTE_NONNULL_ (1);
