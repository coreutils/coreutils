#if HAVE_MMAP_STACK
# define RUN_WITH_BIG_STACK_2(F, A, B)					\
    do									\
      {									\
	run_on_mmaped_stack ((void (*) (void)) F, 2, A, B);		\
	error (0, errno, _("warning: unable to use large stack"));	\
	F (A, B);							\
      }									\
    while (0)
#else
# define RUN_WITH_BIG_STACK_2(F, A, B) \
    do { F (A, B); } while (0)
#endif

void run_on_mmaped_stack (void (*func_) (void), size_t argc_, ...);
