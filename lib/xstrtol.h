#ifndef _xstrtol_h_
#define _xstrtol_h_ 1

#if STRING_TO_UNSIGNED
# define __xstrtol xstrtoul
# define __strtol strtoul
# define __unsigned unsigned
# define __ZLONG_MAX ULONG_MAX
#else
# define __xstrtol xstrtol
# define __strtol strtol
# define __unsigned /* empty */
# define __ZLONG_MAX LONG_MAX
#endif

#undef __P
#if defined (__STDC__) && __STDC__
#define	__P(x) x
#else
#define	__P(x) ()
#endif

enum strtol_error
  {
    LONGINT_OK, LONGINT_INVALID, LONGINT_INVALID_SUFFIX_CHAR, LONGINT_OVERFLOW
  };
typedef enum strtol_error strtol_error;

strtol_error
  __xstrtol __P ((const char *s, char **ptr, int base,
		  __unsigned long int *val, const char *valid_suffixes));

#define _STRTOL_ERROR(exit_code, str, argument_type_string, err)	\
  do									\
    {									\
      switch ((err))							\
	{								\
	case LONGINT_OK:						\
	  abort ();							\
									\
	case LONGINT_INVALID:						\
	  error ((exit_code), 0, "invalid %s `%s'",			\
		 (argument_type_string), (str));			\
	  break;							\
									\
	case LONGINT_INVALID_SUFFIX_CHAR:				\
	  error ((exit_code), 0, "invalid character following %s `%s'",	\
		 (argument_type_string), (str));			\
	  break;							\
									\
	case LONGINT_OVERFLOW:						\
	  /* FIXME: make this message dependent on STRING_TO_UNSIGNED */\
	  error ((exit_code), 0, "%s `%s' larger than maximum long int",\
		 (argument_type_string), (str));			\
	  break;							\
	}								\
    }									\
  while (0)

#define STRTOL_FATAL_ERROR(str, argument_type_string, err)		\
  _STRTOL_ERROR (2, str, argument_type_string, err)

#define STRTOL_FAIL_WARN(str, argument_type_string, err)		\
  _STRTOL_ERROR (0, str, argument_type_string, err)

#endif /* _xstrtol_h_ */
