#undef PARAMS
#if defined (__STDC__) && __STDC__
# define PARAMS(Args) Args
#else
# define PARAMS(Args) ()
#endif

void
  parse_long_options PARAMS ((int _argc, char **_argv,
			      const char *_command_name,
			      const char *_package,
			      const char *_version, void (*_usage) (int)));
