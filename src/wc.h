#include <stdint.h>
struct wc_lines { int err; intmax_t lines; intmax_t bytes; };
struct wc_lines wc_lines_avx2 (int);
struct wc_lines wc_lines_avx512 (int);
