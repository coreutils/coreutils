#ifndef XSTRTOL_H_
# define XSTRTOL_H_ 1

# if STRING_TO_UNSIGNED
#  define __xstrtol xstrtoul
#  define __strtol strtoul
#  define __unsigned unsigned
#  define __ZLONG_MAX ULONG_MAX
# else
#  define __xstrtol xstrtol
#  define __strtol strtol
#  define __unsigned /* empty */
#  define __ZLONG_MAX LONG_MAX
# endif

# undef PARAMS
# if defined (__STDC__) && __STDC__
#  define PARAMS(Args) Args
# else
#  define PARAMS(Args) ()
# endif

enum strtol_error
  {
    LONGINT_OK, LONGINT_INVALID, LONGINT_INVALID_SUFFIX_CHAR, LONGINT_OVERFLOW
  };
typedef enum strtol_error strtol_error;

strtol_error
  __xstrtol PARAMS ((const char *s, char **ptr, int base,
		     __unsigned long int *val, const char *valid_suffixes));

# define _STRTOL_ERROR(Exit_code, Str, Argument_type_string, Err)	\
  do									\
    {									\
      switch ((Err))							\
	{								\
	case LONGINT_OK:						\
	  abort ();							\
									\
	case LONGINT_INVALID:						\
	  error ((Exit_code), 0, "invalid %s `%s'",			\
		 (Argument_type_string), (Str));			\
	  break;							\
									\
	case LONGINT_INVALID_SUFFIX_CHAR:				\
	  error ((Exit_code), 0, "invalid character following %s `%s'",	\
		 (Argument_type_string), (Str));			\
	  break;							\
									\
	case LONGINT_OVERFLOW:						\
	  /* FIXME: make this message dependent on STRING_TO_UNSIGNED */\
	  error ((Exit_code), 0, "%s `%s' larger than maximum long int",\
		 (Argument_type_string), (Str));			\
	  break;							\
	}								\
    }									\
  while (0)

# define STRTOL_FATAL_ERROR(Str, Argument_type_string, Err)		\
  _STRTOL_ERROR (2, Str, Argument_type_string, Err)

# define STRTOL_FAIL_WARN(Str, Argument_type_string, Err)		\
  _STRTOL_ERROR (0, Str, Argument_type_string, Err)

#endif /* not XSTRTOL_H_ */
