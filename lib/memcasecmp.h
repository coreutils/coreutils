#undef __P
#if defined (__STDC__) && __STDC__
#define __P(args) args
#else
#define __P(args) ()
#endif

int
  memcasecmp __P((const void *vs1, const void *vs2, size_t n));
