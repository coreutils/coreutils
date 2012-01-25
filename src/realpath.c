/* realpath - print the resolved path
   Copyright (C) 2011-2012 Free Software Foundation, Inc.

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

/* Written by Pádraig Brady.  */

#include <config.h>
#include <getopt.h>
#include <stdio.h>
#include <sys/types.h>

#include "system.h"
#include "canonicalize.h"
#include "error.h"
#include "quote.h"

/* The official name of this program (e.g., no 'g' prefix).  */
#define PROGRAM_NAME "realpath"

#define AUTHORS proper_name_utf8 ("Padraig Brady", "P\303\241draig Brady")

enum
{
  RELATIVE_TO_OPTION = CHAR_MAX + 1,
  RELATIVE_BASE_OPTION
};

static bool verbose = true;
static bool logical;
static bool use_nuls;
static const char *can_relative_to;
static const char *can_relative_base;

static struct option const longopts[] =
{
  {"canonicalize-existing", no_argument, NULL, 'e'},
  {"canonicalize-missing", no_argument, NULL, 'm'},
  {"relative-to", required_argument, NULL, RELATIVE_TO_OPTION},
  {"relative-base", required_argument, NULL, RELATIVE_BASE_OPTION},
  {"quiet", no_argument, NULL, 'q'},
  {"strip", no_argument, NULL, 's' /* FIXME: deprecate in 2013 or so */},
  {"no-symlinks", no_argument, NULL, 's'},
  {"zero", no_argument, NULL, 'z'},
  {"logical", no_argument, NULL, 'L'},
  {"physical", no_argument, NULL, 'P'},
  {GETOPT_HELP_OPTION_DECL},
  {GETOPT_VERSION_OPTION_DECL},
  {NULL, 0, NULL, 0}
};

void
usage (int status)
{
  if (status != EXIT_SUCCESS)
    emit_try_help ();
  else
    {
      printf (_("Usage: %s [OPTION]... FILE...\n"), program_name);
      fputs (_("\
Print the resolved absolute file name;\n\
all but the last component must exist\n\
\n\
"), stdout);
      fputs (_("\
  -e, --canonicalize-existing  all components of the path must exist\n\
  -m, --canonicalize-missing   no components of the path need exist\n\
  -L, --logical                resolve '..' components before symlinks\n\
  -P, --physical               resolve symlinks as encountered (default)\n\
  -q, --quiet                  suppress most error messages\n\
      --relative-to=FILE       print the resolved path relative to FILE\n\
      --relative-base=FILE     print absolute paths unless paths below FILE\n\
  -s, --strip, --no-symlinks   don't expand symlinks\n\
  -z, --zero                   separate output with NUL rather than newline\n\
\n\
"), stdout);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      emit_ancillary_info ();
    }
  exit (status);
}

/* A wrapper around canonicalize_filename_mode(),
   to call it twice when in LOGICAL mode.  */
static char *
realpath_canon (const char *fname, int can_mode)
{
  char *can_fname = canonicalize_filename_mode (fname, can_mode);
  if (logical && can_fname)  /* canonicalize again to resolve symlinks.  */
    {
      can_mode &= ~CAN_NOLINKS;
      char *can_fname2 = canonicalize_filename_mode (can_fname, can_mode);
      free (can_fname);
      return can_fname2;
    }
  return can_fname;
}

/* Test whether prefix is parent or match of path.  */
static bool _GL_ATTRIBUTE_PURE
path_prefix (const char *prefix, const char *path)
{
  while (*prefix && *path)
    {
      if (*prefix != *path)
        break;
      prefix++;
      path++;
    }
  return (!*prefix && (*path == '/' || !*path));
}

/* Return the length of the longest common prefix
   of PATH1 and PATH2, ensuring only full path components
   are matched.  Return 0 on no match.  */
static int _GL_ATTRIBUTE_PURE
path_common_prefix (const char *path1, const char *path2)
{
  int i = 0;
  int ret = 0;

  while (*path1 && *path2)
    {
      if (*path1 != *path2)
        break;
      if (*path1 == '/')
        ret = i + 1;
      path1++;
      path2++;
      i++;
    }

  if (!*path1 && !*path2)
    ret = i;
  if (!*path1 && *path2 == '/')
    ret = i;
  if (!*path2 && *path1 == '/')
    ret = i;

  return ret;
}

/* Output the relative representation if requested.  */
static bool
relpath (const char *can_fname)
{
  if (can_relative_to)
    {
      /* Enforce --relative-base.  */
      if (can_relative_base)
        {
          if (!path_prefix (can_relative_base, can_fname)
              || !path_prefix (can_relative_base, can_relative_to))
            return false;
        }

      /* Skip the prefix common to --relative-to and path.  */
      int common_index = path_common_prefix (can_relative_to, can_fname);
      const char *relto_suffix = can_relative_to + common_index;
      const char *fname_suffix = can_fname + common_index;

      /* skip over extraneous '/'.  */
      if (*relto_suffix == '/')
        relto_suffix++;
      if (*fname_suffix == '/')
        fname_suffix++;

      /* Replace remaining components of --relative-to with '..', to get
         to a common directory.  Then output the remainder of fname.  */
      if (*relto_suffix)
        {
          fputs ("..", stdout);
          for (; *relto_suffix; ++relto_suffix)
            {
              if (*relto_suffix == '/')
                fputs ("/..", stdout);
            }

          if (*fname_suffix)
            {
              putchar ('/');
              fputs (fname_suffix, stdout);
            }
        }
      else
        {
          if (*fname_suffix)
            fputs (fname_suffix, stdout);
          else
            putchar ('.');
        }

      return true;
    }

  return false;
}

static bool
isdir (const char *path)
{
  struct stat sb;
  if (stat (path, &sb) != 0)
    error (EXIT_FAILURE, errno, _("cannot stat %s"), quote (path));
  return S_ISDIR (sb.st_mode);
}

static bool
process_path (const char *fname, int can_mode)
{
  char *can_fname = realpath_canon (fname, can_mode);
  if (!can_fname)
    {
      if (verbose)
        error (0, errno, "%s", quote (fname));
      return false;
    }

  if (!relpath (can_fname))
    fputs (can_fname, stdout);

  putchar (use_nuls ? '\0' : '\n');

  free (can_fname);

  return true;
}

int
main (int argc, char **argv)
{
  bool ok = true;
  int can_mode = CAN_ALL_BUT_LAST;
  const char *relative_to = NULL;
  const char *relative_base = NULL;

  initialize_main (&argc, &argv);
  set_program_name (argv[0]);
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  while (1)
    {
      int c = getopt_long (argc, argv, "eLmPqsz", longopts, NULL);
      if (c == -1)
        break;
      switch (c)
        {
        case 'e':
          can_mode &= ~CAN_MODE_MASK;
          can_mode |= CAN_EXISTING;
          break;
        case 'm':
          can_mode &= ~CAN_MODE_MASK;
          can_mode |= CAN_MISSING;
          break;
        case 'L':
          can_mode |= CAN_NOLINKS;
          logical = true;
          break;
        case 's':
          can_mode |= CAN_NOLINKS;
          logical = false;
          break;
        case 'P':
          can_mode &= ~CAN_NOLINKS;
          logical = false;
          break;
        case 'q':
          verbose = false;
          break;
        case 'z':
          use_nuls = true;
          break;
        case RELATIVE_TO_OPTION:
          relative_to = optarg;
          break;
        case RELATIVE_BASE_OPTION:
          relative_base = optarg;
          break;
        case_GETOPT_HELP_CHAR;
        case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);
        default:
          usage (EXIT_FAILURE);
        }
    }

  if (optind >= argc)
    {
      error (0, 0, _("missing operand"));
      usage (EXIT_FAILURE);
    }

  if (relative_base && !relative_to)
    {
      error (0, 0, _("--relative-base requires --relative-to"));
      usage (EXIT_FAILURE);
    }

  bool need_dir = (can_mode & CAN_MODE_MASK) == CAN_EXISTING;
  if (relative_to)
    {
      can_relative_to = realpath_canon (relative_to, can_mode);
      if (!can_relative_to)
        error (EXIT_FAILURE, errno, "%s", quote (relative_to));
      if (need_dir && !isdir (can_relative_to))
        error (EXIT_FAILURE, ENOTDIR, "%s", quote (relative_to));
    }
  if (relative_base)
    {
      can_relative_base = realpath_canon (relative_base, can_mode);
      if (!can_relative_base)
        error (EXIT_FAILURE, errno, "%s", quote (relative_base));
      if (need_dir && !isdir (can_relative_base))
        error (EXIT_FAILURE, ENOTDIR, "%s", quote (relative_base));
    }

  for (; optind < argc; ++optind)
    ok &= process_path (argv[optind], can_mode);

  exit (ok ? EXIT_SUCCESS : EXIT_FAILURE);
}
