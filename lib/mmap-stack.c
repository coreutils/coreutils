#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#if HAVE_SYS_MMAN_H
# include <sys/mman.h>
#endif
#if HAVE_UCONTEXT_H
# include <ucontext.h>
#endif

#include "mmap-stack.h"

#ifndef MAP_ANONYMOUS
# define MAP_ANONYMOUS 0
#endif

/* Set up context, *CTX, so that it may be used via makecontext,
   using a block of SIZE bytes of mmap'd memory for its stack.
   Return nonzero upon error.  */
static int
get_context (ucontext_t *ctx, size_t size)
{
  void *stack;
  int fd = 0;

  if (getcontext (ctx))
    return 1;

  /* use tmpfile if MAP_ANONYMOUS is not defined. */
#if MAP_ANONYMOUS == 0
  {
    FILE *fp = tmpfile ();
    if (!fp)
      return 1;
    fd = fileno (fp);
  }
#endif

  stack = mmap (NULL, size,
		PROT_READ | PROT_WRITE,
		MAP_PRIVATE | MAP_ANONYMOUS, fd, 0);
  if (stack == MAP_FAILED)
    return 1;
  ctx->uc_stack.ss_sp = stack;
  ctx->uc_stack.ss_size = size;
  ctx->uc_link = NULL;
  return 0;
}

/* Yep, this is arbitrary.  tough :-)  */
#define ARGC_MAX 6

/* Set up a context without conventional stack limitations,
   then call function FUNC with the specified number (ARGC) of arguments.
   If get_context and setcontext succeed, this function does not return,
   so FUNC must be sure to exit appropriately.  Otherwise, this function
   does return, and the caller should diagnose the failure.  */
void
run_on_mmaped_stack (void (*func) (void), size_t argc, ...)
{
    ucontext_t ctx;
    size_t size = 1024 * 1024 * 1024;
    long bs_argv[ARGC_MAX];
    unsigned i;
    va_list ap;

    va_start (ap, argc);
    for (i = 0; i < argc; i++)
      bs_argv[i] = va_arg (ap, long);
    va_end (ap);

    if (get_context (&ctx, size) == 0)
      {
	makecontext (&ctx, func, argc,
		     bs_argv[0], bs_argv[1], bs_argv[2],
		     bs_argv[3], bs_argv[4], bs_argv[5]);
	setcontext (&ctx);
      }

    /* get_context or setcontext failed;
       Resort to using the conventional stack.  */
}
