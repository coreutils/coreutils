#ifndef COPY_H
# define COPY_H

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

enum Dereference_symlink
{
  DEREF_UNDEFINED = 1,
  DEREF_ALWAYS,
  DEREF_NEVER,
  DEREF_COMMAND_LINE_ARGUMENTS
};

# define VALID_SPARSE_MODE(Mode)	\
  ((Mode) == SPARSE_NEVER		\
   || (Mode) == SPARSE_AUTO		\
   || (Mode) == SPARSE_ALWAYS)

struct cp_options
{
  enum backup_type backup_type;

  /* If nonzero, copy all files except (directories and, if not dereferencing
     them, symbolic links,) as if they were regular files. */
  int copy_as_regular;

  /* If nonzero, dereference symbolic links (copy the files they point to). */
  enum Dereference_symlink dereference;

  /* If nonzero, remove each existing destination nondirectory before
     trying to open it. */
  int unlink_dest_before_opening;

  /* If nonzero, first try to open each existing destination nondirectory,
     then, if the open fails, unlink and try again.
     This option must be set for `cp -f', in case the destination file
     exists when the open is attempted.  It is irrelevant to `mv' since
     any destination is sure to be removed before the open.  */
  int unlink_dest_after_failed_open;

  /* Setting this member is meaningful only if FORCE is also set.
     If nonzero, copy returns nonzero upon failed unlink.
     Otherwise, the failure still elicits a diagnostic, but it doesn't
     change copy's return value.  This is nonzero for cp and mv, and zero
     for install.  */
  /* FIXME: this is now unused.  */
  int failed_unlink_is_fatal;

  /* If nonzero, create hard links instead of copying files.
     Create destination directories as usual. */
  int hard_link;

  /* This value is used to determine whether to prompt before removing
     each existing destination file.  It works differently depending on
     whether move_mode is set.  See code/comments in copy.c.  */
  enum Interactive interactive;

  /* If nonzero, rather than copying, first attempt to use rename.
     If that fails, then resort to copying.  */
  int move_mode;

  /* This process's effective user ID.  */
  uid_t myeuid;

  /* If nonzero, when copying recursively, skip any subdirectories that are
     on different filesystems from the one we started on. */
  int one_file_system;

  /* If nonzero, attempt to give the copies the original files' permissions,
     owner, group, and timestamps. */
  int preserve_ownership;
  int preserve_mode;
  int preserve_timestamps;

  /* If nonzero and any of the above (for preserve) file attributes cannot
     be applied to a destination file, treat it as a failure and return
     nonzero immediately.  E.g. cp -p requires this be nonzero, mv requires
     it be zero.  */
  int require_preserve;

  /* If nonzero, copy directories recursively and copy special files
     as themselves rather than copying their contents. */
  int recursive;

  /* If nonzero, set file mode to value of MODE.  Otherwise,
     set it based on current umask modified by UMASK_KILL.  */
  int set_mode;

  /* Set the mode of the destination file to exactly this value
     if USE_MODE is nonzero.  */
  mode_t mode;

  /* Control creation of sparse files.  */
  enum Sparse_type sparse_mode;

  /* If nonzero, create symbolic links instead of copying files.
     Create destination directories as usual. */
  int symbolic_link;

  /* The bits to preserve in created files' modes. */
  mode_t umask_kill;

  /* If nonzero, do not copy a nondirectory that has an existing destination
     with the same or newer modification time. */
  int update;

  /* If nonzero, display the names of the files before copying them. */
  int verbose;

  /* A pointer to either lstat or stat, depending on
     whether the copy should dereference symlinks.  */
  int (*xstat) ();

  /* If nonzero, stdin is a tty.  */
  int stdin_tty;
};

int stat ();
int lstat ();

/* Arrange to make lstat calls go through the wrapper function
   on systems with an lstat function that does not dereference symlinks
   that are specified with a trailing slash.  */
# if ! LSTAT_FOLLOWS_SLASHED_SYMLINK
int rpl_lstat PARAMS((const char *, struct stat *));
#  undef lstat
#  define lstat rpl_lstat
# endif

int rename ();

/* Arrange to make rename calls go through the wrapper function
   on systems with a rename function that fails for a source path
   specified with a trailing slash.  */
# if RENAME_TRAILING_SLASH_BUG
int rpl_rename PARAMS((const char *, const char *));
#  undef rename
#  define rename rpl_rename
# endif

int
copy PARAMS ((const char *src_path, const char *dst_path,
	      int nonexistent_dst, const struct cp_options *options,
	      int *copy_into_self, int *rename_succeeded));

void
dest_info_init PARAMS ((void));

#endif
