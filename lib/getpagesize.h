/* Emulate getpagesize on systems that lack it.  */

#ifndef HAVE_GETPAGESIZE

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#if !defined getpagesize && defined _SC_PAGESIZE
# if !(defined VMS && __VMS_VER < 70000000)
#  define getpagesize() sysconf (_SC_PAGESIZE)
# endif
#endif

#if !defined getpagesize && defined VMS
# ifdef __ALPHA
#  define getpagesize() 8192
# else
#  define getpagesize() 512
# endif
#endif

/* This is for BeOS.  */
#if !defined getpagesize && HAVE_OS_H
# include <OS.h>
# if defined B_PAGE_SIZE
#  define getpagesize() B_PAGE_SIZE
# endif
#endif

#ifndef getpagesize
# include <sys/param.h>
# ifdef EXEC_PAGESIZE
#  define getpagesize() EXEC_PAGESIZE
# else
#  ifdef NBPG
#   ifndef CLSIZE
#    define CLSIZE 1
#   endif
#   define getpagesize() (NBPG * CLSIZE)
#  else
#   ifdef NBPC
#    define getpagesize() NBPC
#   endif
#  endif
# endif
#endif

#endif /* not HAVE_GETPAGESIZE */
