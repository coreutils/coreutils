#ifndef GETDELIM2_H_
# define GETDELIM2_H_ 1

# include <stddef.h>
# include <stdio.h>

int getdelim2 (char **lineptr, size_t *n, FILE *stream, int delim1, int delim2,
	       size_t offset);

#endif
