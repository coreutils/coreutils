# undef __P
#if __STDC__
# define __P(Args) Args
#else
# define __P(Args) ()
#endif

int
make_path __P ((const char *_argpath,
		int _mode,
		int _parent_mode,
		uid_t _owner,
		gid_t _group,
		int _preserve_existing,
		const char *_verbose_fmt_string));
