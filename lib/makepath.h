#ifndef PARAMS
# if defined PROTOTYPES || (defined __STDC__ && __STDC__)
#  define PARAMS(Args) Args
# else
#  define PARAMS(Args) ()
# endif
#endif

int
make_path PARAMS ((const char *_argpath,
		   int _mode,
		   int _parent_mode,
		   uid_t _owner,
		   gid_t _group,
		   int _preserve_existing,
		   const char *_verbose_fmt_string));

int
make_dir PARAMS ((const char *dir,
		  const char *dirpath,
		  mode_t mode,
		  int *created_dir_p));
