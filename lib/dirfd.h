#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <sys/types.h>

#if HAVE_DIRENT_H
# include <dirent.h>
#else /* not HAVE_DIRENT_H */
# define dirent direct
# if HAVE_SYS_NDIR_H
#  include <sys/ndir.h>
# endif /* HAVE_SYS_NDIR_H */
# if HAVE_SYS_DIR_H
#  include <sys/dir.h>
# endif /* HAVE_SYS_DIR_H */
# if HAVE_NDIR_H
#  include <ndir.h>
# endif /* HAVE_NDIR_H */
#endif /* HAVE_DIRENT_H */

#ifndef HAVE_DECL_DIRFD
"this configure-time declaration test was not run"
#endif
#if !HAVE_DECL_DIRFD && !defined dirfd
int dirfd (DIR const *);
#endif
