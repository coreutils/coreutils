#ifndef __CKSUM_H__
# define __CKSUM_H__

extern bool cksum_debug;

extern int
crc_sum_stream (FILE *stream, void *resstream, uintmax_t *length);

extern void
output_crc (char const *file, int binary_file, void const *digest,
            bool tagged, unsigned char delim, bool args _GL_UNUSED,
            uintmax_t length _GL_UNUSED);

extern bool
cksum_pclmul (FILE *fp, uint_fast32_t *crc_out, uintmax_t *length_out);

extern uint_fast32_t const crctab[8][256];

#endif
