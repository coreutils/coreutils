int make_path (const char *_argpath,
	       int _mode,
	       int _parent_mode,
	       uid_t _owner,
	       gid_t _group,
	       int _preserve_existing,
	       const char *_verbose_fmt_string);

int make_dir (const char *dir,
	      const char *dirpath,
	      mode_t mode,
	      int *created_dir_p);
