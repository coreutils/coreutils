#ifndef _error_h_
#define _error_h_

#ifdef __GNUC__
void error (int, int, const char *, ...)
#if __GNUC__ > 1
    __attribute__ ((format (printf, 3, 4)))
#endif
    ;
#else
void error ();
#endif

#endif /* _error_h_ */
