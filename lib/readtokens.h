#ifndef H_READTOKENS_H
#define H_READTOKENS_H

#ifndef INITIAL_TOKEN_LENGTH
#define INITIAL_TOKEN_LENGTH 20
#endif

#ifndef TOKENBUFFER_DEFINED
#define TOKENBUFFER_DEFINED
struct tokenbuffer
{
  long size;
  char *buffer;
};
typedef struct tokenbuffer token_buffer;

#endif /* not TOKENBUFFER_DEFINED */

void init_tokenbuffer (token_buffer *tokenbuffer);

long
  readtoken (FILE *stream, const char *delim, int n_delim,
	     token_buffer *tokenbuffer);
int
  readtokens (FILE *stream, int projected_n_tokens,
	      const char *delim, int n_delim,
	      char ***tokens_out, long **token_lengths);

#endif /* not H_READTOKENS_H */
