#ifndef __P
#if defined (__GNUC__) || (defined (__STDC__) && __STDC__)
#define __P(args) args
#else
#define __P(args) ()
#endif  /* GCC.  */
#endif  /* Not __P.  */

void
  parse_long_options __P ((int _argc, char **_argv, const char *_command_name,
			   const char *_version_string, void (*_usage) (int)));
