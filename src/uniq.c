/* uniq -- remove duplicate lines from a sorted file
   Copyright (C) 86, 91, 1995-2001, Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

/* Written by Richard Stallman and David MacKenzie. */

#include <config.h>

#include <stdio.h>
#include <getopt.h>
#include <sys/types.h>

#include "system.h"
#include "closeout.h"
#include "argmatch.h"
#include "linebuffer.h"
#include "error.h"
#include "xstrtol.h"
#include "memcasecmp.h"

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "uniq"

#define AUTHORS N_ ("Richard Stallman and David MacKenzie")

#define SWAP_LINES(A, B)			\
  do						\
    {						\
      struct linebuffer *_tmp;			\
      _tmp = (A);				\
      (A) = (B);				\
      (B) = _tmp;				\
    }						\
  while (0)

/* The name this program was run with. */
char *program_name;

/* Number of fields to skip on each line when doing comparisons. */
static int skip_fields;

/* Number of chars to skip after skipping any fields. */
static int skip_chars;

/* Number of chars to compare; if 0, compare the whole lines. */
static int check_chars;

enum countmode
{
  count_occurrences,		/* -c Print count before output lines. */
  count_none			/* Default.  Do not print counts. */
};

/* Whether and how to precede the output lines with a count of the number of
   times they occurred in the input. */
static enum countmode countmode;

enum output_mode
{
  output_repeated,		/* -d Only lines that are repeated. */
  output_all_repeated,		/* -D All lines that are repeated. */
  output_unique,		/* -u Only lines that are not repeated. */
  output_all			/* Default.  Print first copy of each line. */
};

/* Which lines to output. */
static enum output_mode mode;

/* If nonzero, ignore case when comparing.  */
static int ignore_case;

enum delimit_method
{
  /* No delimiters output.  --all-repeated[=none] */
  DM_NONE,

  /* Delimiter precedes all groups.  --all-repeated=precede */
  DM_PRECEDE,

  /* Delimit all groups.  --all-repeated=separate */
  DM_SEPARATE
};

static char const *const delimit_method_string[] =
{
  "none", "precede", "separate", 0
};

static enum delimit_method const delimit_method_map[] =
{
  DM_NONE, DM_PRECEDE, DM_SEPARATE
};

/* Select whether/how to delimit groups of duplicate lines.  */
static enum delimit_method delimit_groups;

static struct option const longopts[] =
{
  {"count", no_argument, NULL, 'c'},
  {"repeated", no_argument, NULL, 'd'},
  {"all-repeated", optional_argument, NULL, 'D'},
  {"ignore-case", no_argument, NULL, 'i'},
  {"unique", no_argument, NULL, 'u'},
  {"skip-fields", required_argument, NULL, 'f'},
  {"skip-chars", required_argument, NULL, 's'},
  {"check-chars", required_argument, NULL, 'w'},
  {GETOPT_HELP_OPTION_DECL},
  {GETOPT_VERSION_OPTION_DECL},
  {NULL, 0, NULL, 0}
};

void
usage (int status)
{
  if (status != 0)
    fprintf (stderr, _("Try `%s --help' for more information.\n"),
	     program_name);
  else
    {
      printf (_("\
Usage: %s [OPTION]... [INPUT [OUTPUT]]\n\
"),
	      program_name);
      printf (_("\
Discard all but one of successive identical lines from INPUT (or\n\
standard input), writing to OUTPUT (or standard output).\n\
\n\
  -c, --count           prefix lines by the number of occurrences\n\
  -d, --repeated        only print duplicate lines\n\
  -D, --all-repeated[=delimit-method] print all duplicate lines\n\
                        delimit-method={all,minimum,none(default)}\n\
                        Delimiting is done with blank lines.\n\
  -f, --skip-fields=N   avoid comparing the first N fields\n\
  -i, --ignore-case     ignore differences in case when comparing\n\
  -s, --skip-chars=N    avoid comparing the first N characters\n\
  -u, --unique          only print unique lines\n\
  -w, --check-chars=N   compare no more than N characters in lines\n\
  -N                    same as -f N\n\
  +N                    same as -s N (obsolescent; will be withdrawn)\n\
      --help            display this help and exit\n\
      --version         output version information and exit\n\
\n\
A field is a run of whitespace, then non-whitespace characters.\n\
Fields are skipped before chars.\n\
"));
      puts (_("\nReport bugs to <bug-textutils@gnu.org>."));
    }
  exit (status == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}

/* Given a linebuffer LINE,
   return a pointer to the beginning of the line's field to be compared. */

static char *
find_field (const struct linebuffer *line)
{
  register int count;
  register char *lp = line->buffer;
  register size_t size = line->length;
  register size_t i = 0;

  for (count = 0; count < skip_fields && i < size; count++)
    {
      while (i < size && ISBLANK (lp[i]))
	i++;
      while (i < size && !ISBLANK (lp[i]))
	i++;
    }

  for (count = 0; count < skip_chars && i < size; count++)
    i++;

  return lp + i;
}

/* Return zero if two strings OLD and NEW match, nonzero if not.
   OLD and NEW point not to the beginnings of the lines
   but rather to the beginnings of the fields to compare.
   OLDLEN and NEWLEN are their lengths. */

static int
different (const char *old, const char *new, size_t oldlen, size_t newlen)
{
  register int order;

  if (check_chars)
    {
      if (oldlen > check_chars)
	oldlen = check_chars;
      if (newlen > check_chars)
	newlen = check_chars;
    }

  /* Use an if-statement here rather than a function variable to
     avoid portability hassles of getting a non-conflicting declaration
     of memcmp.  */
  if (ignore_case)
    order = memcasecmp (old, new, MIN (oldlen, newlen));
  else
    order = memcmp (old, new, MIN (oldlen, newlen));

  if (order == 0)
    return oldlen - newlen;
  return order;
}

/* Output the line in linebuffer LINE to stream STREAM
   provided that the switches say it should be output.
   If requested, print the number of times it occurred, as well;
   LINECOUNT + 1 is the number of times that the line occurred. */

static void
writeline (const struct linebuffer *line, FILE *stream, int linecount)
{
  if ((mode == output_unique && linecount != 0)
      || (mode == output_repeated && linecount == 0)
      || (mode == output_all_repeated && linecount == 0))
    return;

  if (countmode == count_occurrences)
    fprintf (stream, "%7d\t", linecount + 1);

  fwrite (line->buffer, sizeof (char), line->length, stream);
}

/* Process input file INFILE with output to OUTFILE.
   If either is "-", use the standard I/O stream for it instead. */

static void
check_file (const char *infile, const char *outfile)
{
  FILE *istream;
  FILE *ostream;
  struct linebuffer lb1, lb2;
  struct linebuffer *thisline, *prevline;

  if (STREQ (infile, "-"))
    istream = stdin;
  else
    istream = fopen (infile, "r");
  if (istream == NULL)
    error (EXIT_FAILURE, errno, "%s", infile);

  if (STREQ (outfile, "-"))
    ostream = stdout;
  else
    ostream = fopen (outfile, "w");
  if (ostream == NULL)
    error (EXIT_FAILURE, errno, "%s", outfile);

  thisline = &lb1;
  prevline = &lb2;

  initbuffer (thisline);
  initbuffer (prevline);

  /* The duplication in the following `if' and `else' blocks is an
     optimization to distinguish the common case (in which none of
     the following options has been specified: --count, -repeated,
     --all-repeated, --unique) from the others.  In the common case,
     this optimization lets uniq output each different line right away,
     without waiting to see if the next one is different.  */

  if (mode == output_all && countmode == count_none)
    {
      char *prevfield IF_LINT (= NULL);
      size_t prevlen IF_LINT (= 0);

      while (!feof (istream))
	{
	  char *thisfield;
	  size_t thislen;
	  if (readline (thisline, istream) == 0)
	    break;
	  thisfield = find_field (thisline);
	  thislen = thisline->length - (thisfield - thisline->buffer);
	  if (prevline->length == 0
	      || different (thisfield, prevfield, thislen, prevlen))
	    {
	      fwrite (thisline->buffer, sizeof (char),
		      thisline->length, ostream);

	      SWAP_LINES (prevline, thisline);
	      prevfield = thisfield;
	      prevlen = thislen;
	    }
	}
    }
  else
    {
      char *prevfield;
      size_t prevlen;
      int match_count = 0;
      int first_delimiter = 1;

      if (readline (prevline, istream) == 0)
	goto closefiles;
      prevfield = find_field (prevline);
      prevlen = prevline->length - (prevfield - prevline->buffer);

      while (!feof (istream))
	{
	  int match;
	  char *thisfield;
	  size_t thislen;
	  if (readline (thisline, istream) == 0)
	    break;
	  thisfield = find_field (thisline);
	  thislen = thisline->length - (thisfield - thisline->buffer);
	  match = !different (thisfield, prevfield, thislen, prevlen);

	  if (match)
	    ++match_count;

          if (mode == output_all_repeated && delimit_groups != DM_NONE)
	    {
	      if (!match)
		{
		  if (match_count) /* a previous match */
		    first_delimiter = 0; /* Only used when DM_SEPARATE */
		}
	      else if (match_count == 1)
		{
		  if ((delimit_groups == DM_PRECEDE)
		      || (delimit_groups == DM_SEPARATE
			  && !first_delimiter))
		    putc ('\n', ostream);
		}
	    }

	  if (!match || mode == output_all_repeated)
	    {
	      writeline (prevline, ostream, match_count);
	      SWAP_LINES (prevline, thisline);
	      prevfield = thisfield;
	      prevlen = thislen;
	      if (!match)
		match_count = 0;
	    }
	}

      writeline (prevline, ostream, match_count);
    }

 closefiles:
  if (ferror (istream) || fclose (istream) == EOF)
    error (EXIT_FAILURE, errno, _("error reading %s"), infile);

  /* Close ostream only if it's not stdout -- the latter is closed
     via the atexit-invoked close_stdout.  */
  if (ostream != stdout && (ferror (ostream) || fclose (ostream) == EOF))
    error (EXIT_FAILURE, errno, _("error writing %s"), outfile);

  free (lb1.buffer);
  free (lb2.buffer);
}

int
main (int argc, char **argv)
{
  int optc;
  char *infile = "-", *outfile = "-";

  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  skip_chars = 0;
  skip_fields = 0;
  check_chars = 0;
  mode = output_all;
  countmode = count_none;
  delimit_groups = DM_NONE;

  while ((optc = getopt_long (argc, argv, "0123456789cdDf:is:uw:", longopts,
			      NULL)) != -1)
    {
      switch (optc)
	{
	case 0:
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
	  skip_fields = skip_fields * 10 + optc - '0';
	  break;

	case 'c':
	  countmode = count_occurrences;
	  break;

	case 'd':
	  mode = output_repeated;
	  break;

	case 'D':
	  mode = output_all_repeated;
	  if (optarg == NULL)
	    delimit_groups = DM_NONE;
	  else
	    delimit_groups = XARGMATCH ("--all-repeated", optarg,
					delimit_method_string,
					delimit_method_map);
	  break;

	case 'f':		/* Like '-#'. */
	  {
	    long int tmp_long;
	    if (xstrtol (optarg, NULL, 10, &tmp_long, "") != LONGINT_OK
		|| tmp_long <= 0 || tmp_long > INT_MAX)
	      error (EXIT_FAILURE, 0,
		     _("invalid number of fields to skip: `%s'"),
		     optarg);
	    skip_fields = (int) tmp_long;
	  }
	  break;

	case 'i':
	  ignore_case = 1;
	  break;

	case 's':		/* Like '+#'. */
	  {
	    long int tmp_long;
	    if (xstrtol (optarg, NULL, 10, &tmp_long, "") != LONGINT_OK
		|| tmp_long <= 0 || tmp_long > INT_MAX)
	      error (EXIT_FAILURE, 0,
		     _("invalid number of bytes to skip: `%s'"),
		     optarg);
	    skip_chars = (int) tmp_long;
	  }
	  break;

	case 'u':
	  mode = output_unique;
	  break;

	case 'w':
	  {
	    long int tmp_long;
	    if (xstrtol (optarg, NULL, 10, &tmp_long, "") != LONGINT_OK
		|| tmp_long <= 0 || tmp_long > INT_MAX)
	      error (EXIT_FAILURE, 0,
		     _("invalid number of bytes to compare: `%s'"),
		     optarg);
	    check_chars = (int) tmp_long;
	  }
	  break;

	case_GETOPT_HELP_CHAR;

	case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);

	default:
	  usage (1);
	}
    }

  if (argc - optind >= 2 && !STREQ (argv[optind - 1], "--"))
    {
      /* Interpret non-option arguments with leading `+' only
	 if we haven't seen `--'.  */
      while (optind < argc && argv[optind][0] == '+')
	{
	  char *opt_str = argv[optind++];
	  long int tmp_long;
	  if (xstrtol (opt_str, NULL, 10, &tmp_long, "") != LONGINT_OK
	      || tmp_long <= 0 || tmp_long > INT_MAX)
	    error (EXIT_FAILURE, 0,
		   _("invalid number of bytes to skip: `%s'"),
		   opt_str);
	  skip_chars = (int) tmp_long;
	}
    }

  if (optind < argc)
    infile = argv[optind++];

  if (optind < argc)
    outfile = argv[optind++];

  if (optind < argc)
    {
      error (0, 0, _("too many arguments"));
      usage (1);
    }

  if (countmode == count_occurrences && mode == output_all_repeated)
    {
      error (0, 0,
	   _("printing all duplicated lines and repeat counts is meaningless"));
      usage (1);
    }

  check_file (infile, outfile);

  exit (EXIT_SUCCESS);
}
