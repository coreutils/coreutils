#define IOPOLL_BROKEN_OUTPUT -2
#define IOPOLL_ERROR         -3

int iopoll (int fdin, int fdout, bool block);
bool iopoll_input_ok (int fdin);
bool iopoll_output_ok (int fdout);

bool fclose_wait (FILE *f);
bool fwrite_wait (char const *buf, ssize_t size, FILE *f);
