/* yes - output a string repeatedly until killed
   Copyright (C) 1991-2016 Free Software Foundation, Inc.

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

/* David MacKenzie <djm@gnu.ai.mit.edu> */

#include <config.h>
#include <stdio.h>
#include <sys/types.h>
#include <getopt.h>

#include "system.h"

#include "error.h"
#include "long-options.h"

/* The official name of this program (e.g., no 'g' prefix).  */
#define PROGRAM_NAME "yes"

#define AUTHORS proper_name ("David MacKenzie")

void
usage (int status)
{
  if (status != EXIT_SUCCESS)
    emit_try_help ();
  else
    {
      printf (_("\
Usage: %s [STRING]...\n\
  or:  %s OPTION\n\
"),
              program_name, program_name);

      fputs (_("\
Repeatedly output a line with all specified STRING(s), or 'y'.\n\
\n\
"), stdout);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      emit_ancillary_info (PROGRAM_NAME);
    }
  exit (status);
}

int
main (int argc, char **argv)
{
  char buf[BUFSIZ];
  char *pbuf = buf;

  initialize_main (&argc, &argv);
  set_program_name (argv[0]);
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  parse_long_options (argc, argv, PROGRAM_NAME, PACKAGE_NAME, Version,
                      usage, AUTHORS, (char const *) NULL);
  if (getopt_long (argc, argv, "+", NULL, NULL) != -1)
    usage (EXIT_FAILURE);

  char **operand_lim = argv + argc;
  if (optind == argc)
    *operand_lim++ = bad_cast ("y");

  /* Buffer data locally once, rather than having the
     large overhead of stdio buffering each item.  */
  char **operandp;
  for (operandp = argv + optind; operandp < operand_lim; operandp++)
    {
      size_t len = strlen (*operandp);
      if (BUFSIZ < len || BUFSIZ - len <= pbuf - buf)
        break;
      memcpy (pbuf, *operandp, len);
      pbuf += len;
      *pbuf++ = operandp + 1 == operand_lim ? '\n' : ' ';
    }

  /* The normal case is to continuously output the local buffer.  */
  if (operandp == operand_lim)
    {
      size_t line_len = pbuf - buf;
      size_t lines = BUFSIZ / line_len;
      while (--lines)
        {
          memcpy (pbuf, pbuf - line_len, line_len);
          pbuf += line_len;
        }
    }
  while (operandp == operand_lim)
    {
      char const* pwrite = buf;
      size_t to_write = pbuf - buf;
      while (to_write)
        {
          ssize_t written = write (STDOUT_FILENO, pwrite, to_write);
          if (written < 0)
            {
              error (0, errno, _("standard output"));
              return EXIT_FAILURE;
            }
          to_write -= written;
          pwrite += written;
        }
    }

  /* If the data doesn't fit in BUFSIZ then output
     what we've buffered, and iterate over the remaining items.  */
  while (true)
    {
      if ((pbuf - buf) && fwrite (buf, pbuf - buf, 1, stdout) != 1)
        {
          error (0, errno, _("standard output"));
          clearerr (stdout);
          return EXIT_FAILURE;
        }
      for (char **trailing = operandp; trailing < operand_lim; trailing++)
        if (fputs (*trailing, stdout) == EOF
            || putchar (trailing + 1 == operand_lim ? '\n' : ' ') == EOF)
          {
            error (0, errno, _("standard output"));
            clearerr (stdout);
            return EXIT_FAILURE;
          }
    }
}
