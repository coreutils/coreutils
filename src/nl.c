/* nl -- number lines of files
   Copyright (C) 1989, 1992 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* Written by Scott Bartram (nancy!scott@uunet.uu.net)
   Revised by David MacKenzie (djm@gnu.ai.mit.edu) */

#ifdef HAVE_CONFIG_H
#if defined (CONFIG_BROKETS)
/* We use <config.h> instead of "config.h" so that a compilation
   using -I. -I$srcdir will use ./config.h rather than $srcdir/config.h
   (which it would do because it found this file in $srcdir).  */
#include <config.h>
#else
#include "config.h"
#endif
#endif

#include <stdio.h>
#include <sys/types.h>
#include <getopt.h>
#include <regex.h>
#include "linebuffer.h"
#include "system.h"
#include "version.h"

#ifndef TRUE
#define TRUE   1
#define FALSE  0
#endif

/* Line-number formats. */
enum number_format
{
  FORMAT_RIGHT_NOLZ,		/* Right justified, no leading zeroes.  */
  FORMAT_RIGHT_LZ,		/* Right justified, leading zeroes.  */
  FORMAT_LEFT			/* Left justified, no leading zeroes.  */
};

/* Default section delimiter characters.  */
#define DEFAULT_SECTION_DELIMITERS  "\\:"

/* Types of input lines: either one of the section delimiters,
   or text to output. */
enum section
{
  Header, Body, Footer, Text
};

char *xmalloc ();
char *xrealloc ();
void error ();

static enum section check_section ();
static int build_type_arg ();
static int nl_file ();
static void usage ();
static void process_file ();
static void proc_header ();
static void proc_body ();
static void proc_footer ();
static void proc_text ();
static void print_lineno ();
static void build_print_fmt ();

/* The name this program was run with. */
char *program_name;

/* Format of body lines (-b).  */
static char *body_type = "t";

/* Format of header lines (-h).  */
static char *header_type = "n";

/* Format of footer lines (-f).  */
static char *footer_type = "n";

/* Format currently being used (body, header, or footer).  */
static char *current_type;

/* Regex for body lines to number (-bp).  */
static struct re_pattern_buffer body_regex;

/* Regex for header lines to number (-hp).  */
static struct re_pattern_buffer header_regex;

/* Regex for footer lines to number (-fp).  */
static struct re_pattern_buffer footer_regex;

/* Pointer to current regex, if any.  */
static struct re_pattern_buffer *current_regex = NULL;

/* Separator string to print after line number (-s).  */
static char *separator_str = "\t";

/* Input section delimiter string (-d).  */
static char *section_del = DEFAULT_SECTION_DELIMITERS;

/* Header delimiter string.  */
static char *header_del = NULL;

/* Header section delimiter length.  */
static int header_del_len;

/* Body delimiter string.  */
static char *body_del = NULL;

/* Body section delimiter length.  */
static int body_del_len;

/* Footer delimiter string.  */
static char *footer_del = NULL;

/* Footer section delimiter length.  */
static int footer_del_len;

/* Input buffer.  */
static struct linebuffer line_buf;

/* printf format string for line number.  */
static char *print_fmt;

/* printf format string for unnumbered lines.  */
static char *print_no_line_fmt = NULL;

/* Starting line number on each page (-v).  */
static int page_start = 1;

/* Line number increment (-i).  */
static int page_incr = 1;

/* If TRUE, reset line number at start of each page (-p).  */
static int reset_numbers = TRUE;

/* Number of blank lines to consider to be one line for numbering (-l).  */
static int blank_join = 1;

/* Width of line numbers (-w).  */
static int lineno_width = 6;

/* Line number format (-n).  */
static enum number_format lineno_format = FORMAT_RIGHT_NOLZ;

/* Current print line number.  */
static int line_no;

/* Nonzero if we have ever read standard input. */
static int have_read_stdin;

/* If non-zero, display usage information and exit.  */
static int show_help;

/* If non-zero, print the version on standard output then exit.  */
static int show_version;

static struct option const longopts[] =
{
  {"header-numbering", required_argument, NULL, 'h'},
  {"body-numbering", required_argument, NULL, 'b'},
  {"footer-numbering", required_argument, NULL, 'f'},
  {"first-page", required_argument, NULL, 'v'},
  {"page-increment", required_argument, NULL, 'i'},
  {"no-renumber", no_argument, NULL, 'p'},
  {"join-blank-lines", required_argument, NULL, 'l'},
  {"number-separator", required_argument, NULL, 's'},
  {"number-width", required_argument, NULL, 'w'},
  {"number-format", required_argument, NULL, 'n'},
  {"section-delimiter", required_argument, NULL, 'd'},
  {"help", no_argument, &show_help, 1},
  {"version", no_argument, &show_version, 1},
  {NULL, 0, NULL, 0}
};

void
main (argc, argv)
     int argc;
     char **argv;
{
  int c, exit_status = 0;

  program_name = argv[0];
  have_read_stdin = 0;

  while ((c = getopt_long (argc, argv, "h:b:f:v:i:pl:s:w:n:d:", longopts,
			   (int *) 0)) != EOF)
    {
      switch (c)
	{
	case 0:
	  break;

	case 'h':
	  if (build_type_arg (&header_type, &header_regex) != TRUE)
	    usage (2);
	  break;
	case 'b':
	  if (build_type_arg (&body_type, &body_regex) != TRUE)
	    usage (2);
	  break;
	case 'f':
	  if (build_type_arg (&footer_type, &footer_regex) != TRUE)
	    usage (2);
	  break;
	case 'v':
	  page_start = atoi (optarg);
	  break;
	case 'i':
	  page_incr = atoi (optarg);
	  if (page_incr < 1)
	    page_incr = 1;
	  break;
	case 'p':
	  reset_numbers = FALSE;
	  break;
	case 'l':
	  blank_join = atoi (optarg);
	  break;
	case 's':
	  separator_str = optarg;
	  break;
	case 'w':
	  lineno_width = atoi (optarg);
	  if (lineno_width < 1)
	    lineno_width = 1;
	  break;
	case 'n':
	  switch (*optarg)
	    {
	    case 'l':
	      if (optarg[1] == 'n')
		lineno_format = FORMAT_LEFT;
	      else
		usage (2);
	      break;
	    case 'r':
	      switch (optarg[1])
		{
		case 'n':
		  lineno_format = FORMAT_RIGHT_NOLZ;
		  break;
		case 'z':
		  lineno_format = FORMAT_RIGHT_LZ;
		  break;
		default:
		  usage (2);
		  break;
		}
	      break;
	    default:
	      usage (2);
	      break;
	    }
	  break;
	case 'd':
	  section_del = optarg;
	  break;
	default:
	  usage (2);
	  break;
	}
    }

  if (show_version)
    {
      printf ("%s\n", version_string);
      exit (0);
    }

  if (show_help)
    usage (0);

  /* Initialize the section delimiters.  */
  c = strlen (section_del);

  header_del_len = c * 3;
  header_del = xmalloc (header_del_len + 1);
  strcat (strcat (strcpy (header_del, section_del), section_del), section_del);

  body_del_len = c * 2;
  body_del = xmalloc (body_del_len + 1);
  strcat (strcpy (body_del, section_del), section_del);

  footer_del_len = c;
  footer_del = xmalloc (footer_del_len + 1);
  strcpy (footer_del, section_del);

  /* Initialize the input buffer.  */
  initbuffer (&line_buf);

  /* Initialize the printf format for unnumbered lines. */
  c = strlen (separator_str);
  print_no_line_fmt = xmalloc (lineno_width + c + 1);
  memset (print_no_line_fmt, ' ', lineno_width + c);
  print_no_line_fmt[lineno_width + c] = '\0';

  line_no = page_start;
  current_type = body_type;
  current_regex = &body_regex;
  build_print_fmt ();

  /* Main processing. */

  if (optind == argc)
    exit_status |= nl_file ("-");
  else
    for (; optind < argc; optind++)
      exit_status |= nl_file (argv[optind]);

  if (have_read_stdin && fclose (stdin) == EOF)
    {
      error (0, errno, "-");
      exit_status = 1;
    }
  if (ferror (stdout) || fclose (stdout) == EOF)
    error (1, errno, "write error");

  exit (exit_status);
}

/* Process file FILE to standard output.
   Return 0 if successful, 1 if not. */

static int
nl_file (file)
     char *file;
{
  FILE *stream;

  if (!strcmp (file, "-"))
    {
      have_read_stdin = 1;
      stream = stdin;
    }
  else
    {
      stream = fopen (file, "r");
      if (stream == NULL)
	{
	  error (0, errno, "%s", file);
	  return 1;
	}
    }

  process_file (stream);

  if (ferror (stream))
    {
      error (0, errno, "%s", file);
      return 1;
    }
  if (!strcmp (file, "-"))
    clearerr (stream);		/* Also clear EOF. */
  else if (fclose (stream) == EOF)
    {
      error (0, errno, "%s", file);
      return 1;
    }
  return 0;
}

/* Read and process the file pointed to by FP. */

static void
process_file (fp)
     FILE *fp;
{
  while (readline (&line_buf, fp))
    {
      switch ((int) check_section ())
	{
	case Header:
	  proc_header ();
	  break;
	case Body:
	  proc_body ();
	  break;
	case Footer:
	  proc_footer ();
	  break;
	case Text:
	  proc_text ();
	  break;
	}
    }
}

/* Return the type of line in `line_buf'. */

static enum section
check_section ()
{
  if (line_buf.length < 2 || memcmp (line_buf.buffer, section_del, 2))
    return Text;
  if (line_buf.length == header_del_len
      && !memcmp (line_buf.buffer, header_del, header_del_len))
    return Header;
  if (line_buf.length == body_del_len
      && !memcmp (line_buf.buffer, body_del, body_del_len))
    return Body;
  if (line_buf.length == footer_del_len
      && !memcmp (line_buf.buffer, footer_del, footer_del_len))
    return Footer;
  return Text;
}

/* Switch to a header section. */

static void
proc_header ()
{
  current_type = header_type;
  current_regex = &header_regex;
  if (reset_numbers)
    line_no = page_start;
  putchar ('\n');
}

/* Switch to a body section. */

static void
proc_body ()
{
  current_type = body_type;
  current_regex = &body_regex;
  putchar ('\n');
}

/* Switch to a footer section. */

static void
proc_footer ()
{
  current_type = footer_type;
  current_regex = &footer_regex;
  putchar ('\n');
}

/* Process a regular text line in `line_buf'. */

static void
proc_text ()
{
  static int blank_lines = 0;	/* Consecutive blank lines so far. */

  switch (*current_type)
    {
    case 'a':
      if (blank_join > 1)
	{
	  if (line_buf.length || ++blank_lines == blank_join)
	    {
	      print_lineno ();
	      blank_lines = 0;
	    }
	  else
	    printf (print_no_line_fmt);
	}
      else
	print_lineno ();
      break;
    case 't':
      if (line_buf.length)
	print_lineno ();
      else
	printf (print_no_line_fmt);
      break;
    case 'n':
      printf (print_no_line_fmt);
      break;
    case 'p':
      if (re_search (current_regex, line_buf.buffer, line_buf.length,
		     0, line_buf.length, (struct re_registers *) 0) < 0)
	printf (print_no_line_fmt);
      else
	print_lineno ();
      break;
    }
  fwrite (line_buf.buffer, sizeof (char), line_buf.length, stdout);
  putchar ('\n');
}

/* Print and increment the line number. */

static void
print_lineno ()
{
  printf (print_fmt, line_no);
  line_no += page_incr;
}

/* Build the printf format string, based on `lineno_format'. */

static void
build_print_fmt ()
{
  /* 12 = 10 chars for lineno_width, 1 for %, 1 for \0.  */
  print_fmt = xmalloc (strlen (separator_str) + 12);
  switch (lineno_format)
    {
    case FORMAT_RIGHT_NOLZ:
      sprintf (print_fmt, "%%%dd%s", lineno_width, separator_str);
      break;
    case FORMAT_RIGHT_LZ:
      sprintf (print_fmt, "%%0%dd%s", lineno_width, separator_str);
      break;
    case FORMAT_LEFT:
      sprintf (print_fmt, "%%-%dd%s", lineno_width, separator_str);
      break;
    }
}

/* Set the command line flag TYPEP and possibly the regex pointer REGEXP,
   according to `optarg'.  */

static int
build_type_arg (typep, regexp)
     char **typep;
     struct re_pattern_buffer *regexp;
{
  const char *errmsg;
  int rval = TRUE;
  int optlen;

  switch (*optarg)
    {
    case 'a':
    case 't':
    case 'n':
      *typep = optarg;
      break;
    case 'p':
      *typep = optarg++;
      optlen = strlen (optarg);
      regexp->allocated = optlen * 2;
      regexp->buffer = (unsigned char *) xmalloc (regexp->allocated);
      regexp->translate = NULL;
      regexp->fastmap = xmalloc (256);
      regexp->fastmap_accurate = 0;
      errmsg = re_compile_pattern (optarg, optlen, regexp);
      if (errmsg)
	error (1, 0, "%s", errmsg);
      break;
    default:
      rval = FALSE;
      break;
    }
  return rval;
}

/* Print a usage message and quit. */

static void
usage (status)
     int status;
{
  if (status != 0)
    fprintf (stderr, "Try `%s --help' for more information.\n",
	     program_name);
  else
    {
      printf ("\
Usage: %s [OPTION]... [FILE]...\n\
",
	      program_name);
      printf ("\
\n\
  -b, --body-numbering=STYLE      use STYLE for numbering body lines\n\
  -d, --section-delimiter=CC      use CC for separating logical pages\n\
  -f, --footer-numbering=STYLE    use STYLE for numbering footer lines\n\
  -h, --header-numbering=STYLE    use STYLE for numbering header lines\n\
  -i, --page-increment=NUMBER     line number increment at each line\n\
  -l, --join-blank-lines=NUMBER   group of NUMBER empty lines counted as one\n\
  -n, --number-format=FORMAT      insert line numbers according to FORMAT\n\
  -p, --no-renumber               do not reset line numbers at logical pages\n\
  -s, --number-separator=STRING   add STRING after (possible) line number\n\
  -v, --first-page=NUMBER         first line number on each logical page\n\
  -w, --number-width=NUMBER       use NUMBER columns for line numbers\n\
      --help                      display this help and exit\n\
      --version                   output version information and exit\n\
\n\
By default, selects -v1 -i1 -l1 -sTAB -w6 -nrn -hn -bt -fn.  CC are\n\
two delimiter characters for separating logical pages, a missing\n\
second character implies :.  Type \\\\ for \\.  STYLE is one of:\n\
\n\
  a         number all lines\n\
  t         number only nonempty lines\n\
  n         number no lines\n\
  pREGEXP   number only lines that contain a match for REGEXP\n\
\n\
FORMAT is one of:\n\
\n\
  ln   left justified, no leading zeros\n\
  rn   right justified, no leading zeros\n\
  rz   right justified, leading zeros\n\
\n\
With no FILE, or when FILE is -, read standard input.\n\
");
    }
  exit (status);
}
