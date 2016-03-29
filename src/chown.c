/* chown -- change user and group ownership of files
   Copyright (C) 1989-2016 Free Software Foundation, Inc.

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

/* Written by David MacKenzie <djm@gnu.ai.mit.edu>. */

#include <config.h>
#include <stdio.h>
#include <sys/types.h>
#include <getopt.h>

#include "system.h"
#include "chown-core.h"
#include "error.h"
#include "fts_.h"
#include "quote.h"
#include "root-dev-ino.h"
#include "userspec.h"

/* The official name of this program (e.g., no 'g' prefix).  */
#define PROGRAM_NAME "chown"

#define AUTHORS \
  proper_name ("David MacKenzie"), \
  proper_name ("Jim Meyering")

/* The argument to the --reference option.  Use the owner and group IDs
   of this file.  This file must exist.  */
static char *reference_file;

/* For long options that have no equivalent short option, use a
   non-character as a pseudo short option, starting with CHAR_MAX + 1.  */
enum
{
  DEREFERENCE_OPTION = CHAR_MAX + 1,
  FROM_OPTION,
  NO_PRESERVE_ROOT,
  PRESERVE_ROOT,
  REFERENCE_FILE_OPTION
};

static struct option const long_options[] =
{
  {"recursive", no_argument, NULL, 'R'},
  {"changes", no_argument, NULL, 'c'},
  {"dereference", no_argument, NULL, DEREFERENCE_OPTION},
  {"from", required_argument, NULL, FROM_OPTION},
  {"no-dereference", no_argument, NULL, 'h'},
  {"no-preserve-root", no_argument, NULL, NO_PRESERVE_ROOT},
  {"preserve-root", no_argument, NULL, PRESERVE_ROOT},
  {"quiet", no_argument, NULL, 'f'},
  {"silent", no_argument, NULL, 'f'},
  {"reference", required_argument, NULL, REFERENCE_FILE_OPTION},
  {"verbose", no_argument, NULL, 'v'},
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
      printf (_("\
Usage: %s [OPTION]... [OWNER][:[GROUP]] FILE...\n\
  or:  %s [OPTION]... --reference=RFILE FILE...\n\
"),
              program_name, program_name);
      fputs (_("\
Change the owner and/or group of each FILE to OWNER and/or GROUP.\n\
With --reference, change the owner and group of each FILE to those of RFILE.\n\
\n\
"), stdout);
      fputs (_("\
  -c, --changes          like verbose but report only when a change is made\n\
  -f, --silent, --quiet  suppress most error messages\n\
  -v, --verbose          output a diagnostic for every file processed\n\
"), stdout);
      fputs (_("\
      --dereference      affect the referent of each symbolic link (this is\n\
                         the default), rather than the symbolic link itself\n\
  -h, --no-dereference   affect symbolic links instead of any referenced file\n\
"), stdout);
      fputs (_("\
                         (useful only on systems that can change the\n\
                         ownership of a symlink)\n\
"), stdout);
      fputs (_("\
      --from=CURRENT_OWNER:CURRENT_GROUP\n\
                         change the owner and/or group of each file only if\n\
                         its current owner and/or group match those specified\n\
                         here.  Either may be omitted, in which case a match\n\
                         is not required for the omitted attribute\n\
"), stdout);
      fputs (_("\
      --no-preserve-root  do not treat '/' specially (the default)\n\
      --preserve-root    fail to operate recursively on '/'\n\
"), stdout);
      fputs (_("\
      --reference=RFILE  use RFILE's owner and group rather than\n\
                         specifying OWNER:GROUP values\n\
"), stdout);
      fputs (_("\
  -R, --recursive        operate on files and directories recursively\n\
"), stdout);
      fputs (_("\
\n\
The following options modify how a hierarchy is traversed when the -R\n\
option is also specified.  If more than one is specified, only the final\n\
one takes effect.\n\
\n\
  -H                     if a command line argument is a symbolic link\n\
                         to a directory, traverse it\n\
  -L                     traverse every symbolic link to a directory\n\
                         encountered\n\
  -P                     do not traverse any symbolic links (default)\n\
\n\
"), stdout);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      fputs (_("\
\n\
Owner is unchanged if missing.  Group is unchanged if missing, but changed\n\
to login group if implied by a ':' following a symbolic OWNER.\n\
OWNER and GROUP may be numeric as well as symbolic.\n\
"), stdout);
      printf (_("\
\n\
Examples:\n\
  %s root /u        Change the owner of /u to \"root\".\n\
  %s root:staff /u  Likewise, but also change its group to \"staff\".\n\
  %s -hR root /u    Change the owner of /u and subfiles to \"root\".\n\
"),
              program_name, program_name, program_name);
      emit_ancillary_info (PROGRAM_NAME);
    }
  exit (status);
}

int
main (int argc, char **argv)
{
  bool preserve_root = false;

  uid_t uid = -1;	/* Specified uid; -1 if not to be changed. */
  gid_t gid = -1;	/* Specified gid; -1 if not to be changed. */

  /* Change the owner (group) of a file only if it has this uid (gid).
     -1 means there's no restriction.  */
  uid_t required_uid = -1;
  gid_t required_gid = -1;

  /* Bit flags that control how fts works.  */
  int bit_flags = FTS_PHYSICAL;

  /* 1 if --dereference, 0 if --no-dereference, -1 if neither has been
     specified.  */
  int dereference = -1;

  struct Chown_option chopt;
  bool ok;
  int optc;

  initialize_main (&argc, &argv);
  set_program_name (argv[0]);
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  chopt_init (&chopt);

  while ((optc = getopt_long (argc, argv, "HLPRcfhv", long_options, NULL))
         != -1)
    {
      switch (optc)
        {
        case 'H': /* Traverse command-line symlinks-to-directories.  */
          bit_flags = FTS_COMFOLLOW | FTS_PHYSICAL;
          break;

        case 'L': /* Traverse all symlinks-to-directories.  */
          bit_flags = FTS_LOGICAL;
          break;

        case 'P': /* Traverse no symlinks-to-directories.  */
          bit_flags = FTS_PHYSICAL;
          break;

        case 'h': /* --no-dereference: affect symlinks */
          dereference = 0;
          break;

        case DEREFERENCE_OPTION: /* --dereference: affect the referent
                                    of each symlink */
          dereference = 1;
          break;

        case NO_PRESERVE_ROOT:
          preserve_root = false;
          break;

        case PRESERVE_ROOT:
          preserve_root = true;
          break;

        case REFERENCE_FILE_OPTION:
          reference_file = optarg;
          break;

        case FROM_OPTION:
          {
            const char *e = parse_user_spec (optarg,
                                             &required_uid, &required_gid,
                                             NULL, NULL);
            if (e)
              error (EXIT_FAILURE, 0, "%s: %s", e, quote (optarg));
            break;
          }

        case 'R':
          chopt.recurse = true;
          break;

        case 'c':
          chopt.verbosity = V_changes_only;
          break;

        case 'f':
          chopt.force_silent = true;
          break;

        case 'v':
          chopt.verbosity = V_high;
          break;

        case_GETOPT_HELP_CHAR;
        case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);
        default:
          usage (EXIT_FAILURE);
        }
    }

  if (chopt.recurse)
    {
      if (bit_flags == FTS_PHYSICAL)
        {
          if (dereference == 1)
            error (EXIT_FAILURE, 0,
                   _("-R --dereference requires either -H or -L"));
          dereference = 0;
        }
    }
  else
    {
      bit_flags = FTS_PHYSICAL;
    }
  chopt.affect_symlink_referent = (dereference != 0);

  if (argc - optind < (reference_file ? 1 : 2))
    {
      if (argc <= optind)
        error (0, 0, _("missing operand"));
      else
        error (0, 0, _("missing operand after %s"), quote (argv[argc - 1]));
      usage (EXIT_FAILURE);
    }

  if (reference_file)
    {
      struct stat ref_stats;
      if (stat (reference_file, &ref_stats))
        error (EXIT_FAILURE, errno, _("failed to get attributes of %s"),
               quoteaf (reference_file));

      uid = ref_stats.st_uid;
      gid = ref_stats.st_gid;
      chopt.user_name = uid_to_name (ref_stats.st_uid);
      chopt.group_name = gid_to_name (ref_stats.st_gid);
    }
  else
    {
      const char *e = parse_user_spec (argv[optind], &uid, &gid,
                                       &chopt.user_name, &chopt.group_name);
      if (e)
        error (EXIT_FAILURE, 0, "%s: %s", e, quote (argv[optind]));

      /* If a group is specified but no user, set the user name to the
         empty string so that diagnostics say "ownership :GROUP"
         rather than "group GROUP".  */
      if (!chopt.user_name && chopt.group_name)
        chopt.user_name = bad_cast ("");

      optind++;
    }

  if (chopt.recurse && preserve_root)
    {
      static struct dev_ino dev_ino_buf;
      chopt.root_dev_ino = get_root_dev_ino (&dev_ino_buf);
      if (chopt.root_dev_ino == NULL)
        error (EXIT_FAILURE, errno, _("failed to get attributes of %s"),
               quoteaf ("/"));
    }

  bit_flags |= FTS_DEFER_STAT;
  ok = chown_files (argv + optind, bit_flags,
                    uid, gid,
                    required_uid, required_gid, &chopt);

  chopt_free (&chopt);

  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
