#include <stdint.h>
struct wc_lines { int err; uintmax_t lines; uintmax_t bytes; };
struct wc_lines wc_lines_avx2 (int);
