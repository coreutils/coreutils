#ifndef VERSION_ETC_H
# define VERSION_ETC_H 1

# ifndef PARAMS
#  if defined PROTOTYPES || (defined __STDC__ && __STDC__)
#   define PARAMS(Args) Args
#  else
#   define PARAMS(Args) ()
#  endif
# endif

void
version_etc PARAMS ((FILE *stream,
		    const char *command_name, const char *package,
		    const char *version, const char *authors));

#endif /* VERSION_ETC_H */
