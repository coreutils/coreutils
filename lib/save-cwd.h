#ifndef SAVE_CWD_H
# define SAVE_CWD_H 1

struct saved_cwd
  {
    int desc;
    char *name;
  };

int save_cwd (struct saved_cwd *cwd);
int restore_cwd (const struct saved_cwd *cwd, const char *dest,
		 const char *from);
void free_cwd (struct saved_cwd *cwd);

#endif /* SAVE_CWD_H */
