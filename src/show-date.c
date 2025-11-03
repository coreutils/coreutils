#include <config.h>
#include <stdio.h>

#include "system.h"
#include "fprintftime.h"
#include "parse-datetime.h"
#include "quote.h"
#include "show-date.h"
#include "stat-time.h"

/* Display the date and/or time in WHEN according to the format specified
   in FORMAT, followed by a newline.

   If successful, return true.
   If unsuccessful, prints an error message to STDERR and returns false.
   If unsuccessful and ON_ERROR_PRINT_UNFORMATTED, also prints WHEN.TV_SEC
   to STDOUT.  */

extern bool
show_date (char const *format, struct timespec when, timezone_t tz)
{
  struct tm tm;

  if (!localtime_rz (tz, &when.tv_sec, &tm))
    {
      char buf[INT_BUFSIZE_BOUND (intmax_t)];
      error (0, 0, _("time %s is out of range"),
             quote (timetostr (when.tv_sec, buf)));
      return false;
    }

  if (fprintftime (stdout, format, &tm, tz, when.tv_nsec) < 0)
    {
      if (! ferror (stdout))  /* otherwise it will be diagnosed later.  */
        error (0, errno, _("fprintftime error"));
      return false;
    }

  return true;
}
