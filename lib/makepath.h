#if __STDC__
# undef __P
# define __P(args) args
#else
# define __P(args) ()
#endif

int
make_path __P ((const char *_argpath,
		int _mode,
		int _parent_mode,
		uid_t _owner,
		gid_t _group,
		int _preserve_existing,
		const char *_verbose_fmt_string));
