/* getlimits - print various platform dependent limits.
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

/* Written by Pádraig Brady  */

#include <config.h>             /* sets _FILE_OFFSET_BITS=64 etc. */
#include <stdio.h>
#include <sys/types.h>

#include "system.h"
#include "c-ctype.h"
#include "long-options.h"

#define PROGRAM_NAME "getlimits"

#define AUTHORS proper_name_utf8 ("Padraig Brady", "P\303\241draig Brady")

#ifndef TIME_T_MAX
# define TIME_T_MAX TYPE_MAXIMUM (time_t)
#endif

#ifndef TIME_T_MIN
# define TIME_T_MIN TYPE_MINIMUM (time_t)
#endif

#ifndef SSIZE_MIN
# define SSIZE_MIN TYPE_MINIMUM (ssize_t)
#endif

#ifndef PID_T_MIN
# define PID_T_MIN TYPE_MINIMUM (pid_t)
#endif

/* These are not interesting to print.
 * Instead of these defines it would be nice to be able to do
 * #ifdef (TYPE##_MIN) in function macro below.  */
#define SIZE_MIN 0
#define UCHAR_MIN 0
#define UINT_MIN 0
#define ULONG_MIN 0
#define UINTMAX_MIN 0
#define UID_T_MIN 0
#define GID_T_MIN 0

void
usage (int status)
{
  if (status != EXIT_SUCCESS)
    fprintf (stderr, _("Try `%s --help' for more information.\n"),
             program_name);
  else
    {
      printf (_("\
Usage: %s\n\
"), program_name);

      fputs (_("\
Output platform dependent limits in a format useful for shell scripts.\n\
\n\
"), stdout);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      emit_bug_reporting_address ();
    }
  exit (status);
}

/* Add absolute values of ascii decimal strings.
 * Strings can have leading spaces.
 * If any string has a '-' it's preserved in the output:
 * I.E.
 *    1 +  1 ->  2
 *   -1 + -1 -> -2
 *   -1 +  1 -> -2
 *    1 + -1 -> -2
 */
static char *
decimal_ascii_add (const char *str1, const char *str2)
{
  int len1 = strlen (str1);
  int len2 = strlen (str2);
  int rlen = MAX (len1, len2) + 3;  /* space for extra digit or sign + NUL */
  char *result = xmalloc (rlen);
  char *rp = result + rlen - 1;
  const char *d1 = str1 + len1 - 1;
  const char *d2 = str2 + len2 - 1;
  int carry = 0;
  *rp = '\0';

  while (1)
    {
      char c1 = (d1 < str1 ? ' ' : (*d1 == '-' ? ' ' : *d1--));
      char c2 = (d2 < str2 ? ' ' : (*d2 == '-' ? ' ' : *d2--));
      char t1 = c1 + c2 + carry;    /* ASCII digits are BCD */
      if (!c_isdigit (c1) && !c_isdigit (c2) && !carry)
        break;
      carry = t1 > '0' + '9' || t1 == ' ' + '9' + 1;
      t1 += 6 * carry;
      *--rp = (t1 & 0x0F) | 0x30;   /* top nibble to ASCII */
    }
  if ((d1 >= str1 && *d1 == '-') || (d2 >= str2 && (*d2 == '-')))
    *--rp = '-';

  if (rp != result)
    memmove (result, rp, rlen - (rp - result));

  return result;
}

int
main (int argc, char **argv)
{
  char limit[64];               /* big enough for 128 bit at least */
  char *oflow;

  initialize_main (&argc, &argv);
  set_program_name (argv[0]);
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  initialize_exit_failure (EXIT_FAILURE);
  atexit (close_stdout);

  parse_long_options (argc, argv, PROGRAM_NAME, PACKAGE_NAME, VERSION,
                      usage, AUTHORS, (char const *) NULL);

#define print_int(TYPE)                                                  \
  snprintf (limit, sizeof(limit), "%"PRIuMAX, (uintmax_t)TYPE##_MAX);    \
  printf (#TYPE"_MAX=%s\n", limit);                                      \
  oflow = decimal_ascii_add (limit, "1");                                \
  printf (#TYPE"_OFLOW=%s\n", oflow);                                    \
  free (oflow);                                                          \
  if (TYPE##_MIN)                                                        \
    {                                                                    \
      snprintf (limit, sizeof(limit), "%"PRIdMAX, (intmax_t)TYPE##_MIN); \
      printf (#TYPE"_MIN=%s\n", limit);                                  \
      oflow = decimal_ascii_add (limit, "-1");                           \
      printf (#TYPE"_UFLOW=%s\n", oflow);                                \
      free (oflow);                                                      \
    }

  /* Variable sized ints */
  print_int (CHAR);
  print_int (SCHAR);
  print_int (UCHAR);
  print_int (SHRT);
  print_int (INT);
  print_int (UINT);
  print_int (LONG);
  print_int (ULONG);
  print_int (SIZE);
  print_int (SSIZE);
  print_int (TIME_T);
  print_int (UID_T);
  print_int (GID_T);
  print_int (PID_T);
  print_int (OFF_T);
  print_int (INTMAX);
  print_int (UINTMAX);
}
