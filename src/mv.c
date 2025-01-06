/* mv -- move or rename files
   Copyright (C) 1986-2025 Free Software Foundation, Inc.

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

/* Written by Mike Parker, David MacKenzie, and Jim Meyering */

#include <config.h>
#include <stdio.h>
#include <getopt.h>
#include <sys/types.h>
#include <selinux/label.h>

#include "system.h"
#include "argmatch.h"
#include "assure.h"
#include "backupfile.h"
#include "copy.h"
#include "cp-hash.h"
#include "filenamecat.h"
#include "remove.h"
#include "renameatu.h"
#include "root-dev-ino.h"
#include "targetdir.h"
#include "priv-set.h"

/* The official name of this program (e.g., no 'g' prefix).  */
#define PROGRAM_NAME "mv"

#define AUTHORS \
  proper_name ("Mike Parker"), \
  proper_name ("David MacKenzie"), \
  proper_name ("Jim Meyering")

/* For long options that have no equivalent short option, use a
   non-character as a pseudo short option, starting with CHAR_MAX + 1.  */
enum
{
  DEBUG_OPTION = CHAR_MAX + 1,
  EXCHANGE_OPTION,
  NO_COPY_OPTION,
  STRIP_TRAILING_SLASHES_OPTION
};

static char const *const update_type_string[] =
{
  "all", "none", "none-fail", "older", nullptr
};
static enum Update_type const update_type[] =
{
  UPDATE_ALL, UPDATE_NONE, UPDATE_NONE_FAIL, UPDATE_OLDER,
};
ARGMATCH_VERIFY (update_type_string, update_type);

static struct option const long_options[] =
{
  {"backup", optional_argument, nullptr, 'b'},
  {"context", no_argument, nullptr, 'Z'},
  {"debug", no_argument, nullptr, DEBUG_OPTION},
  {"exchange", no_argument, nullptr, EXCHANGE_OPTION},
  {"force", no_argument, nullptr, 'f'},
  {"interactive", no_argument, nullptr, 'i'},
  {"no-clobber", no_argument, nullptr, 'n'},   /* Deprecated.  */
  {"no-copy", no_argument, nullptr, NO_COPY_OPTION},
  {"no-target-directory", no_argument, nullptr, 'T'},
  {"strip-trailing-slashes", no_argument, nullptr,
   STRIP_TRAILING_SLASHES_OPTION},
  {"suffix", required_argument, nullptr, 'S'},
  {"target-directory", required_argument, nullptr, 't'},
  {"update", optional_argument, nullptr, 'u'},
  {"verbose", no_argument, nullptr, 'v'},
  {GETOPT_HELP_OPTION_DECL},
  {GETOPT_VERSION_OPTION_DECL},
  {nullptr, 0, nullptr, 0}
};

static void
rm_option_init (struct rm_options *x)
{
  x->ignore_missing_files = false;
  x->remove_empty_directories = true;
  x->recursive = true;
  x->one_file_system = false;

  /* Should we prompt for removal, too?  No.  Prompting for the 'move'
     part is enough.  It implies removal.  */
  x->interactive = RMI_NEVER;
  x->stdin_tty = false;

  x->verbose = false;

  /* Since this program may well have to process additional command
     line arguments after any call to 'rm', that function must preserve
     the initial working directory, in case one of those is a
     '.'-relative name.  */
  x->require_restore_cwd = true;

  {
    static struct dev_ino dev_ino_buf;
    x->root_dev_ino = get_root_dev_ino (&dev_ino_buf);
    if (x->root_dev_ino == nullptr)
      error (EXIT_FAILURE, errno, _("failed to get attributes of %s"),
             quoteaf ("/"));
  }

  x->preserve_all_root = false;
}

static void
cp_option_init (struct cp_options *x)
{
  bool selinux_enabled = (0 < is_selinux_enabled ());

  cp_options_default (x);
  x->copy_as_regular = false;  /* FIXME: maybe make this an option */
  x->reflink_mode = REFLINK_AUTO;
  x->dereference = DEREF_NEVER;
  x->unlink_dest_before_opening = false;
  x->unlink_dest_after_failed_open = false;
  x->hard_link = false;
  x->interactive = I_UNSPECIFIED;
  x->move_mode = true;
  x->install_mode = false;
  x->one_file_system = false;
  x->preserve_ownership = true;
  x->preserve_links = true;
  x->preserve_mode = true;
  x->preserve_timestamps = true;
  x->explicit_no_preserve_mode= false;
  x->preserve_security_context = selinux_enabled;
  x->set_security_context = nullptr;
  x->reduce_diagnostics = false;
  x->data_copy_required = true;
  x->require_preserve = false;  /* FIXME: maybe make this an option */
  x->require_preserve_context = false;
  x->preserve_xattr = true;
  x->require_preserve_xattr = false;
  x->recursive = true;
  x->sparse_mode = SPARSE_AUTO;  /* FIXME: maybe make this an option */
  x->symbolic_link = false;
  x->set_mode = false;
  x->mode = 0;
  x->stdin_tty = isatty (STDIN_FILENO);

  x->open_dangling_dest_symlink = false;
  x->update = UPDATE_ALL;
  x->verbose = false;
  x->dest_info = nullptr;
  x->src_info = nullptr;
}

/* Move SOURCE onto DEST aka DEST_DIRFD+DEST_RELNAME.
   Handle cross-file-system moves.
   If SOURCE is a directory, DEST must not exist.
   Return true if successful.  */

static bool
do_move (char const *source, char const *dest,
         int dest_dirfd, char const *dest_relname, const struct cp_options *x)
{
  bool copy_into_self;
  bool rename_succeeded;
  bool ok = copy (source, dest, dest_dirfd, dest_relname, 0, x,
                  &copy_into_self, &rename_succeeded);

  if (ok)
    {
      char const *dir_to_remove;
      if (copy_into_self)
        {
          /* In general, when copy returns with copy_into_self set, SOURCE is
             the same as, or a parent of DEST.  In this case we know it's a
             parent.  It doesn't make sense to move a directory into itself, and
             besides in some situations doing so would give highly unintuitive
             results.  Run this 'mkdir b; touch a c; mv * b' in an empty
             directory.  Here's the result of running echo $(find b -print):
             b b/a b/b b/b/a b/c.  Notice that only file 'a' was copied
             into b/b.  Handle this by giving a diagnostic, removing the
             copied-into-self directory, DEST ('b/b' in the example),
             and failing.  */

          dir_to_remove = nullptr;
          ok = false;
        }
      else if (rename_succeeded)
        {
          /* No need to remove anything.  SOURCE was successfully
             renamed to DEST.  Or the user declined to rename a file.  */
          dir_to_remove = nullptr;
        }
      else
        {
          /* This may mean SOURCE and DEST referred to different devices.
             It may also conceivably mean that even though they referred
             to the same device, rename wasn't implemented for that device.

             E.g., (from Joel N. Weber),
             [...] there might someday be cases where you can't rename
             but you can copy where the device name is the same, especially
             on Hurd.  Consider an ftpfs with a primitive ftp server that
             supports uploading, downloading and deleting, but not renaming.

             Also, note that comparing device numbers is not a reliable
             check for 'can-rename'.  Some systems can be set up so that
             files from many different physical devices all have the same
             st_dev field.  This is a feature of some NFS mounting
             configurations.

             We reach this point if SOURCE has been successfully copied
             to DEST.  Now we have to remove SOURCE.

             This function used to resort to copying only when rename
             failed and set errno to EXDEV.  */

          dir_to_remove = source;
        }

      if (dir_to_remove != nullptr)
        {
          struct rm_options rm_options;
          enum RM_status status;
          char const *dir[2];

          rm_option_init (&rm_options);
          rm_options.verbose = x->verbose;
          dir[0] = dir_to_remove;
          dir[1] = nullptr;

          status = rm ((void *) dir, &rm_options);
          affirm (VALID_STATUS (status));
          if (status == RM_ERROR)
            ok = false;
        }
    }

  return ok;
}

void
usage (int status)
{
  if (status != EXIT_SUCCESS)
    emit_try_help ();
  else
    {
      printf (_("\
Usage: %s [OPTION]... [-T] SOURCE DEST\n\
  or:  %s [OPTION]... SOURCE... DIRECTORY\n\
  or:  %s [OPTION]... -t DIRECTORY SOURCE...\n\
"),
              program_name, program_name, program_name);
      fputs (_("\
Rename SOURCE to DEST, or move SOURCE(s) to DIRECTORY.\n\
"), stdout);

      emit_mandatory_arg_note ();

      fputs (_("\
      --backup[=CONTROL]       make a backup of each existing destination file\
\n\
  -b                           like --backup but does not accept an argument\n\
"), stdout);
      fputs (_("\
      --debug                  explain how a file is copied.  Implies -v\n\
"), stdout);
      fputs (_("\
      --exchange               exchange source and destination\n\
"), stdout);
      fputs (_("\
  -f, --force                  do not prompt before overwriting\n\
  -i, --interactive            prompt before overwrite\n\
  -n, --no-clobber             do not overwrite an existing file\n\
If you specify more than one of -i, -f, -n, only the final one takes effect.\n\
"), stdout);
      fputs (_("\
      --no-copy                do not copy if renaming fails\n\
      --strip-trailing-slashes  remove any trailing slashes from each SOURCE\n\
                                 argument\n\
  -S, --suffix=SUFFIX          override the usual backup suffix\n\
"), stdout);
      fputs (_("\
  -t, --target-directory=DIRECTORY  move all SOURCE arguments into DIRECTORY\n\
  -T, --no-target-directory    treat DEST as a normal file\n\
"), stdout);
      fputs (_("\
      --update[=UPDATE]        control which existing files are updated;\n\
                                 UPDATE={all,none,none-fail,older(default)}\n\
  -u                           equivalent to --update[=older].  See below\n\
"), stdout);
      fputs (_("\
  -v, --verbose                explain what is being done\n\
  -Z, --context                set SELinux security context of destination\n\
                                 file to default type\n\
"), stdout);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      emit_update_parameters_note ();
      emit_backup_suffix_note ();
      emit_ancillary_info (PROGRAM_NAME);
    }
  exit (status);
}

int
main (int argc, char **argv)
{
  int c;
  bool ok;
  bool make_backups = false;
  char const *backup_suffix = nullptr;
  char *version_control_string = nullptr;
  struct cp_options x;
  bool remove_trailing_slashes = false;
  char const *target_directory = nullptr;
  bool no_target_directory = false;
  int n_files;
  char **file;
  bool selinux_enabled = (0 < is_selinux_enabled ());

  initialize_main (&argc, &argv);
  set_program_name (argv[0]);
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdin);

  cp_option_init (&x);

  /* Try to disable the ability to unlink a directory.  */
  priv_set_remove_linkdir ();

  while ((c = getopt_long (argc, argv, "bfint:uvS:TZ", long_options, nullptr))
         != -1)
    {
      switch (c)
        {
        case 'b':
          make_backups = true;
          if (optarg)
            version_control_string = optarg;
          break;
        case 'f':
          x.interactive = I_ALWAYS_YES;
          break;
        case 'i':
          x.interactive = I_ASK_USER;
          break;
        case 'n':
          x.interactive = I_ALWAYS_SKIP;
          break;
        case DEBUG_OPTION:
          x.debug = x.verbose = true;
          break;
        case EXCHANGE_OPTION:
          x.exchange = true;
          break;
        case NO_COPY_OPTION:
          x.no_copy = true;
          break;
        case STRIP_TRAILING_SLASHES_OPTION:
          remove_trailing_slashes = true;
          break;
        case 't':
          if (target_directory)
            error (EXIT_FAILURE, 0, _("multiple target directories specified"));
          target_directory = optarg;
          break;
        case 'T':
          no_target_directory = true;
          break;
        case 'u':
          x.update = UPDATE_OLDER;
          if (optarg)
            x.update = XARGMATCH ("--update", optarg,
                                  update_type_string, update_type);
          break;
        case 'v':
          x.verbose = true;
          break;
        case 'S':
          make_backups = true;
          backup_suffix = optarg;
          break;
        case 'Z':
          /* As a performance enhancement, don't even bother trying
             to "restorecon" when not on an selinux-enabled kernel.  */
          if (selinux_enabled)
            {
              x.preserve_security_context = false;
              x.set_security_context = selabel_open (SELABEL_CTX_FILE,
                                                     nullptr, 0);
              if (! x.set_security_context)
                error (0, errno, _("warning: ignoring --context"));
            }
          break;
        case_GETOPT_HELP_CHAR;
        case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);
        default:
          usage (EXIT_FAILURE);
        }
    }

  n_files = argc - optind;
  file = argv + optind;

  if (n_files <= !target_directory)
    {
      if (n_files <= 0)
        error (0, 0, _("missing file operand"));
      else
        error (0, 0, _("missing destination file operand after %s"),
               quoteaf (file[0]));
      usage (EXIT_FAILURE);
    }

  struct stat sb;
  sb.st_mode = 0;
  int target_dirfd = AT_FDCWD;
  if (no_target_directory)
    {
      if (target_directory)
        error (EXIT_FAILURE, 0,
               _("cannot combine --target-directory (-t) "
                 "and --no-target-directory (-T)"));
      if (2 < n_files)
        {
          error (0, 0, _("extra operand %s"), quoteaf (file[2]));
          usage (EXIT_FAILURE);
        }
    }
  else if (target_directory)
    {
      target_dirfd = target_directory_operand (target_directory, &sb);
      if (! target_dirfd_valid (target_dirfd))
        error (EXIT_FAILURE, errno, _("target directory %s"),
               quoteaf (target_directory));
    }
  else
    {
      char const *lastfile = file[n_files - 1];
      if (n_files == 2 && !x.exchange)
        x.rename_errno = (renameatu (AT_FDCWD, file[0], AT_FDCWD, lastfile,
                                     RENAME_NOREPLACE)
                          ? errno : 0);
      if (x.rename_errno != 0)
        {
          int fd = target_directory_operand (lastfile, &sb);
          if (target_dirfd_valid (fd))
            {
              x.rename_errno = -1;
              target_dirfd = fd;
              target_directory = lastfile;
              n_files--;
            }
          else
            {
              /* The last operand LASTFILE cannot be opened as a directory.
                 If there are more than two operands, report an error.

                 Also, report an error if LASTFILE is known to be a directory
                 even though it could not be opened, which can happen if
                 opening failed with EACCES on a platform lacking O_PATH.
                 In this case use stat to test whether LASTFILE is a
                 directory, in case opening a non-directory with (O_SEARCH
                 | O_DIRECTORY) failed with EACCES not ENOTDIR.  */
              int err = errno;
              if (2 < n_files
                  || (O_PATHSEARCH == O_SEARCH && err == EACCES
                      && (sb.st_mode != 0 || stat (lastfile, &sb) == 0)
                      && S_ISDIR (sb.st_mode)))
                error (EXIT_FAILURE, err, _("target %s"), quoteaf (lastfile));
            }
        }
    }

  /* Handle the ambiguity in the semantics of mv induced by the
     varying semantics of the rename function.  POSIX-compatible
     systems (e.g., GNU/Linux) have a rename function that honors a
     trailing slash in the source, while others (Solaris 9, FreeBSD
     7.2) have a rename function that ignores it.  */
  if (remove_trailing_slashes)
    for (int i = 0; i < n_files; i++)
      strip_trailing_slashes (file[i]);

  if (x.interactive == I_ALWAYS_SKIP)
    x.update = UPDATE_NONE;

  if (make_backups
      && (x.exchange
          || x.update == UPDATE_NONE
          || x.update == UPDATE_NONE_FAIL))
    {
      error (0, 0,
             _("cannot combine --backup with "
               "--exchange, -n, or --update=none-fail"));
      usage (EXIT_FAILURE);
    }

  x.backup_type = (make_backups
                   ? xget_version (_("backup type"),
                                   version_control_string)
                   : no_backups);
  set_simple_backup_suffix (backup_suffix);

  hash_init ();

  if (target_directory)
    {
      /* Initialize the hash table only if we'll need it.
         The problem it is used to detect can arise only if there are
         two or more files to move.  */
      if (2 <= n_files)
        dest_info_init (&x);

      ok = true;
      for (int i = 0; i < n_files; ++i)
        {
          x.last_file = i + 1 == n_files;
          char const *source = file[i];
          char const *source_basename = last_component (source);
          char *dest_relname;
          char *dest = file_name_concat (target_directory, source_basename,
                                         &dest_relname);
          strip_trailing_slashes (dest_relname);
          ok &= do_move (source, dest, target_dirfd, dest_relname, &x);
          free (dest);
        }
    }
  else
    {
      x.last_file = true;
      ok = do_move (file[0], file[1], AT_FDCWD, file[1], &x);
    }

  main_exit (ok ? EXIT_SUCCESS : EXIT_FAILURE);
}
