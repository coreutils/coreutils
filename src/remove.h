#ifndef REMOVE_H
# define REMOVE_H

struct rm_options
{
  /* If nonzero, ignore nonexistent files.  */
  int ignore_missing_files;

  /* If nonzero, query the user about whether to remove each file.  */
  int interactive;

  /* If nonzero, recursively remove directories.  */
  int recursive;

  /* If nonzero, stdin is a tty.  */
  int stdin_tty;

  /* If nonzero, remove directories with unlink instead of rmdir, and don't
     require a directory to be empty before trying to unlink it.
     Only works for the super-user.  */
  int unlink_dirs;

  /* If nonzero, display the name of each file removed.  */
  int verbose;
};

enum RM_status
{
  /* These must be listed in order of increasing seriousness. */
  RM_OK = 2,
  RM_USER_DECLINED,
  RM_ERROR,
  RM_NONEMPTY_DIR
};

# define VALID_STATUS(S) \
  ((S) == RM_OK || (S) == RM_USER_DECLINED || (S) == RM_ERROR)

# define UPDATE_STATUS(S, New_value)				\
  do								\
    {								\
      if ((New_value) == RM_ERROR				\
	  || ((New_value) == RM_USER_DECLINED && (S) == RM_OK))	\
	(S) = (New_value);					\
    }								\
  while (0)

enum RM_status rm (size_t n_files, char const *const *file,
		   struct rm_options const *x);

#endif
