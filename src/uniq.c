/* uniq -- remove duplicate lines from a sorted file
   Copyright (C) 1986-2025 Free Software Foundation, Inc.

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

/* Written by Richard M. Stallman and David MacKenzie. */

#include <config.h>

#include <getopt.h>
#include <sys/types.h>

#include "system.h"
#include "argmatch.h"
#include "linebuffer.h"
#include "fadvise.h"
#include "mcel.h"
#include "posixver.h"
#include "skipchars.h"
#include "stdio--.h"
#include "xstrtol.h"
#include "memcasecmp.h"
#include "quote.h"

/* The official name of this program (e.g., no 'g' prefix).  */
#define PROGRAM_NAME "uniq"

#define AUTHORS \
  proper_name ("Richard M. Stallman"), \
  proper_name ("David MacKenzie")

static void
swap_lines (struct linebuffer **a, struct linebuffer **b)
{
  struct linebuffer *tmp = *a;
  *a = *b;
  *b = tmp;
}

/* Number of fields to skip on each line when doing comparisons. */
static idx_t skip_fields = false;

/* Number of chars to skip after skipping any fields. */
static idx_t skip_chars = false;

/* Number of chars to compare. */
static idx_t check_chars = IDX_MAX;

/* Whether to precede the output lines with a count of the number of
   times they occurred in the input. */
static bool count_occurrences = false;

/* Which lines to output: unique lines, the first of a group of
   repeated lines, and the second and subsequent of a group of
   repeated lines.  */
static bool output_unique = true;
static bool output_first_repeated = true;
static bool output_later_repeated = false;

/* If true, ignore case when comparing.  */
static bool ignore_case;

enum delimit_method
{
  /* No delimiters output.  --all-repeated[=none] */
  DM_NONE,

  /* Delimiter precedes all groups.  --all-repeated=prepend */
  DM_PREPEND,

  /* Delimit all groups.  --all-repeated=separate */
  DM_SEPARATE
};

static char const *const delimit_method_string[] =
{
  "none", "prepend", "separate", nullptr
};

static enum delimit_method const delimit_method_map[] =
{
  DM_NONE, DM_PREPEND, DM_SEPARATE
};

/* Select whether/how to delimit groups of duplicate lines.  */
static enum delimit_method delimit_groups = DM_NONE;

enum grouping_method
{
  /* No grouping, when "--group" isn't used */
  GM_NONE,

  /* Delimiter precedes all groups.  --group=prepend */
  GM_PREPEND,

  /* Delimiter follows all groups.   --group=append */
  GM_APPEND,

  /* Delimiter between groups.    --group[=separate] */
  GM_SEPARATE,

  /* Delimiter before and after each group. --group=both */
  GM_BOTH
};

static char const *const grouping_method_string[] =
{
  "prepend", "append", "separate", "both", nullptr
};

static enum grouping_method const grouping_method_map[] =
{
  GM_PREPEND, GM_APPEND, GM_SEPARATE, GM_BOTH
};

static enum grouping_method grouping = GM_NONE;

enum
{
  GROUP_OPTION = CHAR_MAX + 1
};

static struct option const longopts[] =
{
  {"count", no_argument, nullptr, 'c'},
  {"repeated", no_argument, nullptr, 'd'},
  {"all-repeated", optional_argument, nullptr, 'D'},
  {"group", optional_argument, nullptr, GROUP_OPTION},
  {"ignore-case", no_argument, nullptr, 'i'},
  {"unique", no_argument, nullptr, 'u'},
  {"skip-fields", required_argument, nullptr, 'f'},
  {"skip-chars", required_argument, nullptr, 's'},
  {"check-chars", required_argument, nullptr, 'w'},
  {"zero-terminated", no_argument, nullptr, 'z'},
  {GETOPT_HELP_OPTION_DECL},
  {GETOPT_VERSION_OPTION_DECL},
  {nullptr, 0, nullptr, 0}
};

void
usage (int status)
{
  if (status != EXIT_SUCCESS)
    emit_try_help ();
  else
    {
      printf (_("\
Usage: %s [OPTION]... [INPUT [OUTPUT]]\n\
"),
              program_name);
      fputs (_("\
Filter adjacent matching lines from INPUT (or standard input),\n\
writing to OUTPUT (or standard output).\n\
\n\
With no options, matching lines are merged to the first occurrence.\n\
"), stdout);

      emit_mandatory_arg_note ();

     fputs (_("\
  -c, --count           prefix lines by the number of occurrences\n\
  -d, --repeated        only print duplicate lines, one for each group\n\
"), stdout);
     fputs (_("\
  -D                    print all duplicate lines\n\
      --all-repeated[=METHOD]  like -D, but allow separating groups\n\
                                 with an empty line;\n\
                                 METHOD={none(default),prepend,separate}\n\
"), stdout);
     fputs (_("\
  -f, --skip-fields=N   avoid comparing the first N fields\n\
"), stdout);
     fputs (_("\
      --group[=METHOD]  show all items, separating groups with an empty line;\n\
                          METHOD={separate(default),prepend,append,both}\n\
"), stdout);
     fputs (_("\
  -i, --ignore-case     ignore differences in case when comparing\n\
  -s, --skip-chars=N    avoid comparing the first N characters\n\
  -u, --unique          only print unique lines\n\
"), stdout);
      fputs (_("\
  -z, --zero-terminated     line delimiter is NUL, not newline\n\
"), stdout);
     fputs (_("\
  -w, --check-chars=N   compare no more than N characters in lines\n\
"), stdout);
     fputs (HELP_OPTION_DESCRIPTION, stdout);
     fputs (VERSION_OPTION_DESCRIPTION, stdout);
     fputs (_("\
\n\
A field is a run of blanks (usually spaces and/or TABs), then non-blank\n\
characters.  Fields are skipped before chars.\n\
"), stdout);
     fputs (_("\
\n\
'uniq' does not detect repeated lines unless they are adjacent.\n\
You may want to sort the input first, or use 'sort -u' without 'uniq'.\n\
"), stdout);
      emit_ancillary_info (PROGRAM_NAME);
    }
  exit (status);
}

static bool
strict_posix2 (void)
{
  int posix_ver = posix2_version ();
  return 200112 <= posix_ver && posix_ver < 200809;
}

/* Convert OPT to idx_t, reporting an error using MSGID if OPT is
   invalid.  Silently convert too-large values to IDX_MAX.  */

static idx_t
size_opt (char const *opt, char const *msgid)
{
  intmax_t size;
  if (LONGINT_OVERFLOW < xstrtoimax (opt, nullptr, 10, &size, "")
      || size < 0)
    error (EXIT_FAILURE, 0, "%s: %s", opt, _(msgid));
  return MIN (size, IDX_MAX);
}

static bool
newline_or_blank (mcel_t g)
{
  return g.ch == '\n' || c32isblank (g.ch);
}

/* Given a linebuffer LINE,
   return a pointer to the beginning of the line's field to be compared.
   Store into *PLEN the length in bytes of FIELD.  */

static char *
find_field (struct linebuffer const *line, idx_t *plen)
{
  char *lp = line->buffer;
  char const *lim = lp + line->length - 1;

  for (idx_t i = skip_fields; 0 < i && lp < lim; i--)
    {
      lp = skip_buf_matching (lp, lim, newline_or_blank, true);
      lp = skip_buf_matching (lp, lim, newline_or_blank, false);
    }

  for (idx_t i = skip_chars; 0 < i && lp < lim; i--)
    lp += mcel_scan (lp, lim).len;

  /* Compute the length in bytes cheaply if possible; otherwise, scan.  */
  idx_t len;
  if (lim - lp <= check_chars)
    len = lim - lp;
  else if (MB_CUR_MAX <= 1)
    len = check_chars;
  else
    {
      char *ep = lp;
      for (idx_t i = check_chars; 0 < i && lp < lim; i--)
        ep += mcel_scan (lp, lim).len;
      len = ep - lp;
    }

  *plen = len;
  return lp;
}

/* Return false if two strings OLD and NEW match, true if not.
   OLD and NEW point not to the beginnings of the lines
   but rather to the beginnings of the fields to compare.
   OLDLEN and NEWLEN are their lengths. */

static bool
different (char *old, char *new, idx_t oldlen, idx_t newlen)
{
  if (ignore_case)
    return oldlen != newlen || memcasecmp (old, new, oldlen);
  else
    return oldlen != newlen || memcmp (old, new, oldlen);
}

/* Output the line in linebuffer LINE to standard output
   provided that the switches say it should be output.
   MATCH is true if the line matches the previous line.
   If requested, print the number of times it occurred, as well;
   LINECOUNT + 1 is the number of times that the line occurred. */

static void
writeline (struct linebuffer const *line,
           bool match, intmax_t linecount)
{
  if (! (linecount == 0 ? output_unique
         : !match ? output_first_repeated
         : output_later_repeated))
    return;

  if (count_occurrences)
    printf ("%7jd ", linecount + 1);

  if (fwrite (line->buffer, sizeof (char), line->length, stdout)
      != line->length)
    write_error ();
}

/* Process input file INFILE with output to OUTFILE.
   If either is "-", use the standard I/O stream for it instead. */

static void
check_file (char const *infile, char const *outfile, char delimiter)
{
  struct linebuffer lb1, lb2;
  struct linebuffer *thisline, *prevline;

  if (! (streq (infile, "-") || freopen (infile, "r", stdin)))
    error (EXIT_FAILURE, errno, "%s", quotef (infile));
  if (! (streq (outfile, "-") || freopen (outfile, "w", stdout)))
    error (EXIT_FAILURE, errno, "%s", quotef (outfile));

  fadvise (stdin, FADVISE_SEQUENTIAL);

  thisline = &lb1;
  prevline = &lb2;

  initbuffer (thisline);
  initbuffer (prevline);

  /* The duplication in the following 'if' and 'else' blocks is an
     optimization to distinguish between when we can print input
     lines immediately (1. & 2.) or not.

     1. --group => all input lines are printed.
        checking for unique/duplicated lines is used only for printing
        group separators.

     2. The default case in which none of these options has been specified:
          --count, --repeated,  --all-repeated, --unique
        In the default case, this optimization lets uniq output each different
        line right away, without waiting to see if the next one is different.

     3. All other cases.
  */
  if (output_unique && output_first_repeated && !count_occurrences)
    {
      char *prevfield = nullptr;
      idx_t prevlen;
      bool first_group_printed = false;

      while (!feof (stdin)
             && readlinebuffer_delim (thisline, stdin, delimiter) != 0)
        {
          idx_t thislen;
          char *thisfield = find_field (thisline, &thislen);
          bool new_group = (!prevfield
                            || different (thisfield, prevfield,
                                          thislen, prevlen));

          if (new_group && grouping != GM_NONE
              && (grouping == GM_PREPEND || grouping == GM_BOTH
                  || (first_group_printed && (grouping == GM_APPEND
                                              || grouping == GM_SEPARATE))))
            putchar (delimiter);

          if (new_group || grouping != GM_NONE)
            {
              if (fwrite (thisline->buffer, sizeof (char), thisline->length,
                  stdout) != thisline->length)
                write_error ();

              swap_lines (&prevline, &thisline);
              prevfield = thisfield;
              prevlen = thislen;
              first_group_printed = true;
            }
        }
      if ((grouping == GM_BOTH || grouping == GM_APPEND) && first_group_printed)
        putchar (delimiter);
    }
  else
    {
      if (readlinebuffer_delim (prevline, stdin, delimiter) == 0)
        goto closefiles;

      idx_t prevlen;
      char *prevfield = find_field (prevline, &prevlen);
      intmax_t match_count = 0;
      bool first_delimiter = true;

      while (!feof (stdin))
        {
          if (readlinebuffer_delim (thisline, stdin, delimiter) == 0)
            {
              if (ferror (stdin))
                goto closefiles;
              break;
            }
          idx_t thislen;
          char *thisfield = find_field (thisline, &thislen);
          bool match = !different (thisfield, prevfield, thislen, prevlen);
          match_count += match;

          if (match_count == INTMAX_MAX)
            {
              if (count_occurrences)
                error (EXIT_FAILURE, 0, _("too many repeated lines"));
              match_count--;
            }

          if (delimit_groups != DM_NONE)
            {
              if (!match)
                {
                  if (match_count) /* a previous match */
                    first_delimiter = false; /* Only used when DM_SEPARATE */
                }
              else if (match_count == 1)
                {
                  if ((delimit_groups == DM_PREPEND)
                      || (delimit_groups == DM_SEPARATE
                          && !first_delimiter))
                    putchar (delimiter);
                }
            }

          if (!match || output_later_repeated)
            {
              writeline (prevline, match, match_count);
              swap_lines (&prevline, &thisline);
              prevfield = thisfield;
              prevlen = thislen;
              if (!match)
                match_count = 0;
            }
        }

      writeline (prevline, false, match_count);
    }

 closefiles:
  if (ferror (stdin) || fclose (stdin) != 0)
    error (EXIT_FAILURE, errno, _("error reading %s"), quoteaf (infile));

  /* stdout is handled via the atexit-invoked close_stdout function.  */

  free (lb1.buffer);
  free (lb2.buffer);
}

enum Skip_field_option_type
  {
    SFO_NONE,
    SFO_OBSOLETE,
    SFO_NEW
  };

int
main (int argc, char **argv)
{
  int optc = 0;
  bool posixly_correct = (getenv ("POSIXLY_CORRECT") != nullptr);
  enum Skip_field_option_type skip_field_option_type = SFO_NONE;
  int nfiles = 0;
  char const *file[2];
  char delimiter = '\n';	/* change with --zero-terminated, -z */
  bool output_option_used = false;   /* if true, one of -u/-d/-D/-c was used */

  file[0] = file[1] = "-";
  initialize_main (&argc, &argv);
  set_program_name (argv[0]);
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  while (true)
    {
      /* Parse an operand with leading "+" as a file after "--" was
         seen; or if pedantic and a file was seen; or if not
         obsolete.  */

      if (optc == -1
          || (posixly_correct && nfiles != 0)
          || ((optc = getopt_long (argc, argv,
                                   "-0123456789Dcdf:is:uw:z",
                                   longopts, nullptr))
              == -1))
        {
          if (argc <= optind)
            break;
          if (nfiles == 2)
            {
              error (0, 0, _("extra operand %s"), quote (argv[optind]));
              usage (EXIT_FAILURE);
            }
          file[nfiles++] = argv[optind++];
        }
      else switch (optc)
        {
        case 1:
          {
            intmax_t size;
            if (optarg[0] == '+'
                && ! strict_posix2 ()
                && (xstrtoimax (optarg, nullptr, 10, &size, "")
                    <= LONGINT_OVERFLOW))
              skip_chars = MIN (size, IDX_MAX);
            else if (nfiles == 2)
              {
                error (0, 0, _("extra operand %s"), quote (optarg));
                usage (EXIT_FAILURE);
              }
            else
              file[nfiles++] = optarg;
          }
          break;

        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
          {
            if (skip_field_option_type == SFO_NEW)
              skip_fields = 0;

            if (!DECIMAL_DIGIT_ACCUMULATE (skip_fields, optc - '0'))
              skip_fields = IDX_MAX;

            skip_field_option_type = SFO_OBSOLETE;
          }
          break;

        case 'c':
          count_occurrences = true;
          output_option_used = true;
          break;

        case 'd':
          output_unique = false;
          output_option_used = true;
          break;

        case 'D':
          output_unique = false;
          output_later_repeated = true;
          if (optarg == nullptr)
            delimit_groups = DM_NONE;
          else
            delimit_groups = XARGMATCH ("--all-repeated", optarg,
                                        delimit_method_string,
                                        delimit_method_map);
          output_option_used = true;
          break;

        case GROUP_OPTION:
          if (optarg == nullptr)
            grouping = GM_SEPARATE;
          else
            grouping = XARGMATCH ("--group", optarg,
                                  grouping_method_string,
                                  grouping_method_map);
          break;

        case 'f':
          skip_field_option_type = SFO_NEW;
          skip_fields = size_opt (optarg,
                                  N_("invalid number of fields to skip"));
          break;

        case 'i':
          ignore_case = true;
          break;

        case 's':
          skip_chars = size_opt (optarg,
                                 N_("invalid number of bytes to skip"));
          break;

        case 'u':
          output_first_repeated = false;
          output_option_used = true;
          break;

        case 'w':
          check_chars = size_opt (optarg,
                                  N_("invalid number of bytes to compare"));
          break;

        case 'z':
          delimiter = '\0';
          break;

        case_GETOPT_HELP_CHAR;

        case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);

        default:
          usage (EXIT_FAILURE);
        }
    }

  /* Note we could allow --group with -D at least, and that would
     avoid the need to specify a grouping method to --all-repeated.
     It was thought best to avoid deprecating those parameters though
     and keep --group separate to other options.  */
  if (grouping != GM_NONE && output_option_used)
    {
      error (0, 0, _("--group is mutually exclusive with -c/-d/-D/-u"));
      usage (EXIT_FAILURE);
    }

  if (grouping != GM_NONE && count_occurrences)
    {
      error (0, 0,
           _("grouping and printing repeat counts is meaningless"));
      usage (EXIT_FAILURE);
    }

  if (count_occurrences && output_later_repeated)
    {
      error (0, 0,
           _("printing all duplicated lines and repeat counts is meaningless"));
      usage (EXIT_FAILURE);
    }

  check_file (file[0], file[1], delimiter);

  return EXIT_SUCCESS;
}
