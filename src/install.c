/* install - copy files and set attributes
   Copyright (C) 1989-2025 Free Software Foundation, Inc.

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

/* Written by David MacKenzie <djm@gnu.ai.mit.edu> */

#include <config.h>
#include <stdio.h>
#include <getopt.h>
#include <sys/types.h>
#include <signal.h>
#include <pwd.h>
#include <grp.h>
#include <selinux/label.h>
#include <spawn.h>
#include <sys/wait.h>

#include "system.h"
#include "backupfile.h"
#include "cp-hash.h"
#include "copy.h"
#include "filenamecat.h"
#include "full-read.h"
#include "mkancesdirs.h"
#include "mkdir-p.h"
#include "modechange.h"
#include "prog-fprintf.h"
#include "quote.h"
#include "savewd.h"
#include "selinux.h"
#include "stat-time.h"
#include "targetdir.h"
#include "utimens.h"
#include "xstrtol.h"

/* The official name of this program (e.g., no 'g' prefix).  */
#define PROGRAM_NAME "install"

#define AUTHORS proper_name ("David MacKenzie")

static int selinux_enabled = 0;
static bool use_default_selinux_context = true;

#if ! HAVE_ENDGRENT
# define endgrent() ((void) 0)
#endif

#if ! HAVE_ENDPWENT
# define endpwent() ((void) 0)
#endif

/* The user name that will own the files, or nullptr to make the owner
   the current user ID. */
static char *owner_name;

/* The user ID corresponding to 'owner_name'. */
static uid_t owner_id;

/* The group name that will own the files, or nullptr to make the group
   the current group ID. */
static char *group_name;

/* The group ID corresponding to 'group_name'. */
static gid_t group_id;

#define DEFAULT_MODE (S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)

/* The file mode bits to which non-directory files will be set.  The umask has
   no effect. */
static mode_t mode = DEFAULT_MODE;

/* Similar, but for directories.  */
static mode_t dir_mode = DEFAULT_MODE;

/* The file mode bits that the user cares about.  This should be a
   superset of DIR_MODE and a subset of CHMOD_MODE_BITS.  This matters
   for directories, since otherwise directories may keep their S_ISUID
   or S_ISGID bits.  */
static mode_t dir_mode_bits = CHMOD_MODE_BITS;

/* Compare files before installing (-C) */
static bool copy_only_if_needed;

/* If true, strip executable files after copying them. */
static bool strip_files;

/* If true, install a directory instead of a regular file. */
static bool dir_arg;

/* Program used to strip binaries, "strip" is default */
static char const *strip_program = "strip";

/* For long options that have no equivalent short option, use a
   non-character as a pseudo short option, starting with CHAR_MAX + 1.  */
enum
{
  DEBUG_OPTION = CHAR_MAX + 1,
  PRESERVE_CONTEXT_OPTION,
  STRIP_PROGRAM_OPTION
};

static struct option const long_options[] =
{
  {"backup", optional_argument, nullptr, 'b'},
  {"compare", no_argument, nullptr, 'C'},
  {GETOPT_SELINUX_CONTEXT_OPTION_DECL},
  {"debug", no_argument, nullptr, DEBUG_OPTION},
  {"directory", no_argument, nullptr, 'd'},
  {"group", required_argument, nullptr, 'g'},
  {"mode", required_argument, nullptr, 'm'},
  {"no-target-directory", no_argument, nullptr, 'T'},
  {"owner", required_argument, nullptr, 'o'},
  {"preserve-timestamps", no_argument, nullptr, 'p'},
  {"preserve-context", no_argument, nullptr, PRESERVE_CONTEXT_OPTION},
  {"strip", no_argument, nullptr, 's'},
  {"strip-program", required_argument, nullptr, STRIP_PROGRAM_OPTION},
  {"suffix", required_argument, nullptr, 'S'},
  {"target-directory", required_argument, nullptr, 't'},
  {"verbose", no_argument, nullptr, 'v'},
  {GETOPT_HELP_OPTION_DECL},
  {GETOPT_VERSION_OPTION_DECL},
  {nullptr, 0, nullptr, 0}
};

/* Compare content of opened files using file descriptors A_FD and B_FD. Return
   true if files are equal. */
static bool
have_same_content (int a_fd, int b_fd)
{
  enum { CMP_BLOCK_SIZE = 4096 };
  static char a_buff[CMP_BLOCK_SIZE];
  static char b_buff[CMP_BLOCK_SIZE];

  idx_t size;
  while (0 < (size = full_read (a_fd, a_buff, sizeof a_buff))) {
    if (size != full_read (b_fd, b_buff, sizeof b_buff))
      return false;

    if (!memeq (a_buff, b_buff, size))
      return false;
  }

  return size == 0;
}

/* Return true for mode with non-permission bits. */
static bool
extra_mode (mode_t input)
{
  mode_t mask = S_IRWXUGO | S_IFMT;
  return !! (input & ~ mask);
}

/* Return true if copy of file SRC_NAME to file DEST_NAME aka
   DEST_DIRFD+DEST_RELNAME is necessary. */
static bool
need_copy (char const *src_name, char const *dest_name,
           int dest_dirfd, char const *dest_relname,
           const struct cp_options *x)
{
  struct stat src_sb, dest_sb;
  int src_fd, dest_fd;
  bool content_match;

  if (extra_mode (mode))
    return true;

  /* compare files using stat */
  if (stat (src_name, &src_sb) != 0)
    return true;

  if (fstatat (dest_dirfd, dest_relname, &dest_sb, AT_SYMLINK_NOFOLLOW) != 0)
    return true;

  if (!S_ISREG (src_sb.st_mode) || !S_ISREG (dest_sb.st_mode)
      || extra_mode (src_sb.st_mode) || extra_mode (dest_sb.st_mode))
    return true;

  if (src_sb.st_size != dest_sb.st_size
      || (dest_sb.st_mode & CHMOD_MODE_BITS) != mode)
    return true;

  if (owner_id == (uid_t) -1)
    {
      errno = 0;
      uid_t ruid = getuid ();
      if ((ruid == (uid_t) -1 && errno) || dest_sb.st_uid != ruid)
        return true;
    }
  else if (dest_sb.st_uid != owner_id)
    return true;

  if (group_id == (uid_t) -1)
    {
      errno = 0;
      gid_t rgid = getgid ();
      if ((rgid == (uid_t) -1 && errno) || dest_sb.st_gid != rgid)
        return true;
    }
  else if (dest_sb.st_gid != group_id)
    return true;

  /* compare SELinux context if preserving */
  if (selinux_enabled && x->preserve_security_context)
    {
      char *file_scontext_raw = nullptr;
      char *to_scontext_raw = nullptr;
      bool scontext_match;

      if (getfilecon_raw (src_name, &file_scontext_raw) == -1)
        return true;

      if (getfilecon_raw (dest_name, &to_scontext_raw) == -1)
        {
          freecon (file_scontext_raw);
          return true;
        }

      scontext_match = streq (file_scontext_raw, to_scontext_raw);

      freecon (file_scontext_raw);
      freecon (to_scontext_raw);
      if (!scontext_match)
        return true;
    }

  /* compare files content */
  src_fd = open (src_name, O_RDONLY | O_BINARY);
  if (src_fd < 0)
    return true;

  dest_fd = openat (dest_dirfd, dest_relname, O_RDONLY | O_BINARY);
  if (dest_fd < 0)
    {
      close (src_fd);
      return true;
    }

  content_match = have_same_content (src_fd, dest_fd);

  close (src_fd);
  close (dest_fd);
  return !content_match;
}

static void
cp_option_init (struct cp_options *x)
{
  cp_options_default (x);
  x->copy_as_regular = true;
  x->reflink_mode = REFLINK_AUTO;
  x->dereference = DEREF_ALWAYS;
  x->unlink_dest_before_opening = true;
  x->unlink_dest_after_failed_open = false;
  x->hard_link = false;
  x->interactive = I_UNSPECIFIED;
  x->move_mode = false;
  x->install_mode = true;
  x->one_file_system = false;
  x->preserve_ownership = false;
  x->preserve_links = false;
  x->preserve_mode = false;
  x->preserve_timestamps = false;
  x->explicit_no_preserve_mode = false;
  x->reduce_diagnostics=false;
  x->data_copy_required = true;
  x->require_preserve = false;
  x->require_preserve_xattr = false;
  x->recursive = false;
  x->sparse_mode = SPARSE_AUTO;
  x->symbolic_link = false;
  x->backup_type = no_backups;

  /* Create destination files initially writable so we can run strip on them.
     Although GNU strip works fine on read-only files, some others
     would fail.  */
  x->set_mode = true;
  x->mode = S_IRUSR | S_IWUSR;
  x->stdin_tty = false;

  x->open_dangling_dest_symlink = false;
  x->update = UPDATE_ALL;
  x->require_preserve_context = false;  /* Not used by install currently.  */
  x->preserve_security_context = false; /* Whether to copy context from src.  */
  x->set_security_context = nullptr; /* Whether to set sys default context.  */
  x->preserve_xattr = false;
  x->verbose = false;
  x->dest_info = nullptr;
  x->src_info = nullptr;
}

static struct selabel_handle *
get_labeling_handle (void)
{
  static bool initialized;
  static struct selabel_handle *hnd;
  if (!initialized)
    {
      initialized = true;
      hnd = selabel_open (SELABEL_CTX_FILE, nullptr, 0);
      if (!hnd)
        error (0, errno, _("warning: security labeling handle failed"));
    }
  return hnd;
}

/* Modify file context to match the specified policy.
   If an error occurs the file will remain with the default directory
   context.  Note this sets the context to that returned by selabel_lookup
   and thus discards MLS levels and user identity of the FILE.  */
static void
setdefaultfilecon (char const *file)
{
  struct stat st;
  char *scontext_raw = nullptr;

  if (selinux_enabled != 1)
    {
      /* Indicate no context found. */
      return;
    }
  if (lstat (file, &st) != 0)
    return;

  struct selabel_handle *hnd = get_labeling_handle ();
  if (!hnd)
    return;
  if (selabel_lookup_raw (hnd, &scontext_raw, file, st.st_mode) != 0)
    {
      if (errno != ENOENT && ! ignorable_ctx_err (errno))
        error (0, errno, _("warning: %s: context lookup failed"),
               quotef (file));
      return;
    }

  if (lsetfilecon_raw (file, scontext_raw) < 0 && errno != ENOTSUP)
    error (0, errno,
           _("warning: %s: failed to change context to %s"),
           quotef_n (0, file), quote_n (1, scontext_raw));

  freecon (scontext_raw);
}

/* Report that directory DIR was made, if OPTIONS requests this.  */
static void
announce_mkdir (char const *dir, void *options)
{
  struct cp_options const *x = options;
  if (x->verbose)
    prog_fprintf (stdout, _("creating directory %s"), quoteaf (dir));
}

/* Make ancestor directory DIR, whose last file name component is
   COMPONENT, with options OPTIONS.  Assume the working directory is
   COMPONENT's parent.  */
static int
make_ancestor (char const *dir, char const *component, void *options)
{
  struct cp_options const *x = options;
  if (x->set_security_context
      && defaultcon (x->set_security_context, component, S_IFDIR) < 0
      && ! ignorable_ctx_err (errno))
    error (0, errno, _("failed to set default creation context for %s"),
           quoteaf (dir));

  int r = mkdir (component, DEFAULT_MODE);
  if (r == 0)
    announce_mkdir (dir, options);
  return r;
}

/* Process a command-line file name, for the -d option.  */
static int
process_dir (char *dir, struct savewd *wd, void *options)
{
  struct cp_options const *x = options;

  int ret = (make_dir_parents (dir, wd, make_ancestor, options,
                               dir_mode, announce_mkdir,
                               dir_mode_bits, owner_id, group_id, false)
          ? EXIT_SUCCESS
          : EXIT_FAILURE);

  /* FIXME: Due to the current structure of make_dir_parents()
     we don't have the facility to call defaultcon() before the
     final component of DIR is created.  So for now, create the
     final component with the context from previous component
     and here we set the context for the final component. */
  if (ret == EXIT_SUCCESS && x->set_security_context)
    {
      if (! restorecon (x->set_security_context, last_component (dir), false)
          && ! ignorable_ctx_err (errno))
        error (0, errno, _("failed to restore context for %s"),
               quoteaf (dir));
    }

  return ret;
}

/* Copy file FROM onto file TO aka TO_DIRFD+TO_RELNAME, creating TO if
   necessary.  Return true if successful.  */

static bool
copy_file (char const *from, char const *to,
           int to_dirfd, char const *to_relname, const struct cp_options *x)
{
  bool copy_into_self;

  if (copy_only_if_needed && !need_copy (from, to, to_dirfd, to_relname, x))
    return true;

  /* Allow installing from non-regular files like /dev/null.
     Charles Karney reported that some Sun version of install allows that
     and that sendmail's installation process relies on the behavior.
     However, since !x->recursive, the call to "copy" will fail if FROM
     is a directory.  */

  return copy (from, to, to_dirfd, to_relname, 0, x, &copy_into_self, nullptr);
}

/* Set the attributes of file or directory NAME aka DIRFD+RELNAME.
   Return true if successful.  */

static bool
change_attributes (char const *name, int dirfd, char const *relname)
{
  bool ok = false;
  /* chown must precede chmod because on some systems,
     chown clears the set[ug]id bits for non-superusers,
     resulting in incorrect permissions.
     On System V, users can give away files with chown and then not
     be able to chmod them.  So don't give files away.

     We don't normally ignore errors from chown because the idea of
     the install command is that the file is supposed to end up with
     precisely the attributes that the user specified (or defaulted).
     If the file doesn't end up with the group they asked for, they'll
     want to know.  */

  if (! (owner_id == (uid_t) -1 && group_id == (gid_t) -1)
      && lchownat (dirfd, relname, owner_id, group_id) != 0)
    error (0, errno, _("cannot change ownership of %s"), quoteaf (name));
  else if (chmodat (dirfd, relname, mode) != 0)
    error (0, errno, _("cannot change permissions of %s"), quoteaf (name));
  else
    ok = true;

  if (use_default_selinux_context)
    setdefaultfilecon (name);

  return ok;
}

/* Set the timestamps of file DEST aka DIRFD+RELNAME to match those of SRC_SB.
   Return true if successful.  */

static bool
change_timestamps (struct stat const *src_sb, char const *dest,
                   int dirfd, char const *relname)
{
  struct timespec timespec[2];
  timespec[0] = get_stat_atime (src_sb);
  timespec[1] = get_stat_mtime (src_sb);

  if (utimensat (dirfd, relname, timespec, 0))
    {
      error (0, errno, _("cannot set timestamps for %s"), quoteaf (dest));
      return false;
    }
  return true;
}

/* Strip the symbol table from the file NAME.
   We could dig the magic number out of the file first to
   determine whether to strip it, but the header files and
   magic numbers vary so much from system to system that making
   it portable would be very difficult.  Not worth the effort. */

static bool
strip (char const *name)
{
  posix_spawnattr_t attr;
  posix_spawnattr_t *attrp = nullptr;

  /* Try to use vfork for systems where it matters.  */
  if (posix_spawnattr_init (&attr) == 0)
    {
      if (posix_spawnattr_setflags (&attr, POSIX_SPAWN_USEVFORK) == 0)
        attrp = &attr;
      else
        posix_spawnattr_destroy (&attr);
    }

  /* Construct the arguments to 'strip'.  */
  char *concat_name = nullptr;
  char const *safe_name = name;
  if (name && *name == '-')
    safe_name = concat_name = file_name_concat (".", name, nullptr);
  char const *const argv[] = { strip_program, safe_name, nullptr };

  /* Run 'strip'.  */
  pid_t pid;
  int result = posix_spawnp (&pid, strip_program, nullptr, attrp,
                             (char * const *) argv, environ);

  bool ok = false;
  if (result != 0)
    {
      error (0, result,
             streq (strip_program, "strip")
               ? _("cannot run %s")
               : _("cannot run strip program %s"),
             quoteaf (strip_program));
    }
  else
    {
      /* Wait for 'strip' to complete, and emit a warning message on failure  */
      int status;
      if (waitpid (pid, &status, 0) < 0)
        error (0, errno, _("waiting for strip"));
      else if (! WIFEXITED (status) || WEXITSTATUS (status))
        error (0, 0, _("strip process terminated abnormally"));
      else
        ok = true;
    }

  free (concat_name);
  if (attrp)
    posix_spawnattr_destroy (attrp);

  return ok;
}

/* Initialize the user and group ownership of the files to install. */

static void
get_ids (void)
{
  struct passwd *pw;
  struct group *gr;

  if (owner_name)
    {
      pw = getpwnam (owner_name);
      if (pw == nullptr)
        {
          uintmax_t tmp;
          if (xstrtoumax (owner_name, nullptr, 0, &tmp, "") != LONGINT_OK
              || ckd_add (&owner_id, tmp, 0))
            error (EXIT_FAILURE, 0, _("invalid user %s"),
                   quoteaf (owner_name));
        }
      else
        owner_id = pw->pw_uid;
      endpwent ();
    }
  else
    owner_id = (uid_t) -1;

  if (group_name)
    {
      gr = getgrnam (group_name);
      if (gr == nullptr)
        {
          uintmax_t tmp;
          if (xstrtoumax (group_name, nullptr, 0, &tmp, "") != LONGINT_OK
              || ckd_add (&group_id, tmp, 0))
            error (EXIT_FAILURE, 0, _("invalid group %s"),
                   quoteaf (group_name));
        }
      else
        group_id = gr->gr_gid;
      endgrent ();
    }
  else
    group_id = (gid_t) -1;
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
  or:  %s [OPTION]... -d DIRECTORY...\n\
"),
              program_name, program_name, program_name, program_name);
      fputs (_("\
\n\
This install program copies files (often just compiled) into destination\n\
locations you choose.  If you want to download and install a ready-to-use\n\
package on a GNU/Linux system, you should instead be using a package manager\n\
like yum(1) or apt-get(1).\n\
\n\
In the first three forms, copy SOURCE to DEST or multiple SOURCE(s) to\n\
the existing DIRECTORY, while setting permission modes and owner/group.\n\
In the 4th form, create all components of the given DIRECTORY(ies).\n\
"), stdout);

      emit_mandatory_arg_note ();

      fputs (_("\
      --backup[=CONTROL]  make a backup of each existing destination file\n\
  -b                  like --backup but does not accept an argument\n\
  -c                  (ignored)\n\
  -C, --compare       compare content of source and destination files, and\n\
                        if no change to content, ownership, and permissions,\n\
                        do not modify the destination at all\n\
  -d, --directory     treat all arguments as directory names; create all\n\
                        components of the specified directories\n\
"), stdout);
      fputs (_("\
  -D                  create all leading components of DEST except the last,\n\
                        or all components of --target-directory,\n\
                        then copy SOURCE to DEST\n\
"), stdout);
      fputs (_("\
      --debug         explain how a file is copied.  Implies -v\n\
"), stdout);
      fputs (_("\
  -g, --group=GROUP   set group ownership, instead of process' current group\n\
  -m, --mode=MODE     set permission mode (as in chmod), instead of rwxr-xr-x\n\
  -o, --owner=OWNER   set ownership (super-user only)\n\
"), stdout);
      fputs (_("\
  -p, --preserve-timestamps   apply access/modification times of SOURCE files\n\
                        to corresponding destination files\n\
  -s, --strip         strip symbol tables\n\
      --strip-program=PROGRAM  program used to strip binaries\n\
  -S, --suffix=SUFFIX  override the usual backup suffix\n\
  -t, --target-directory=DIRECTORY  copy all SOURCE arguments into DIRECTORY\n\
  -T, --no-target-directory  treat DEST as a normal file\n\
"), stdout);
      fputs (_("\
  -v, --verbose       print the name of each created file or directory\n\
"), stdout);
      fputs (_("\
      --preserve-context  preserve SELinux security context\n\
  -Z                      set SELinux security context of destination\n\
                            file and each created directory to default type\n\
      --context[=CTX]     like -Z, or if CTX is specified then set the\n\
                            SELinux or SMACK security context to CTX\n\
"), stdout);

      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      emit_backup_suffix_note ();
      emit_ancillary_info (PROGRAM_NAME);
    }
  exit (status);
}

/* Copy file FROM onto file TO aka TO_DIRFD+TO_RELNAME and give TO the
   appropriate attributes.  X gives the command options.
   Return true if successful.  */

static bool
install_file_in_file (char const *from, char const *to,
                      int to_dirfd, char const *to_relname,
                      const struct cp_options *x)
{
  struct stat from_sb;
  if (x->preserve_timestamps && stat (from, &from_sb) != 0)
    {
      error (0, errno, _("cannot stat %s"), quoteaf (from));
      return false;
    }
  if (! copy_file (from, to, to_dirfd, to_relname, x))
    return false;
  if (strip_files)
    if (! strip (to))
      {
        if (unlinkat (to_dirfd, to_relname, 0) != 0)  /* Cleanup.  */
          error (EXIT_FAILURE, errno, _("cannot unlink %s"), quoteaf (to));
        return false;
      }
  if (x->preserve_timestamps && (strip_files || ! S_ISREG (from_sb.st_mode))
      && ! change_timestamps (&from_sb, to, to_dirfd, to_relname))
    return false;
  return change_attributes (to, to_dirfd, to_relname);
}

/* Create any missing parent directories of TO,
   while maintaining the current Working Directory.
   Return true if successful.  */

static bool
mkancesdirs_safe_wd (char const *from, char *to, struct cp_options *x,
                     bool save_always)
{
  bool save_working_directory =
    save_always
    || ! (IS_ABSOLUTE_FILE_NAME (from) && IS_ABSOLUTE_FILE_NAME (to));
  int status = EXIT_SUCCESS;

  struct savewd wd;
  savewd_init (&wd);
  if (! save_working_directory)
    savewd_finish (&wd);

  if (mkancesdirs (to, &wd, make_ancestor, x) == -1)
    {
      error (0, errno, _("cannot create directory %s"), quoteaf (to));
      status = EXIT_FAILURE;
    }

  if (save_working_directory)
    {
      int restore_result = savewd_restore (&wd, status);
      int restore_errno = errno;
      savewd_finish (&wd);
      if (EXIT_SUCCESS < restore_result)
        return false;
      if (restore_result < 0 && status == EXIT_SUCCESS)
        {
          error (0, restore_errno, _("cannot create directory %s"),
                 quoteaf (to));
          return false;
        }
    }
  return status == EXIT_SUCCESS;
}

/* Copy file FROM onto file TO, creating any missing parent directories of TO.
   Return true if successful.  */

static bool
install_file_in_file_parents (char const *from, char *to,
                              const struct cp_options *x)
{
  return (mkancesdirs_safe_wd (from, to, (struct cp_options *)x, false)
          && install_file_in_file (from, to, AT_FDCWD, to, x));
}

/* Copy file FROM into directory TO_DIR, keeping its same name,
   and give the copy the appropriate attributes.
   Return true if successful.  */

static bool
install_file_in_dir (char const *from, char const *to_dir,
                     const struct cp_options *x, bool mkdir_and_install,
                     int *target_dirfd)
{
  char const *from_base = last_component (from);
  char *to_relname;
  char *to = file_name_concat (to_dir, from_base, &to_relname);
  bool ret = true;

  if (!target_dirfd_valid (*target_dirfd)
      && (ret = mkdir_and_install)
      && (ret = mkancesdirs_safe_wd (from, to, (struct cp_options *) x, true)))
    {
      int fd = open (to_dir, O_PATHSEARCH | O_DIRECTORY);
      if (fd < 0)
        {
          error (0, errno, _("cannot open %s"), quoteaf (to));
          ret = false;
        }
      else
        *target_dirfd = fd;
    }

  if (ret)
    {
      int to_dirfd = *target_dirfd;
      if (!target_dirfd_valid (to_dirfd))
        {
          to_dirfd = AT_FDCWD;
          to_relname = to;
        }
      ret = install_file_in_file (from, to, to_dirfd, to_relname, x);
    }

  free (to);
  return ret;
}

int
main (int argc, char **argv)
{
  int optc;
  int exit_status = EXIT_SUCCESS;
  char const *specified_mode = nullptr;
  bool make_backups = false;
  char const *backup_suffix = nullptr;
  char *version_control_string = nullptr;
  bool mkdir_and_install = false;
  struct cp_options x;
  char const *target_directory = nullptr;
  bool no_target_directory = false;
  int n_files;
  char **file;
  bool strip_program_specified = false;
  char const *scontext = nullptr;
  /* set iff kernel has extra selinux system calls */
  selinux_enabled = (0 < is_selinux_enabled ());

  initialize_main (&argc, &argv);
  set_program_name (argv[0]);
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdin);

  cp_option_init (&x);

  owner_name = nullptr;
  group_name = nullptr;
  strip_files = false;
  dir_arg = false;
  umask (0);

  while ((optc = getopt_long (argc, argv, "bcCsDdg:m:o:pt:TvS:Z", long_options,
                              nullptr))
         != -1)
    {
      switch (optc)
        {
        case 'b':
          make_backups = true;
          if (optarg)
            version_control_string = optarg;
          break;
        case 'c':
          break;
        case 'C':
          copy_only_if_needed = true;
          break;
        case 's':
          strip_files = true;
#ifdef SIGCHLD
          /* System V fork+wait does not work if SIGCHLD is ignored.  */
          signal (SIGCHLD, SIG_DFL);
#endif
          break;
        case DEBUG_OPTION:
          x.debug = x.verbose = true;
          break;
        case STRIP_PROGRAM_OPTION:
          strip_program = xstrdup (optarg);
          strip_program_specified = true;
          break;
        case 'd':
          dir_arg = true;
          break;
        case 'D':
          mkdir_and_install = true;
          break;
        case 'v':
          x.verbose = true;
          break;
        case 'g':
          group_name = optarg;
          break;
        case 'm':
          specified_mode = optarg;
          break;
        case 'o':
          owner_name = optarg;
          break;
        case 'p':
          x.preserve_timestamps = true;
          break;
        case 'S':
          make_backups = true;
          backup_suffix = optarg;
          break;
        case 't':
          if (target_directory)
            error (EXIT_FAILURE, 0,
                   _("multiple target directories specified"));
          target_directory = optarg;
          break;
        case 'T':
          no_target_directory = true;
          break;

        case PRESERVE_CONTEXT_OPTION:
          if (! selinux_enabled)
            {
              error (0, 0, _("WARNING: ignoring --preserve-context; "
                             "this kernel is not SELinux-enabled"));
              break;
            }
          x.preserve_security_context = true;
          use_default_selinux_context = false;
          break;
        case 'Z':
          if (selinux_enabled)
            {
              /* Disable use of the install(1) specific setdefaultfilecon().
                 Note setdefaultfilecon() is different from the newer and more
                 generic restorecon() in that the former sets the context of
                 the dest files to that returned by selabel_lookup directly,
                 thus discarding MLS level and user identity of the file.
                 TODO: consider removing setdefaultfilecon() in future.  */
              use_default_selinux_context = false;

              if (optarg)
                scontext = optarg;
              else
                x.set_security_context = get_labeling_handle ();
            }
          else if (optarg)
            {
              error (0, 0,
                     _("warning: ignoring --context; "
                       "it requires an SELinux-enabled kernel"));
            }
          break;
        case_GETOPT_HELP_CHAR;
        case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);
        default:
          usage (EXIT_FAILURE);
        }
    }

  /* Check for invalid combinations of arguments. */
  if (dir_arg && strip_files)
    error (EXIT_FAILURE, 0,
           _("the strip option may not be used when installing a directory"));
  if (dir_arg && target_directory)
    error (EXIT_FAILURE, 0,
           _("target directory not allowed when installing a directory"));

  x.backup_type = (make_backups
                   ? xget_version (_("backup type"),
                                   version_control_string)
                   : no_backups);
  set_simple_backup_suffix (backup_suffix);

  if (x.preserve_security_context && (x.set_security_context || scontext))
    error (EXIT_FAILURE, 0,
           _("cannot set target context and preserve it"));

  if (scontext && setfscreatecon (scontext) < 0)
    error (EXIT_FAILURE, errno,
           _("failed to set default file creation context to %s"),
         quote (scontext));

  n_files = argc - optind;
  file = argv + optind;

  if (n_files <= ! (dir_arg || target_directory))
    {
      if (n_files <= 0)
        error (0, 0, _("missing file operand"));
      else
        error (0, 0, _("missing destination file operand after %s"),
               quoteaf (file[0]));
      usage (EXIT_FAILURE);
    }

  struct stat sb;
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
      if (! (target_dirfd_valid (target_dirfd)
             || (mkdir_and_install && errno == ENOENT)))
        error (EXIT_FAILURE, errno, _("failed to access %s"),
               quoteaf (target_directory));
    }
  else if (!dir_arg)
    {
      char const *lastfile = file[n_files - 1];
      int fd = target_directory_operand (lastfile, &sb);
      if (target_dirfd_valid (fd))
        {
          target_dirfd = fd;
          target_directory = lastfile;
          n_files--;
        }
      else if (2 < n_files)
        error (EXIT_FAILURE, errno, _("target %s"), quoteaf (lastfile));
    }

  if (specified_mode)
    {
      struct mode_change *change = mode_compile (specified_mode);
      if (!change)
        error (EXIT_FAILURE, 0, _("invalid mode %s"), quote (specified_mode));
      mode = mode_adjust (0, false, 0, change, nullptr);
      dir_mode = mode_adjust (0, true, 0, change, &dir_mode_bits);
      free (change);
    }

  if (strip_program_specified && !strip_files)
    error (0, 0, _("WARNING: ignoring --strip-program option as -s option was "
                   "not specified"));

  if (copy_only_if_needed && x.preserve_timestamps)
    {
      error (0, 0, _("options --compare (-C) and --preserve-timestamps are "
                     "mutually exclusive"));
      usage (EXIT_FAILURE);
    }

  if (copy_only_if_needed && strip_files)
    {
      error (0, 0, _("options --compare (-C) and --strip are mutually "
                     "exclusive"));
      usage (EXIT_FAILURE);
    }

  if (copy_only_if_needed && extra_mode (mode))
    error (0, 0, _("the --compare (-C) option is ignored when you"
                   " specify a mode with non-permission bits"));

  get_ids ();

  if (dir_arg)
    exit_status = savewd_process_files (n_files, file, process_dir, &x);
  else
    {
      /* FIXME: it's a little gross that this initialization is
         required by copy.c::copy. */
      hash_init ();

      if (!target_directory)
        {
          if (! (mkdir_and_install
                 ? install_file_in_file_parents (file[0], file[1], &x)
                 : install_file_in_file (file[0], file[1], AT_FDCWD,
                                         file[1], &x)))
            exit_status = EXIT_FAILURE;
        }
      else
        {
          int i;
          dest_info_init (&x);
          for (i = 0; i < n_files; i++)
            if (! install_file_in_dir (file[i], target_directory, &x,
                                       i == 0 && mkdir_and_install,
                                       &target_dirfd))
              exit_status = EXIT_FAILURE;
        }
    }

  main_exit (exit_status);
}
