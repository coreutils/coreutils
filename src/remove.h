
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
  RM_OK = 1,
  RM_USER_DECLINED,
  RM_ERROR
};

#define VALID_STATUS(S) \
  ((S) == RM_OK || (S) == RM_USER_DECLINED || (S) == RM_ERROR)

struct File_spec
{
  char *filename;
  unsigned int have_filetype_mode:1;
  unsigned int have_full_mode:1;
  unsigned int have_device:1;
  mode_t mode;
  ino_t st_ino;
  dev_t st_dev;
};

struct dev_ino
{
  ino_t st_ino;
  dev_t st_dev;
};

enum RM_status rm PARAMS ((struct File_spec *fs,
			   int user_specified_name,
			   struct rm_options const *x,
			   struct dev_ino const *cwd_dev_ino));
void fspec_init_file PARAMS ((struct File_spec *fs, const char *filename));
void remove_init PARAMS ((void));
void remove_fini PARAMS ((void));
