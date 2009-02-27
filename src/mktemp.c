/* Create a temporary file or directory, safely.
   Copyright (C) 2007-2009 Free Software Foundation, Inc.

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

/* Written by Jim Meyering.  */

#include <config.h>
#include <stdio.h>
#include <sys/types.h>
#include <getopt.h>

#include "system.h"

#include "error.h"
#include "filenamecat.h"
#include "quote.h"
#include "tempname.h"

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "mktemp"

#define AUTHORS proper_name ("Jim Meyering")

static const char *default_template = "tmp.XXXXXXXXXX";

/* For long options that have no equivalent short option, use a
   non-character as a pseudo short option, starting with CHAR_MAX + 1.  */
enum
{
  TMPDIR_OPTION = CHAR_MAX + 1
};

static struct option const longopts[] =
{
  {"directory", no_argument, NULL, 'd'},
  {"quiet", no_argument, NULL, 'q'},
  {"dry-run", no_argument, NULL, 'u'},
  {"tmpdir", optional_argument, NULL, TMPDIR_OPTION},
  {GETOPT_HELP_OPTION_DECL},
  {GETOPT_VERSION_OPTION_DECL},
  {NULL, 0, NULL, 0}
};

void
usage (int status)
{
  if (status != EXIT_SUCCESS)
    fprintf (stderr, _("Try `%s --help' for more information.\n"),
             program_name);
  else
    {
      printf (_("Usage: %s [OPTION]... [TEMPLATE]\n"), program_name);
      fputs (_("\
Create a temporary file or directory, safely, and print its name.\n\
If TEMPLATE is not specified, use tmp.XXXXXXXXXX.\n\
"), stdout);
      fputs ("\n", stdout);
      fputs (_("\
  -d, --directory  create a directory, not a file\n\
"), stdout);
      fputs (_("\
  -q, --quiet      suppress diagnostics about file/dir-creation failure\n\
"), stdout);
      fputs (_("\
  -u, --dry-run    do not create anything; merely print a name (unsafe)\n\
"), stdout);
      fputs (_("\
  --tmpdir[=DIR]   interpret TEMPLATE relative to DIR.  If DIR is\n\
                     not specified, use $TMPDIR if set, else /tmp.\n\
                     With this option, TEMPLATE must not be an absolute name.\n\
                     Unlike with -t, TEMPLATE may contain slashes, but even\n\
                     here, mktemp still creates only the final component.\n\
"), stdout);
      fputs ("\n", stdout);
      fputs (_("\
  -p DIR           use DIR as a prefix; implies -t [deprecated]\n\
"), stdout);
      fputs (_("\
  -t               interpret TEMPLATE as a single file name component,\n\
                     relative to a directory: $TMPDIR, if set; else the\n\
                     directory specified via -p; else /tmp [deprecated]\n\
"), stdout);
      fputs ("\n", stdout);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      emit_bug_reporting_address ();
    }

  exit (status);
}

static size_t
count_trailing_X_s (const char *s)
{
  size_t len = strlen (s);
  size_t n = 0;
  for ( ; len && s[len-1] == 'X'; len--)
    ++n;
  return n;
}

static int
mkstemp_len (char *tmpl, size_t suff_len, bool dry_run)
{
  return gen_tempname_len (tmpl, dry_run ? GT_NOCREATE : GT_FILE, suff_len);
}

static int
mkdtemp_len (char *tmpl, size_t suff_len, bool dry_run)
{
  return gen_tempname_len (tmpl, dry_run ? GT_NOCREATE : GT_DIR, suff_len);
}

int
main (int argc, char **argv)
{
  char const *dest_dir;
  char const *dest_dir_arg = NULL;
  bool suppress_stderr = false;
  int c;
  unsigned int n_args;
  char *template;
  bool use_dest_dir = false;
  bool deprecated_t_option = false;
  bool create_directory = false;
  bool dry_run = false;
  int status = EXIT_SUCCESS;
  size_t x_count;
  char *dest_name;

  initialize_main (&argc, &argv);
  set_program_name (argv[0]);
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  while ((c = getopt_long (argc, argv, "dp:qtuV", longopts, NULL)) != -1)
    {
      switch (c)
        {
        case 'd':
          create_directory = true;
          break;
        case 'p':
          dest_dir_arg = optarg;
          use_dest_dir = true;
          break;
        case 'q':
          suppress_stderr = true;
          break;
        case 't':
          use_dest_dir = true;
          deprecated_t_option = true;
          break;
        case 'u':
          dry_run = true;
          break;

        case TMPDIR_OPTION:
          use_dest_dir = true;
          dest_dir_arg = optarg;
          break;

        case_GETOPT_HELP_CHAR;

        case 'V':
        case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);
        default:
          usage (EXIT_FAILURE);
        }
    }

  if (suppress_stderr)
    {
      /* From here on, redirect stderr to /dev/null.
         A diagnostic from getopt_long, above, would still go to stderr.  */
      if (!freopen ("/dev/null", "wb", stderr))
        error (EXIT_FAILURE, errno,
               _("failed to redirect stderr to /dev/null"));
    }

  n_args = argc - optind;
  if (2 <= n_args)
    {
      error (0, 0, _("too many templates"));
      usage (EXIT_FAILURE);
    }

  if (n_args == 0)
    {
      use_dest_dir = true;
      template = (char *) default_template;
    }
  else
    {
      template = argv[optind];
    }

  x_count = count_trailing_X_s (template);
  if (x_count < 3)
    error (EXIT_FAILURE, 0, _("too few X's in template %s"), quote (template));

  if (use_dest_dir)
    {
      if (deprecated_t_option)
        {
          char *env = getenv ("TMPDIR");
          dest_dir = (env && *env
                      ? env
                      : (dest_dir_arg ? dest_dir_arg : "/tmp"));

          if (last_component (template) != template)
            error (EXIT_FAILURE, 0,
                   _("invalid template, %s, contains directory separator"),
                   quote (template));
        }
      else
        {
          if (dest_dir_arg && *dest_dir_arg)
            dest_dir = dest_dir_arg;
          else
            {
              char *env = getenv ("TMPDIR");
              dest_dir = (env && *env ? env : "/tmp");
            }
          if (IS_ABSOLUTE_FILE_NAME (template))
            error (EXIT_FAILURE, 0,
                   _("invalid template, %s; with --tmpdir,"
                     " it may not be absolute"),
                   quote (template));
        }

      template = file_name_concat (dest_dir, template, NULL);
    }
  else
    {
      template = xstrdup (template);
    }

  /* Make a copy to be used in case of diagnostic, since failing
     mkstemp may leave the buffer in an undefined state.  */
  dest_name = xstrdup (template);

  if (create_directory)
    {
      int err = mkdtemp_len (dest_name, x_count, dry_run);
      if (err != 0)
        {
          error (0, errno, _("failed to create directory via template %s"),
                 quote (template));
          status = EXIT_FAILURE;
        }
    }
  else
    {
      int fd = mkstemp_len (dest_name, x_count, dry_run);
      if (fd < 0 || (!dry_run && close (fd) != 0))
        {
          error (0, errno, _("failed to create file via template %s"),
                 quote (template));
          status = EXIT_FAILURE;
        }
    }

  if (status == EXIT_SUCCESS)
    puts (dest_name);

#ifdef lint
  free (dest_name);
  free (template);
#endif

  exit (status);
}
