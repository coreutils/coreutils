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

struct cp_options
{
  /* If nonzero, copy all files except (directories and, if not dereferencing
     them, symbolic links,) as if they were regular files. */
  int copy_as_regular;

  /* If nonzero, dereference symbolic links (copy the files they point to). */
  int dereference;

  /* If nonzero, remove existing destination nondirectories. */
  int force;

  /* If nonzero, create hard links instead of copying files.
     Create destination directories as usual. */
  int hard_link;

  /* If nonzero, query before overwriting existing destinations
     with regular files. */
  int interactive;

  /* If nonzero, when copying recursively, skip any subdirectories that are
     on different filesystems from the one we started on. */
  int one_file_system;

  /* If nonzero, give the copies the original files' permissions,
     ownership, and timestamps. */
  int preserve;

  /* If nonzero, copy directories recursively and copy special files
     as themselves rather than copying their contents. */
  int recursive;

  /* Control creation of sparse files.  */
  enum Sparse_type sparse_mode;

  /* If nonzero, create symbolic links instead of copying files.
     Create destination directories as usual. */
  int symbolic_link;

  /* The bits to preserve in created files' modes. */
  int umask_kill;

  /* If nonzero, do not copy a nondirectory that has an existing destination
     with the same or newer modification time. */
  int update;

  /* If nonzero, display the names of the files before copying them. */
  int verbose;

  /* This process's effective user ID.  */
  uid_t myeuid;

  /* A pointer to either lstat or stat, depending on
     whether the copy should dereference symlinks.  */
  int (*xstat) ();
};

int
copy __P ((const char *src_path, const char *dst_path,
	   int nonexistent_dst, const struct cp_options *options));

#endif
