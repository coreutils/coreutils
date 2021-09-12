extern int
bsd_sum_stream (FILE *stream, void *resstream, uintmax_t *length);

extern int
sysv_sum_stream (FILE *stream, void *resstream, uintmax_t *length);

typedef int (*sumfn)(FILE *, void *, uintmax_t*);


extern void
output_bsd (char const *file, int binary_file, void const *digest,
            bool tagged, unsigned char delim, bool args _GL_UNUSED,
            uintmax_t length _GL_UNUSED);

extern void
output_sysv (char const *file, int binary_file, void const *digest,
             bool tagged, unsigned char delim, bool args _GL_UNUSED,
             uintmax_t length _GL_UNUSED);
