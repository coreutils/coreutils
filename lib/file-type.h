#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>

char const *file_type (struct stat const *);

#if STAT_MACROS_BROKEN
# undef S_ISBLK
# undef S_ISCHR
# undef S_ISDIR
# undef S_ISDOOR
# undef S_ISFIFO
# undef S_ISLNK
# undef S_ISMPB
# undef S_ISMPC
# undef S_ISNWK
# undef S_ISREG
# undef S_ISSOCK
#endif /* STAT_MACROS_BROKEN.  */

#ifndef S_IFMT
# define S_IFMT 0170000
#endif
#if !defined(S_ISBLK) && defined(S_IFBLK)
# define S_ISBLK(m) (((m) & S_IFMT) == S_IFBLK)
#endif
#if !defined(S_ISCHR) && defined(S_IFCHR)
# define S_ISCHR(m) (((m) & S_IFMT) == S_IFCHR)
#endif
#if !defined(S_ISDIR) && defined(S_IFDIR)
# define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#endif
#if !defined(S_ISREG) && defined(S_IFREG)
# define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#endif
#if !defined(S_ISFIFO) && defined(S_IFIFO)
# define S_ISFIFO(m) (((m) & S_IFMT) == S_IFIFO)
#endif
#if !defined(S_ISLNK) && defined(S_IFLNK)
# define S_ISLNK(m) (((m) & S_IFMT) == S_IFLNK)
#endif
#if !defined(S_ISSOCK) && defined(S_IFSOCK)
# define S_ISSOCK(m) (((m) & S_IFMT) == S_IFSOCK)
#endif
#if !defined(S_ISMPB) && defined(S_IFMPB) /* V7 */
# define S_ISMPB(m) (((m) & S_IFMT) == S_IFMPB)
# define S_ISMPC(m) (((m) & S_IFMT) == S_IFMPC)
#endif
#if !defined(S_ISNWK) && defined(S_IFNWK) /* HP/UX */
# define S_ISNWK(m) (((m) & S_IFMT) == S_IFNWK)
#endif
#if !defined(S_ISDOOR) && defined(S_IFDOOR) /* Solaris 2.5 and up */
# define S_ISDOOR(m) (((m) & S_IFMT) == S_IFDOOR)
#endif

/* If any of the following S_* macros are undefined, define them here
   so each use doesn't have to be guarded with e.g., #ifdef S_ISLNK.  */
#ifndef S_ISREG
# define S_ISREG(Mode) 0
#endif

#ifndef S_ISDIR
# define S_ISDIR(Mode) 0
#endif

#ifndef S_ISLNK
# define S_ISLNK(Mode) 0
#endif

#ifndef S_ISFIFO
# define S_ISFIFO(Mode) 0
#endif

#ifndef S_ISSOCK
# define S_ISSOCK(Mode) 0
#endif

#ifndef S_ISCHR
# define S_ISCHR(Mode) 0
#endif

#ifndef S_ISBLK
# define S_ISBLK(Mode) 0
#endif

#ifndef S_ISDOOR
# define S_ISDOOR(Mode) 0
#endif

#ifndef S_TYPEISSEM
# define S_TYPEISSEM(Stat_buf_p) 0
#endif

#ifndef S_TYPEISSHM
# define S_TYPEISSHM(Stat_buf_p) 0
#endif

#ifndef S_TYPEISMQ
# define S_TYPEISMQ(Stat_buf_p) 0
#endif
