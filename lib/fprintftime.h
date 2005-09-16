#include <stdio.h>
#include <time.h>

/* A cross between fprintf and nstrftime, that prints directly
   to the output stream, without the need for the potentially
   large buffer that nstrftime would require.

   Output to stream FP the result of formatting (according to the
   nstrftime format string, FMT) the time data, *TM, and the UTC
   and NANOSECONDS values.  */
size_t fprintftime (FILE *fp, char const *fmt, struct tm const *tm,
		    int utc, int nanoseconds);
