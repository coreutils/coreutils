#ifndef SAVE_CWD_H
# define SAVE_CWD_H 1

struct saved_cwd
  {
    int desc;
    char *name;
  };

# ifndef __P
#  if defined (__GNUC__) || (defined (__STDC__) && __STDC__)
#   define __P(args) args
#  else
#   define __P(args) ()
#  endif  /* GCC.  */
# endif  /* Not __P.  */

int save_cwd __P((struct saved_cwd *cwd));
int restore_cwd __P((const struct saved_cwd *cwd, const char *dest,
		     const char *from));
void free_cwd __P((struct saved_cwd *cwd));

#endif /* SAVE_CWD_H */
