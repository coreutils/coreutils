#ifndef CANONICALIZE_H_
# define CANONICALIZE_H_

enum canonicalize_mode_t
  {
    CAN_EXISTING = 0,  /* All path components must exist. */
    CAN_ALL_BUT_LAST = 1,  /* All path components exluding last one must exist. */
    CAN_MISSING = 2,  /* No requirements on components existence. */
  };
typedef enum canonicalize_mode_t canonicalize_mode_t;

char *canonicalize_filename_mode (const char *, canonicalize_mode_t);

#if !HAVE_CANONICALIZE_FILE_NAME
char *canonicalize_file_name (const char *);
#endif

#endif /* !CANONICALIZE_H_ */
