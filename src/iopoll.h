#define IOPOLL_BROKEN_OUTPUT -2
#define IOPOLL_ERROR         -3

int iopoll (int fdin, int fdout, bool block);
bool iopoll_input_ok (int fdin);
bool iopoll_output_ok (int fdout);

bool close_wait (int fd);
bool write_wait (int fd, void const *buffer, size_t size);
