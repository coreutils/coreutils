#ifndef SETENV_H
# define SETENV_H 1

# undef __P
# if defined (__STDC__) && __STDC__
#  define	__P(x) x
# else
#  define	__P(x) ()
# endif

int
  setenv __P ((const char *name, const char *value, int replace));

void
  unsetenv __P ((const char *name));

#endif /* SETENV_H */
