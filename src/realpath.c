/* realpath - print the resolved path
   Copyright (C) 2011-2025 Free Software Foundation, Inc.

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

/* Written by Pádraig Brady.  */

#include <config.h>
#include <getopt.h>
#include <stdio.h>
#include <sys/types.h>

#include "system.h"
#include "canonicalize.h"
#include "relpath.h"

/* The official name of this program (e.g., no 'g' prefix).  */
#define PROGRAM_NAME "realpath"

#define AUTHORS proper_name_lite ("Padraig Brady", "P\303\241draig Brady")

enum
{
  RELATIVE_TO_OPTION = CHAR_MAX + 1,
  RELATIVE_BASE_OPTION
};

static bool verbose = true;
static bool logical;
static bool use_nuls;
static char const *can_relative_to;
static char const *can_relative_base;

static struct option const longopts[] =
{
  {"canonicalize", no_argument, nullptr, 'E'},
  {"canonicalize-existing", no_argument, nullptr, 'e'},
  {"canonicalize-missing", no_argument, nullptr, 'm'},
  {"relative-to", required_argument, nullptr, RELATIVE_TO_OPTION},
  {"relative-base", required_argument, nullptr, RELATIVE_BASE_OPTION},
  {"quiet", no_argument, nullptr, 'q'},
  {"strip", no_argument, nullptr, 's'},
  {"no-symlinks", no_argument, nullptr, 's'},
  {"zero", no_argument, nullptr, 'z'},
  {"logical", no_argument, nullptr, 'L'},
  {"physical", no_argument, nullptr, 'P'},
  {GETOPT_HELP_OPTION_DECL},
  {GETOPT_VERSION_OPTION_DECL},
  {nullptr, 0, nullptr, 0}
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
Print the resolved absolute file name.\n\
"), stdout);
      fputs (_("\
  -E, --canonicalize           all but the last component must exist (default)\
\n\
  -e, --canonicalize-existing  all components of the path must exist\n\
  -m, --canonicalize-missing   no path components need exist or be a directory\
\n\
  -L, --logical                resolve '..' components before symlinks\n\
  -P, --physical               resolve symlinks as encountered (default)\n\
  -q, --quiet                  suppress most error messages\n\
      --relative-to=DIR        print the resolved path relative to DIR\n\
      --relative-base=DIR      print absolute paths unless paths below DIR\n\
  -s, --strip, --no-symlinks   don't expand symlinks\n\
  -z, --zero                   end each output line with NUL, not newline\n\
"), stdout);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      emit_ancillary_info (PROGRAM_NAME);
    }
  exit (status);
}

/* A wrapper around canonicalize_filename_mode(),
   to call it twice when in LOGICAL mode.  */
static char *
realpath_canon (char const *fname, int can_mode)
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

/* Test whether canonical prefix is parent or match of path.  */
ATTRIBUTE_PURE
static bool
path_prefix (char const *prefix, char const *path)
{
  /* We already know prefix[0] and path[0] are '/'.  */
  prefix++;
  path++;

  /* '/' is the prefix of everything except '//' (since we know '//'
     is only present after canonicalization if it is distinct).  */
  if (!*prefix)
    return *path != '/';

  /* Likewise, '//' is a prefix of any double-slash path.  */
  if (*prefix == '/' && !prefix[1])
    return *path == '/';

  /* Any other prefix has a non-slash portion.  */
  while (*prefix && *path)
    {
      if (*prefix != *path)
        break;
      prefix++;
      path++;
    }
  return (!*prefix && (*path == '/' || !*path));
}

static bool
isdir (char const *path)
{
  struct stat sb;
  if (stat (path, &sb) != 0)
    error (EXIT_FAILURE, errno, _("cannot stat %s"), quoteaf (path));
  return S_ISDIR (sb.st_mode);
}

static bool
process_path (char const *fname, int can_mode)
{
  char *can_fname = realpath_canon (fname, can_mode);
  if (!can_fname)
    {
      if (verbose)
        error (0, errno, "%s", quotef (fname));
      return false;
    }

  if (!can_relative_to
      || (can_relative_base && !path_prefix (can_relative_base, can_fname))
      || (can_relative_to && !relpath (can_fname, can_relative_to, nullptr, 0)))
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
  char const *relative_to = nullptr;
  char const *relative_base = nullptr;

  initialize_main (&argc, &argv);
  set_program_name (argv[0]);
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  while (true)
    {
      int c = getopt_long (argc, argv, "EeLmPqsz", longopts, nullptr);
      if (c == -1)
        break;
      switch (c)
        {
        case 'E':
          can_mode &= ~CAN_MODE_MASK;
          can_mode |= CAN_ALL_BUT_LAST;
          break;
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
    relative_to = relative_base;

  bool need_dir = (can_mode & CAN_MODE_MASK) == CAN_EXISTING;
  if (relative_to)
    {
      can_relative_to = realpath_canon (relative_to, can_mode);
      if (!can_relative_to)
        error (EXIT_FAILURE, errno, "%s", quotef (relative_to));
      if (need_dir && !isdir (can_relative_to))
        error (EXIT_FAILURE, ENOTDIR, "%s", quotef (relative_to));
    }
  if (relative_base == relative_to)
    can_relative_base = can_relative_to;
  else if (relative_base)
    {
      char *base = realpath_canon (relative_base, can_mode);
      if (!base)
        error (EXIT_FAILURE, errno, "%s", quotef (relative_base));
      if (need_dir && !isdir (base))
        error (EXIT_FAILURE, ENOTDIR, "%s", quotef (relative_base));
      /* --relative-to is a no-op if it does not have --relative-base
           as a prefix */
      if (path_prefix (base, can_relative_to))
        can_relative_base = base;
      else
        {
          free (base);
          can_relative_base = can_relative_to;
          can_relative_to = nullptr;
        }
    }

  for (; optind < argc; ++optind)
    ok &= process_path (argv[optind], can_mode);

  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
