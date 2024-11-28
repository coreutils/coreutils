#ifndef __CKSUM_H__
# define __CKSUM_H__

extern bool cksum_debug;

extern int
crc_sum_stream (FILE *stream, void *resstream, uintmax_t *length);

extern int
crc32b_sum_stream (FILE *stream, void *resstream, uintmax_t *length);

extern void
output_crc (char const *file, int binary_file, void const *digest, bool raw,
            bool tagged, unsigned char delim, bool args, uintmax_t length)
  _GL_ATTRIBUTE_NONNULL ((3));

extern bool
cksum_vmull (FILE *fp, uint_fast32_t *crc_out, uintmax_t *length_out);

extern bool
cksum_pclmul (FILE *fp, uint_fast32_t *crc_out, uintmax_t *length_out);

extern bool
cksum_avx2 (FILE *fp, uint_fast32_t *crc_out, uintmax_t *length_out);

extern bool
cksum_avx512 (FILE *fp, uint_fast32_t *crc_out, uintmax_t *length_out);

extern uint_fast32_t const crctab[8][256];

#endif
