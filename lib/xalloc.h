#undef PARAMS
#if defined (__STDC__) && __STDC__
# define PARAMS(Args) Args
#else
# define PARAMS(Args) ()
#endif

/* Exit value when the requested amount of memory is not available.
   The caller may set it to some other value.  */
extern int xalloc_exit_failure;

/* FIXME: describe */
extern char *const xalloc_msg_memory_exhausted;

/* FIXME: describe */
extern void (*xalloc_fail_func) ();

void *xmalloc PARAMS ((size_t n));
void *xcalloc PARAMS ((size_t n, size_t s));
void *xrealloc PARAMS ((void *p, size_t n));
