/* Some systems don't have EOPNOTSUPP.  */
#ifndef EOPNOTSUPP
# ifdef ENOTSUP
#  define EOPNOTSUPP ENOTSUP
# else
/* Some systems don't have ENOTSUP either.  */
#  define EOPNOTSUPP EINVAL
# endif
#endif
