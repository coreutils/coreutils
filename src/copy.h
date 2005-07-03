/* core functions for copying files and directories
   Copyright (C) 89, 90, 91, 1995-2005 Free Software Foundation.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

/* Extracted from cp.c and librarified by Jim Meyering.  */

#ifndef COPY_H
# define COPY_H

# include <stdbool.h>
# include "hash.h"
# include "lstat.h"

/* Control creation of sparse files (files with holes).  */
enum Sparse_type
{
  SPARSE_UNUSED,

  /* Never create holes in DEST.  */
  SPARSE_NEVER,

  /* This is the default.  Use a crude (and sometimes inaccurate)
     heuristic to determine if SOURCE has holes.  If so, try to create
     holes in DEST.  */
  SPARSE_AUTO,

  /* For every sufficiently long sequence of bytes in SOURCE, try to
     create a corresponding hole in DEST.  There is a performance penalty
     here because CP has to search for holes in SRC.  But if the holes are
     big enough, that penalty can be offset by the decrease in the amount
     of data written to disk.   */
  SPARSE_ALWAYS
};

/* This type is used to help mv (via copy.c) distinguish these cases.  */
enum Interactive
{
  I_ALWAYS_YES = 1,
  I_ALWAYS_NO,
  I_ASK_USER,
  I_UNSPECIFIED
};

/* How to handle symbolic links.  */
enum Dereference_symlink
{
  DEREF_UNDEFINED = 1,

  /* Copy the symbolic link itself.  -P  */
  DEREF_NEVER,

  /* If the symbolic is a command line argument, then copy
     its referent.  Otherwise, copy the symbolic link itself.  -H  */
  DEREF_COMMAND_LINE_ARGUMENTS,

  /* Copy the referent of the symbolic link.  -L  */
  DEREF_ALWAYS
};

# define VALID_SPARSE_MODE(Mode)	\
  ((Mode) == SPARSE_NEVER		\
   || (Mode) == SPARSE_AUTO		\
   || (Mode) == SPARSE_ALWAYS)

/* These options control how files are copied by at least the
   following programs: mv (when rename doesn't work), cp, install.
   So, if you add a new member, be sure to initialize it in
   mv.c, cp.c, and install.c.  */
struct cp_options
{
  enum backup_type backup_type;

  /* If true, copy all files except (directories and, if not dereferencing
     them, symbolic links,) as if they were regular files.  */
  bool copy_as_regular;

  /* How to handle symlinks.  */
  enum Dereference_symlink dereference;

  /* If true, remove each existing destination nondirectory before
     trying to open it.  */
  bool unlink_dest_before_opening;

  /* If true, first try to open each existing destination nondirectory,
     then, if the open fails, unlink and try again.
     This option must be set for `cp -f', in case the destination file
     exists when the open is attempted.  It is irrelevant to `mv' since
     any destination is sure to be removed before the open.  */
  bool unlink_dest_after_failed_open;

  /* If true, create hard links instead of copying files.
     Create destination directories as usual. */
  bool hard_link;

  /* This value is used to determine whether to prompt before removing
     each existing destination file.  It works differently depending on
     whether move_mode is set.  See code/comments in copy.c.  */
  enum Interactive interactive;

  /* If true, rather than copying, first attempt to use rename.
     If that fails, then resort to copying.  */
  bool move_mode;

  /* Whether this process has appropriate privileges to chown a file
     whose owner is not the effective user ID.  */
  bool chown_privileges;

  /* If true, when copying recursively, skip any subdirectories that are
     on different file systems from the one we started on.  */
  bool one_file_system;

  /* If true, attempt to give the copies the original files' permissions,
     owner, group, and timestamps. */
  bool preserve_ownership;
  bool preserve_mode;
  bool preserve_timestamps;

  /* Enabled for mv, and for cp by the --preserve=links option.
     If true, attempt to preserve in the destination files any
     logical hard links between the source files.  If used with cp's
     --no-dereference option, and copying two hard-linked files,
     the two corresponding destination files will also be hard linked.

     If used with cp's --dereference (-L) option, then, as that option implies,
     hard links are *not* preserved.  However, when copying a file F and
     a symlink S to F, the resulting S and F in the destination directory
     will be hard links to the same file (a copy of F).  */
  bool preserve_links;

  /* If true and any of the above (for preserve) file attributes cannot
     be applied to a destination file, treat it as a failure and return
     nonzero immediately.  E.g. cp -p requires this be nonzero, mv requires
     it be zero.  */
  bool require_preserve;

  /* If true, copy directories recursively and copy special files
     as themselves rather than copying their contents. */
  bool recursive;

  /* If true, set file mode to value of MODE.  Otherwise,
     set it based on current umask modified by UMASK_KILL.  */
  bool set_mode;

  /* Set the mode of the destination file to exactly this value
     if SET_MODE is nonzero.  */
  mode_t mode;

  /* Control creation of sparse files.  */
  enum Sparse_type sparse_mode;

  /* If true, create symbolic links instead of copying files.
     Create destination directories as usual. */
  bool symbolic_link;

  /* The bits to preserve in created files' modes. */
  mode_t umask_kill;

  /* If true, do not copy a nondirectory that has an existing destination
     with the same or newer modification time. */
  bool update;

  /* If true, display the names of the files before copying them. */
  bool verbose;

  /* If true, stdin is a tty.  */
  bool stdin_tty;

  /* This is a set of destination name/inode/dev triples.  Each such triple
     represents a file we have created corresponding to a source file name
     that was specified on the command line.  Use it to avoid clobbering
     source files in commands like this:
       rm -rf a b c; mkdir a b c; touch a/f b/f; mv a/f b/f c
     For now, it protects only regular files when copying (i.e. not renaming).
     When renaming, it protects all non-directories.
     Use dest_info_init to initialize it, or set it to NULL to disable
     this feature.  */
  Hash_table *dest_info;

  /* FIXME */
  Hash_table *src_info;
};

# define XSTAT(X, Src_name, Src_sb) \
  ((X)->dereference == DEREF_NEVER \
   ? lstat (Src_name, Src_sb) \
   : stat (Src_name, Src_sb))

/* Arrange to make rename calls go through the wrapper function
   on systems with a rename function that fails for a source file name
   specified with a trailing slash.  */
# if RENAME_TRAILING_SLASH_BUG
int rpl_rename (const char *, const char *);
#  undef rename
#  define rename rpl_rename
# endif

bool copy (char const *src_name, char const *dst_name,
	   bool nonexistent_dst, const struct cp_options *options,
	   bool *copy_into_self, bool *rename_succeeded);

void dest_info_init (struct cp_options *);
void src_info_init (struct cp_options *);

bool chown_privileges (void);
bool chown_failure_ok (struct cp_options const *);

#endif
