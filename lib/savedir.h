#undef __P
#if defined (__STDC__) && __STDC__
# define __P(x) x
#else
# define __P(x) ()
#endif

char *
savedir __P((const char *dir, unsigned int name_size));
