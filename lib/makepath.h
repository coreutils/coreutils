#ifndef __P
#if defined (__GNUC__) || (defined (__STDC__) && __STDC__)
#define __P(args) args
#else
#define __P(args) ()
#endif /* GCC.  */
#endif /* Not __P.  */

int
  make_path __P ((const char *_argpath,
		  int _mode,
		  int _parent_mode,
		  uid_t _owner,
		  gid_t _group,
		  int _preserve_existing,
		  const char *_verbose_fmt_string));
