#undef __P
#if defined (__STDC__) && __STDC__
# define __P(Args) Args
#else
# define __P(Args) ()
#endif

void
  parse_long_options __P ((int _argc, char **_argv, const char *_command_name,
			   const char *_package,
			   const char *_version_string, void (*_usage) (int)));
