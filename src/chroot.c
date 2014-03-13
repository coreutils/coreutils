/* chroot -- run command or shell with special root directory
   Copyright (C) 1995-2014 Free Software Foundation, Inc.

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

/* Written by Roland McGrath.  */

#include <config.h>
#include <getopt.h>
#include <stdio.h>
#include <sys/types.h>
#include <grp.h>

#include "system.h"
#include "error.h"
#include "ignore-value.h"
#include "quote.h"
#include "userspec.h"
#include "xstrtol.h"

/* The official name of this program (e.g., no 'g' prefix).  */
#define PROGRAM_NAME "chroot"

#define AUTHORS proper_name ("Roland McGrath")

#ifndef MAXGID
# define MAXGID GID_T_MAX
#endif

enum
{
  GROUPS = UCHAR_MAX + 1,
  USERSPEC
};

static struct option const long_opts[] =
{
  {"groups", required_argument, NULL, GROUPS},
  {"userspec", required_argument, NULL, USERSPEC},
  {GETOPT_HELP_OPTION_DECL},
  {GETOPT_VERSION_OPTION_DECL},
  {NULL, 0, NULL, 0}
};

#if ! HAVE_SETGROUPS
/* At least Interix lacks supplemental group support.  */
static int
setgroups (size_t size _GL_UNUSED, gid_t const *list _GL_UNUSED)
{
  errno = ENOTSUP;
  return -1;
}
#endif

/* Determine the group IDs for the specified supplementary GROUPS,
   which is a comma separated list of supplementary groups (names or numbers).
   Allocate an array for the parsed IDs and store it in PGIDS,
   which may be allocated even on parse failure.
   Update the number of parsed groups in PN_GIDS on success.
   Upon any failure return nonzero, and issue diagnostic if SHOW_ERRORS is true.
   Otherwise return zero.  */

static int
parse_additional_groups (char const *groups, GETGROUPS_T **pgids,
                         size_t *pn_gids, bool show_errors)
{
  GETGROUPS_T *gids = NULL;
  size_t n_gids_allocated = 0;
  size_t n_gids = 0;
  char *buffer = xstrdup (groups);
  char const *tmp;
  int ret = 0;

  for (tmp = strtok (buffer, ","); tmp; tmp = strtok (NULL, ","))
    {
      struct group *g;
      unsigned long int value;

      if (xstrtoul (tmp, NULL, 10, &value, "") == LONGINT_OK && value <= MAXGID)
        {
          while (isspace (to_uchar (*tmp)))
            tmp++;
          if (*tmp != '+')
            {
              /* Handle the case where the name is numeric.  */
              g = getgrnam (tmp);
              if (g != NULL)
                value = g->gr_gid;
            }
          g = (struct group *)! NULL;  /* We've got a group from the number.  */
        }
      else
        {
          g = getgrnam (tmp);
          if (g != NULL)
            value = g->gr_gid;
        }

      if (g == NULL)
        {
          ret = -1;

          if (show_errors)
            {
              error (0, errno, _("invalid group %s"), quote (tmp));
              continue;
            }

          break;
        }

      if (n_gids == n_gids_allocated)
        gids = X2NREALLOC (gids, &n_gids_allocated);
      gids[n_gids++] = value;
    }

  if (ret == 0 && n_gids == 0)
    {
      if (show_errors)
        error (0, 0, _("invalid group list %s"), quote (groups));
      ret = -1;
    }

  *pgids = gids;

  if (ret == 0)
    *pn_gids = n_gids;

  free (buffer);
  return ret;
}

void
usage (int status)
{
  if (status != EXIT_SUCCESS)
    emit_try_help ();
  else
    {
      printf (_("\
Usage: %s [OPTION] NEWROOT [COMMAND [ARG]...]\n\
  or:  %s OPTION\n\
"), program_name, program_name);

      fputs (_("\
Run COMMAND with root directory set to NEWROOT.\n\
\n\
"), stdout);

      fputs (_("\
  --userspec=USER:GROUP  specify user and group (ID or name) to use\n\
  --groups=G_LIST        specify supplementary groups as g1,g2,..,gN\n\
"), stdout);

      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      fputs (_("\
\n\
If no command is given, run '${SHELL} -i' (default: '/bin/sh -i').\n\
"), stdout);
      emit_ancillary_info ();
    }
  exit (status);
}

int
main (int argc, char **argv)
{
  int c;

  /* Input user and groups spec.  */
  char const *userspec = NULL;
  char const *groups = NULL;

  /* Parsed user and group IDs.  */
  uid_t uid = -1;
  gid_t gid = -1;
  GETGROUPS_T *out_gids = NULL;
  size_t n_gids = 0;

  initialize_main (&argc, &argv);
  set_program_name (argv[0]);
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  initialize_exit_failure (EXIT_CANCELED);
  atexit (close_stdout);

  while ((c = getopt_long (argc, argv, "+", long_opts, NULL)) != -1)
    {
      switch (c)
        {
        case USERSPEC:
          userspec = optarg;
          break;

        case GROUPS:
          groups = optarg;
          break;

        case_GETOPT_HELP_CHAR;

        case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);

        default:
          usage (EXIT_CANCELED);
        }
    }

  if (argc <= optind)
    {
      error (0, 0, _("missing operand"));
      usage (EXIT_CANCELED);
    }

  /* We have to look up users and groups twice.
     - First, outside the chroot to load potentially necessary passwd/group
       parsing plugins (e.g. NSS);
     - Second, inside chroot to redo the parsing in case IDs are different.  */
  if (userspec)
    ignore_value (parse_user_spec (userspec, &uid, &gid, NULL, NULL));
  if (groups && *groups)
    ignore_value (parse_additional_groups (groups, &out_gids, &n_gids, false));

  if (chroot (argv[optind]) != 0)
    error (EXIT_CANCELED, errno, _("cannot change root directory to %s"),
           argv[optind]);

  if (chdir ("/"))
    error (EXIT_CANCELED, errno, _("cannot chdir to root directory"));

  if (argc == optind + 1)
    {
      /* No command.  Run an interactive shell.  */
      char *shell = getenv ("SHELL");
      if (shell == NULL)
        shell = bad_cast ("/bin/sh");
      argv[0] = shell;
      argv[1] = bad_cast ("-i");
      argv[2] = NULL;
    }
  else
    {
      /* The following arguments give the command.  */
      argv += optind + 1;
    }

  bool fail = false;

  /* Attempt to set all three: supplementary groups, group ID, user ID.
     Diagnose any failures.  If any have failed, exit before execvp.  */
  if (userspec)
    {
      char const *err = parse_user_spec (userspec, &uid, &gid, NULL, NULL);

      if (err && uid == -1 && gid == -1)
        error (EXIT_CANCELED, errno, "%s", err);
    }

  GETGROUPS_T *gids = out_gids;
  GETGROUPS_T *in_gids = NULL;
  if (groups && *groups)
    {
      if (parse_additional_groups (groups, &in_gids, &n_gids, !n_gids) != 0)
        {
          /* If look-up outside the chroot worked, then go with those gids.  */
          if (! n_gids)
            fail = true;
        }
      else
        gids = in_gids;
    }

  if (gids && setgroups (n_gids, gids) != 0)
    {
      error (0, errno, _("failed to set additional groups"));
      fail = true;
    }

  free (in_gids);
  free (out_gids);

  if (gid != (gid_t) -1 && setgid (gid))
    {
      error (0, errno, _("failed to set group-ID"));
      fail = true;
    }

  if (uid != (uid_t) -1 && setuid (uid))
    {
      error (0, errno, _("failed to set user-ID"));
      fail = true;
    }

  if (fail)
    exit (EXIT_CANCELED);

  /* Execute the given command.  */
  execvp (argv[0], argv);

  {
    int exit_status = (errno == ENOENT ? EXIT_ENOENT : EXIT_CANNOT_INVOKE);
    error (0, errno, _("failed to run command %s"), quote (argv[0]));
    exit (exit_status);
  }
}
