/* filetime -- display and manipulate file timestamps with microsecond precision
   Copyright (C) 2026 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

#include <config.h>
#include <stdio.h>
#include <getopt.h>
#include <sys/types.h>

#include "system.h"
#include "assure.h"
#include "parse-datetime.h"
#include "quote.h"
#include "stat-time.h"
#include "utimens.h"

/* The official name of this program (e.g., no 'g' prefix).  */
#define PROGRAM_NAME "filetime"

#define AUTHORS \
  proper_name ("GNU coreutils contributors")

/* Bitmasks for 'which_times'. */
#define FT_ATIME 1
#define FT_MTIME 2

/* Operation modes — only one may be active. */
enum ft_mode
{
  MODE_DISPLAY,  /* Default: print timestamps. */
  MODE_SET,      /* --set: write a specific timestamp. */
  MODE_COPY,     /* --copy-from: replicate another file's timestamps. */
  MODE_ADJUST    /* --adjust: shift existing timestamps by a delta. */
};

/* Long-only option codes. */
enum
{
  ADJUST_OPTION = CHAR_MAX + 1,
  COPY_FROM_OPTION
};

/* Which timestamps to act on (FT_ATIME, FT_MTIME, or both). */
static int which_times;

/* Active operation. */
static enum ft_mode mode = MODE_DISPLAY;

/* (-n) If true, operate on symlinks themselves rather than their targets. */
static bool no_dereference;

/* (-e) If true, display timestamps as epoch seconds rather than ISO format. */
static bool use_epoch;

/* Number of fractional-second digits to display (0, 3, 6, or 9). */
static int display_precision = 6;

/* MODE_SET: the target timespec (same value applied to atime and/or mtime). */
static struct timespec set_time;

/* MODE_COPY: path of the file whose timestamps we're copying. */
static char const *copy_source;

/* MODE_ADJUST: magnitude and sign of the delta. */
static struct timespec adjust_delta;
static bool adjust_negative;

static struct option const longopts[] =
{
  {"set",            required_argument, NULL, 's'},
  {"copy-from",      required_argument, NULL, COPY_FROM_OPTION},
  {"adjust",         required_argument, NULL, ADJUST_OPTION},
  {"no-dereference", no_argument,       NULL, 'n'},
  {"epoch",          no_argument,       NULL, 'e'},
  {"precision",      required_argument, NULL, 'p'},
  {GETOPT_HELP_OPTION_DECL},
  {GETOPT_VERSION_OPTION_DECL},
  {NULL, 0, NULL, 0}
};

/* Parse a fractional-seconds string starting at *P (which must point to '.').
   Store the result as nanoseconds in *NSEC and advance *P past all digits. */
static void
parse_frac (char const **p, long *nsec)
{
  affirm (**p == '.');
  (*p)++;
  long val = 0;
  int digits = 0;
  while ('0' <= **p && **p <= '9' && digits < 9)
    {
      val = val * 10 + (**p - '0');
      (*p)++;
      digits++;
    }
  /* Pad to nine digits so val is always in nanoseconds. */
  for (int i = digits; i < 9; i++)
    val *= 10;
  /* Discard any excess precision beyond nine digits. */
  while ('0' <= **p && **p <= '9')
    (*p)++;
  *nsec = val;
}

/* Parse the argument to --set.
   Accepts:
     @SECONDS[.FRAC]          epoch seconds (signed) with optional fraction
     Any other string         delegated to parse_datetime (same as touch -d).  */
static struct timespec
parse_set_time (char const *str)
{
  struct timespec result;
  if (*str == '@')
    {
      char *end;
      str++;
      errno = 0;
      long long sec = strtoll (str, &end, 10);
      if (errno || end == str)
        error (EXIT_FAILURE, 0, _("invalid date/time %s"), quote (str - 1));
      result.tv_sec = (time_t) sec;
      if (*end == '.')
        {
          char const *p = end;
          parse_frac (&p, &result.tv_nsec);
          end = (char *) p;
        }
      else
        result.tv_nsec = 0;
      if (*end != '\0')
        error (EXIT_FAILURE, 0, _("invalid date/time %s"), quote (str - 1));
    }
  else
    {
      struct timespec now = current_timespec ();
      if (! parse_datetime (&result, str, &now))
        error (EXIT_FAILURE, 0, _("invalid date/time %s"), quote (str));
    }
  return result;
}

/* Parse the argument to --adjust.
   Format: [+-]SECONDS[.FRAC]
   Sets adjust_delta and adjust_negative.  */
static void
parse_adjust (char const *str)
{
  char const *p = str;
  adjust_negative = false;
  if (*p == '+')
    p++;
  else if (*p == '-')
    {
      adjust_negative = true;
      p++;
    }

  char *end;
  errno = 0;
  long long sec = strtoll (p, &end, 10);
  if (errno || end == p || sec < 0)
    error (EXIT_FAILURE, 0, _("invalid adjustment %s"), quote (str));
  adjust_delta.tv_sec = (time_t) sec;

  if (*end == '.')
    {
      char const *fp = end;
      parse_frac (&fp, &adjust_delta.tv_nsec);
      end = (char *) fp;
    }
  else
    adjust_delta.tv_nsec = 0;

  if (*end != '\0')
    error (EXIT_FAILURE, 0, _("invalid adjustment %s"), quote (str));
}

/* Return TS ± DELTA according to NEGATIVE. */
static struct timespec
timespec_adjust (struct timespec ts, struct timespec delta, bool negative)
{
  struct timespec r;
  if (!negative)
    {
      r.tv_sec  = ts.tv_sec  + delta.tv_sec;
      r.tv_nsec = ts.tv_nsec + delta.tv_nsec;
      if (r.tv_nsec >= 1000000000)
        {
          r.tv_sec++;
          r.tv_nsec -= 1000000000;
        }
    }
  else
    {
      r.tv_sec  = ts.tv_sec  - delta.tv_sec;
      r.tv_nsec = ts.tv_nsec - delta.tv_nsec;
      if (r.tv_nsec < 0)
        {
          r.tv_sec--;
          r.tv_nsec += 1000000000;
        }
    }
  return r;
}

/* Format TS into BUF according to the current display settings.
   Either ISO-8601-like with sub-second fraction, or raw epoch seconds. */
static void
format_timespec (struct timespec ts, char *buf, size_t bufsize)
{
  /* Compute the fractional part at the requested precision. */
  long frac = ts.tv_nsec;
  int divisor = 1;
  for (int i = display_precision; i < 9; i++)
    divisor *= 10;
  frac /= divisor;

  if (use_epoch)
    {
      if (display_precision > 0)
        snprintf (buf, bufsize, "%lld.%0*ld",
                  (long long) ts.tv_sec, display_precision, frac);
      else
        snprintf (buf, bufsize, "%lld", (long long) ts.tv_sec);
      return;
    }

  struct tm *tm = localtime (&ts.tv_sec);
  if (!tm)
    {
      /* Fallback: raw epoch on localtime failure. */
      snprintf (buf, bufsize, "%lld.%09ld", (long long) ts.tv_sec, ts.tv_nsec);
      return;
    }

  size_t len = strftime (buf, bufsize, "%Y-%m-%d %H:%M:%S", tm);
  if (display_precision > 0)
    len += (size_t) snprintf (buf + len, bufsize - len,
                              ".%0*ld", display_precision, frac);
  strftime (buf + len, bufsize - len, " %z", tm);
}

/* Display the timestamps of FILE.  Return true on success. */
static bool
display_file (char const *file)
{
  struct stat st;
  if ((no_dereference ? lstat (file, &st) : stat (file, &st)) != 0)
    {
      error (0, errno, _("cannot stat %s"), quoteaf (file));
      return false;
    }

  char buf[64];
  if (which_times & FT_ATIME)
    {
      format_timespec (get_stat_atime (&st), buf, sizeof buf);
      printf ("  access: %s\n", buf);
    }
  if (which_times & FT_MTIME)
    {
      format_timespec (get_stat_mtime (&st), buf, sizeof buf);
      printf ("  modify: %s\n", buf);
    }
  return true;
}

/* Apply the active set/copy/adjust operation to FILE.  Return true on success.
   For MODE_COPY, the reference stat is read once before the file loop
   (copy_src_times) — see main().  */
static struct timespec copy_src_times[2];  /* [0]=atime [1]=mtime */

static bool
apply_times (char const *file)
{
  struct timespec newtime[2];

  switch (mode)
    {
    case MODE_SET:
      newtime[0] = newtime[1] = set_time;
      break;

    case MODE_COPY:
      newtime[0] = copy_src_times[0];
      newtime[1] = copy_src_times[1];
      break;

    case MODE_ADJUST:
      {
        struct stat st;
        if ((no_dereference ? lstat (file, &st) : stat (file, &st)) != 0)
          {
            error (0, errno, _("cannot stat %s"), quoteaf (file));
            return false;
          }
        newtime[0] = timespec_adjust (get_stat_atime (&st),
                                      adjust_delta, adjust_negative);
        newtime[1] = timespec_adjust (get_stat_mtime (&st),
                                      adjust_delta, adjust_negative);
        break;
      }

    default:
      unreachable ();
    }

  /* Mark the timestamp we are not touching as OMIT. */
  if (which_times != (FT_ATIME | FT_MTIME))
    {
      if (which_times == FT_MTIME)
        newtime[0].tv_nsec = UTIME_OMIT;
      else
        newtime[1].tv_nsec = UTIME_OMIT;
    }

  int atflag = no_dereference ? AT_SYMLINK_NOFOLLOW : 0;
  if (fdutimensat (-1, AT_FDCWD, file, newtime, atflag) != 0)
    {
      error (0, errno, _("setting times of %s"), quoteaf (file));
      return false;
    }
  return true;
}

void
usage (int status)
{
  if (status != EXIT_SUCCESS)
    emit_try_help ();
  else
    {
      printf (_("Usage: %s [OPTION]... FILE...\n"), program_name);
      fputs (_("\
Display or manipulate file timestamps with sub-second precision.\n\
\n\
Without an action option, display the timestamps of each FILE.\n\
"), stdout);

      emit_mandatory_arg_note ();

      fputs (_("\
Actions (mutually exclusive):\n\
"), stdout);
      oputs (_("\
  -s, --set=TIME         set timestamps to TIME\n\
      --copy-from=FILE   replicate timestamps from FILE\n\
      --adjust=DELTA     shift existing timestamps by DELTA seconds\n\
"));
      fputs (_("\
TIME formats:\n\
  @SECONDS[.FRAC]        Unix epoch seconds with optional fraction\n\
  'YYYY-MM-DD HH:MM:SS[.FRAC]'  or any format accepted by 'date --date'\n\
\n\
DELTA format:\n\
  [+-]SECONDS[.FRAC]     e.g. +1.5, -0.000100, +3600\n\
\n\
"), stdout);
      fputs (_("\
Selection:\n\
"), stdout);
      oputs (_("\
  -a                     act on access time only\n\
  -m                     act on modification time only\n\
"));
      fputs (_("\
Display options:\n\
"), stdout);
      oputs (_("\
  -e, --epoch            display timestamps as epoch seconds\n\
  -p, --precision=N      fractional-second digits: 0, 3, 6 (default), or 9\n\
"));
      oputs (_("\
  -n, --no-dereference   affect each symbolic link instead of its target\n\
"));
      oputs (HELP_OPTION_DESCRIPTION);
      oputs (VERSION_OPTION_DESCRIPTION);
      emit_ancillary_info (PROGRAM_NAME);
    }
  exit (status);
}

int
main (int argc, char **argv)
{
  initialize_main (&argc, &argv);
  set_program_name (argv[0]);
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  int c;
  while ((c = getopt_long (argc, argv, "ams:nep:", longopts, NULL)) != -1)
    {
      switch (c)
        {
        case 'a':
          which_times |= FT_ATIME;
          break;

        case 'm':
          which_times |= FT_MTIME;
          break;

        case 's':
          if (mode != MODE_DISPLAY)
            {
              error (0, 0, _("only one action option may be specified"));
              usage (EXIT_FAILURE);
            }
          mode = MODE_SET;
          set_time = parse_set_time (optarg);
          break;

        case COPY_FROM_OPTION:
          if (mode != MODE_DISPLAY)
            {
              error (0, 0, _("only one action option may be specified"));
              usage (EXIT_FAILURE);
            }
          mode = MODE_COPY;
          copy_source = optarg;
          break;

        case ADJUST_OPTION:
          if (mode != MODE_DISPLAY)
            {
              error (0, 0, _("only one action option may be specified"));
              usage (EXIT_FAILURE);
            }
          mode = MODE_ADJUST;
          parse_adjust (optarg);
          break;

        case 'n':
          no_dereference = true;
          break;

        case 'e':
          use_epoch = true;
          break;

        case 'p':
          {
            char *endp;
            errno = 0;
            long val = strtol (optarg, &endp, 10);
            if (errno || *endp
                || (val != 0 && val != 3 && val != 6 && val != 9))
              error (EXIT_FAILURE, 0,
                     _("invalid precision %s (must be 0, 3, 6, or 9)"),
                     quote (optarg));
            display_precision = (int) val;
            break;
          }

        case_GETOPT_HELP_CHAR;

        case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);

        default:
          usage (EXIT_FAILURE);
        }
    }

  if (which_times == 0)
    which_times = FT_ATIME | FT_MTIME;

  if (optind == argc)
    {
      error (0, 0, _("missing file operand"));
      usage (EXIT_FAILURE);
    }

  /* For MODE_COPY, stat the source once up front. */
  if (mode == MODE_COPY)
    {
      struct stat ref_st;
      if ((no_dereference ? lstat (copy_source, &ref_st)
                          : stat  (copy_source, &ref_st)) != 0)
        error (EXIT_FAILURE, errno,
               _("cannot stat %s"), quoteaf (copy_source));
      copy_src_times[0] = get_stat_atime (&ref_st);
      copy_src_times[1] = get_stat_mtime (&ref_st);
    }

  bool ok = true;
  for (; optind < argc; ++optind)
    {
      char const *file = argv[optind];
      if (mode == MODE_DISPLAY)
        {
          /* Print filename header, then the selected timestamps. */
          printf ("%s:\n", file);
          ok &= display_file (file);
        }
      else
        ok &= apply_times (file);
    }

  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
