/* Some systems don't have ENOSYS.  */
#ifndef ENOSYS
# ifdef ENOTSUP
#  define ENOSYS ENOTSUP
# else
/* Some systems don't have ENOTSUP either.  */
#  define ENOSYS ENOMSG
# endif
#endif
