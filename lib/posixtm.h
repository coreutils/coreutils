/* POSIX Date Syntax flags.  */
#define PDS_LEADING_YEAR 1
#define PDS_TRAILING_YEAR 2
#define PDS_CENTURY 4
#define PDS_SECONDS 8

time_t
posixtime (const char *s, unsigned int syntax_bits);

struct tm *
posixtm (const char *s, unsigned int syntax_bits);
