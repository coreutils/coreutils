#if __STDC__
# undef __P
# define __P(args) args
#else
# define __P(args) ()
#endif

char *
path_concat __P ((const char *dir, const char *base, char **base_in_result));
