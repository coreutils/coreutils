/* tac - concatenate and print files in reverse
   Copyright (C) 1988-1991, 1995-2002 Free Software Foundation, Inc.

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

/* Written by Jay Lepreau (lepreau@cs.utah.edu).
   GNU enhancements by David MacKenzie (djm@gnu.ai.mit.edu). */

/* Copy each FILE, or the standard input if none are given or when a
   FILE name of "-" is encountered, to the standard output with the
   order of the records reversed.  The records are separated by
   instances of a string, or a newline if none is given.  By default, the
   separator string is attached to the end of the record that it
   follows in the file.

   Options:
   -b, --before			The separator is attached to the beginning
				of the record that it precedes in the file.
   -r, --regex			The separator is a regular expression.
   -s, --separator=separator	Use SEPARATOR as the record separator.

   To reverse a file byte by byte, use (in bash, ksh, or sh):
tac -r -s '.\|
' file */

#include <config.h>

#include <stdio.h>
#include <getopt.h>
#include <sys/types.h>
#include "system.h"
#include "closeout.h"

#include <regex.h>

#include "error.h"
#include "safe-read.h"

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "tac"

#define AUTHORS N_ ("Jay Lepreau and David MacKenzie")

#if defined __MSDOS__ || defined _WIN32
/* Define this to non-zero on systems for which the regular mechanism
   (of unlinking an open file and expecting to be able to write, seek
   back to the beginning, then reread it) doesn't work.  E.g., on Windows
   and DOS systems.  */
# define DONT_UNLINK_WHILE_OPEN 1
#endif


#ifndef DEFAULT_TMPDIR
# define DEFAULT_TMPDIR "/tmp"
#endif

/* The number of bytes per atomic read. */
#define INITIAL_READSIZE 8192

/* The number of bytes per atomic write. */
#define WRITESIZE 8192

/* The name this program was run with. */
char *program_name;

/* The string that separates the records of the file. */
static char *separator;

/* If nonzero, print `separator' along with the record preceding it
   in the file; otherwise with the record following it. */
static int separator_ends_record;

/* 0 if `separator' is to be matched as a regular expression;
   otherwise, the length of `separator', used as a sentinel to
   stop the search. */
static size_t sentinel_length;

/* The length of a match with `separator'.  If `sentinel_length' is 0,
   `match_length' is computed every time a match succeeds;
   otherwise, it is simply the length of `separator'. */
static int match_length;

/* The input buffer. */
static char *G_buffer;

/* The number of bytes to read at once into `buffer'. */
static size_t read_size;

/* The size of `buffer'.  This is read_size * 2 + sentinel_length + 2.
   The extra 2 bytes allow `past_end' to have a value beyond the
   end of `G_buffer' and `match_start' to run off the front of `G_buffer'. */
static unsigned G_buffer_size;

/* The compiled regular expression representing `separator'. */
static struct re_pattern_buffer compiled_separator;

static struct option const longopts[] =
{
  {"before", no_argument, NULL, 'b'},
  {"regex", no_argument, NULL, 'r'},
  {"separator", required_argument, NULL, 's'},
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
Usage: %s [OPTION]... [FILE]...\n\
"),
	      program_name);
      fputs (_("\
Write each FILE to standard output, last line first.\n\
With no FILE, or when FILE is -, read standard input.\n\
\n\
"), stdout);
      fputs (_("\
Mandatory arguments to long options are mandatory for short options too.\n\
"), stdout);
      fputs (_("\
  -b, --before             attach the separator before instead of after\n\
  -r, --regex              interpret the separator as a regular expression\n\
  -s, --separator=STRING   use STRING as the separator instead of newline\n\
"), stdout);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      printf (_("\nReport bugs to <%s>.\n"), PACKAGE_BUGREPORT);
    }
  exit (status == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}

/* Print the characters from START to PAST_END - 1.
   If START is NULL, just flush the buffer. */

static void
output (const char *start, const char *past_end)
{
  static char buffer[WRITESIZE];
  static size_t bytes_in_buffer = 0;
  size_t bytes_to_add = past_end - start;
  size_t bytes_available = WRITESIZE - bytes_in_buffer;

  if (start == 0)
    {
      fwrite (buffer, 1, bytes_in_buffer, stdout);
      bytes_in_buffer = 0;
      return;
    }

  /* Write out as many full buffers as possible. */
  while (bytes_to_add >= bytes_available)
    {
      memcpy (buffer + bytes_in_buffer, start, bytes_available);
      bytes_to_add -= bytes_available;
      start += bytes_available;
      fwrite (buffer, 1, WRITESIZE, stdout);
      bytes_in_buffer = 0;
      bytes_available = WRITESIZE;
    }

  memcpy (buffer + bytes_in_buffer, start, bytes_to_add);
  bytes_in_buffer += bytes_to_add;
}

/* Print in reverse the file open on descriptor FD for reading FILE.
   Return 0 if ok, 1 if an error occurs. */

static int
tac_seekable (int input_fd, const char *file)
{
  /* Pointer to the location in `G_buffer' where the search for
     the next separator will begin. */
  char *match_start;

  /* Pointer to one past the rightmost character in `G_buffer' that
     has not been printed yet. */
  char *past_end;

  /* Length of the record growing in `G_buffer'. */
  size_t saved_record_size;

  /* Offset in the file of the next read. */
  off_t file_pos;

  /* Nonzero if `output' has not been called yet for any file.
     Only used when the separator is attached to the preceding record. */
  int first_time = 1;
  char first_char = *separator;	/* Speed optimization, non-regexp. */
  char *separator1 = separator + 1; /* Speed optimization, non-regexp. */
  int match_length1 = match_length - 1; /* Speed optimization, non-regexp. */
  struct re_registers regs;

  /* Find the size of the input file. */
  file_pos = lseek (input_fd, (off_t) 0, SEEK_END);
  if (file_pos < 1)
    return 0;			/* It's an empty file. */

  /* Arrange for the first read to lop off enough to leave the rest of the
     file a multiple of `read_size'.  Since `read_size' can change, this may
     not always hold during the program run, but since it usually will, leave
     it here for i/o efficiency (page/sector boundaries and all that).
     Note: the efficiency gain has not been verified. */
  saved_record_size = file_pos % read_size;
  if (saved_record_size == 0)
    saved_record_size = read_size;
  file_pos -= saved_record_size;
  /* `file_pos' now points to the start of the last (probably partial) block
     in the input file. */

  if (lseek (input_fd, file_pos, SEEK_SET) < 0)
    error (0, errno, "%s: seek failed", file);

  if (safe_read (input_fd, G_buffer, saved_record_size) != saved_record_size)
    {
      error (0, errno, "%s", file);
      return 1;
    }

  match_start = past_end = G_buffer + saved_record_size;
  /* For non-regexp search, move past impossible positions for a match. */
  if (sentinel_length)
    match_start -= match_length1;

  for (;;)
    {
      /* Search backward from `match_start' - 1 to `G_buffer' for a match
	 with `separator'; for speed, use strncmp if `separator' contains no
	 metacharacters.
	 If the match succeeds, set `match_start' to point to the start of
	 the match and `match_length' to the length of the match.
	 Otherwise, make `match_start' < `G_buffer'. */
      if (sentinel_length == 0)
	{
	  int i = match_start - G_buffer;
	  int ret;

	  ret = re_search (&compiled_separator, G_buffer, i, i - 1, -i, &regs);
	  if (ret == -1)
	    match_start = G_buffer - 1;
	  else if (ret == -2)
	    {
	      error (EXIT_FAILURE, 0,
		     _("error in regular expression search"));
	    }
	  else
	    {
	      match_start = G_buffer + regs.start[0];
	      match_length = regs.end[0] - regs.start[0];
	    }
	}
      else
	{
	  /* `match_length' is constant for non-regexp boundaries. */
	  while (*--match_start != first_char
		 || (match_length1 && strncmp (match_start + 1, separator1,
					       match_length1)))
	    /* Do nothing. */ ;
	}

      /* Check whether we backed off the front of `G_buffer' without finding
         a match for `separator'. */
      if (match_start < G_buffer)
	{
	  if (file_pos == 0)
	    {
	      /* Hit the beginning of the file; print the remaining record. */
	      output (G_buffer, past_end);
	      return 0;
	    }

	  saved_record_size = past_end - G_buffer;
	  if (saved_record_size > read_size)
	    {
	      /* `G_buffer_size' is about twice `read_size', so since
		 we want to read in another `read_size' bytes before
		 the data already in `G_buffer', we need to increase
		 `G_buffer_size'. */
	      char *newbuffer;
	      int offset = sentinel_length ? sentinel_length : 1;

	      read_size *= 2;
	      G_buffer_size = read_size * 2 + sentinel_length + 2;
	      newbuffer = xrealloc (G_buffer - offset, G_buffer_size);
	      newbuffer += offset;
	      /* Adjust the pointers for the new buffer location.  */
	      match_start += newbuffer - G_buffer;
	      past_end += newbuffer - G_buffer;
	      G_buffer = newbuffer;
	    }

	  /* Back up to the start of the next bufferfull of the file.  */
	  if (file_pos >= read_size)
	    file_pos -= read_size;
	  else
	    {
	      read_size = file_pos;
	      file_pos = 0;
	    }
	  lseek (input_fd, file_pos, SEEK_SET);

	  /* Shift the pending record data right to make room for the new.
	     The source and destination regions probably overlap.  */
	  memmove (G_buffer + read_size, G_buffer, saved_record_size);
	  past_end = G_buffer + read_size + saved_record_size;
	  /* For non-regexp searches, avoid unneccessary scanning. */
	  if (sentinel_length)
	    match_start = G_buffer + read_size;
	  else
	    match_start = past_end;

	  if (safe_read (input_fd, G_buffer, read_size) != read_size)
	    {
	      error (0, errno, "%s", file);
	      return 1;
	    }
	}
      else
	{
	  /* Found a match of `separator'. */
	  if (separator_ends_record)
	    {
	      char *match_end = match_start + match_length;

	      /* If this match of `separator' isn't at the end of the
	         file, print the record. */
	      if (first_time == 0 || match_end != past_end)
		output (match_end, past_end);
	      past_end = match_end;
	      first_time = 0;
	    }
	  else
	    {
	      output (match_start, past_end);
	      past_end = match_start;
	    }

	  /* For non-regex matching, we can back up.  */
	  if (sentinel_length > 0)
	    match_start -= match_length - 1;
	}
    }
}

/* Print FILE in reverse.
   Return 0 if ok, 1 if an error occurs. */

static int
tac_file (const char *file)
{
  int errors;
  FILE *in;

  in = fopen (file, "r");
  if (in == NULL)
    {
      error (0, errno, "%s", file);
      return 1;
    }
  SET_BINARY (fileno (in));
  errors = tac_seekable (fileno (in), file);
  if (ferror (in) || fclose (in) == EOF)
    {
      error (0, errno, "%s", file);
      return 1;
    }
  return errors;
}

#if DONT_UNLINK_WHILE_OPEN

static const char *file_to_remove;
static FILE *fp_to_close;

static void
unlink_tempfile (void)
{
  fclose (fp_to_close);
  unlink (file_to_remove);
}

static void
record_tempfile (const char *fn, FILE *fp)
{
  if (!file_to_remove)
    {
      file_to_remove = fn;
      fp_to_close = fp;
      atexit (unlink_tempfile);
    }
}

#endif

/* Make a copy of the standard input in `FIXME'. */

static void
save_stdin (FILE **g_tmp, char **g_tempfile)
{
  static char *template = NULL;
  static char *tempdir;
  char *tempfile;
  FILE *tmp;
  int fd;

  if (template == NULL)
    {
      tempdir = getenv ("TMPDIR");
      if (tempdir == NULL)
	tempdir = DEFAULT_TMPDIR;
      template = xmalloc (strlen (tempdir) + 11);
    }
  sprintf (template, "%s/tacXXXXXX", tempdir);
  tempfile = template;
  fd = mkstemp (template);
  if (fd == -1)
    error (EXIT_FAILURE, errno, "%s", tempfile);

  tmp = fdopen (fd, "w+");
  if (tmp == NULL)
    error (EXIT_FAILURE, errno, "%s", tempfile);

#if DONT_UNLINK_WHILE_OPEN
  record_tempfile (tempfile, tmp);
#else
  unlink (tempfile);
#endif

  while (1)
    {
      size_t bytes_read = safe_read (STDIN_FILENO, G_buffer, read_size);
      if (bytes_read == 0)
	break;
      if (bytes_read == SAFE_READ_ERROR)
	error (EXIT_FAILURE, errno, _("stdin: read error"));

      if (fwrite (G_buffer, 1, bytes_read, tmp) != bytes_read)
	break;
    }

  if (ferror (tmp) || fflush (tmp) == EOF)
    error (EXIT_FAILURE, errno, "%s", tempfile);

  SET_BINARY (fileno (tmp));
  *g_tmp = tmp;
  *g_tempfile = tempfile;
}

/* Print the standard input in reverse, saving it to temporary
   file first if it is a pipe.
   Return 0 if ok, 1 if an error occurs. */

static int
tac_stdin (void)
{
  int errors;
  struct stat stats;

  /* No tempfile is needed for "tac < file".
     Use fstat instead of checking for errno == ESPIPE because
     lseek doesn't work on some special files but doesn't return an
     error, either. */
  if (fstat (STDIN_FILENO, &stats))
    {
      error (0, errno, _("standard input"));
      return 1;
    }

  if (S_ISREG (stats.st_mode))
    {
      errors = tac_seekable (fileno (stdin), _("standard input"));
    }
  else
    {
      FILE *tmp_stream;
      char *tmp_file;
      save_stdin (&tmp_stream, &tmp_file);
      errors = tac_seekable (fileno (tmp_stream), tmp_file);
    }

  return errors;
}

#if 0
/* BUF_END points one byte past the end of the buffer to be searched.  */

static void *
memrchr (const char *buf_start, const char *buf_end, int c)
{
  const char *p = buf_end;
  while (buf_start <= --p)
    {
      if (*(const unsigned char *) p == c)
	return (void *) p;
    }
  return NULL;
}

/* FIXME: describe */

static int
tac_mem (const char *buf, size_t n_bytes, FILE *out)
{
  const char *nl;
  const char *bol;

  if (n_bytes == 0)
    return 0;

  nl = memrchr (buf, buf + n_bytes, '\n');
  bol = (nl == NULL ? buf : nl + 1);

  /* If the last line of the input file has no terminating newline,
     treat it as a special case.  */
  if (bol < buf + n_bytes)
    {
      /* Print out the line from bol to end of input.  */
      fwrite (bol, 1, (buf + n_bytes) - bol, out);

      /* Add a newline here.  Otherwise, the first and second lines
	 of output would appear to have been joined.  */
      fputc ('\n', out);
    }

  while ((nl = memrchr (buf, bol - 1, '\n')) != NULL)
    {
      /* Output the line (which includes a trailing newline)
	 from NL+1 to BOL-1.  */
      fwrite (nl + 1, 1, bol - (nl + 1), out);

      bol = nl + 1;
    }

  /* If there's anything left, output the last line: BUF .. BOL-1.
     When the first byte of the input is a newline, there is nothing
     left to do here.  */
  if (buf < bol)
    fwrite (buf, 1, bol - buf, out);

  /* FIXME: this is work in progress.... */
  return ferror (out);
}

/* FIXME: describe */

static int
tac_stdin_to_mem (void)
{
  char *buf = NULL;
  size_t bufsiz = 8 * BUFSIZ;
  size_t delta = 8 * BUFSIZ;
  size_t n_bytes = 0;

  while (1)
    {
      size_t bytes_read;
      if (buf == NULL)
	buf = (char *) malloc (bufsiz);
      else
	buf = (char *) realloc (buf, bufsiz);

      if (buf == NULL)
	{
	  /* Free the buffer and fall back on the code that relies on a
	     temporary file.  */
	  free (buf);
	  /* FIXME */
	  abort ();
	}
      bytes_read = safe_read (STDIN_FILENO, buf + n_bytes, bufsiz - n_bytes);
      if (bytes_read == 0)
	break;
      if (bytes_read == SAFE_READ_ERROR)
	error (EXIT_FAILURE, errno, _("stdin: read error"));
      n_bytes += bytes_read;

      bufsiz += delta;
    }

  tac_mem (buf, n_bytes, stdout);

  return 0;
}
#endif

int
main (int argc, char **argv)
{
  const char *error_message;	/* Return value from re_compile_pattern. */
  int optc, errors;
  int have_read_stdin = 0;

  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  errors = 0;
  separator = "\n";
  sentinel_length = 1;
  separator_ends_record = 1;

  while ((optc = getopt_long (argc, argv, "brs:", longopts, NULL)) != -1)
    {
      switch (optc)
	{
	case 0:
	  break;
	case 'b':
	  separator_ends_record = 0;
	  break;
	case 'r':
	  sentinel_length = 0;
	  break;
	case 's':
	  separator = optarg;
	  if (*separator == 0)
	    error (EXIT_FAILURE, 0, _("separator cannot be empty"));
	  break;
	case_GETOPT_HELP_CHAR;
	case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);
	default:
	  usage (EXIT_FAILURE);
	}
    }

  if (sentinel_length == 0)
    {
      compiled_separator.allocated = 100;
      compiled_separator.buffer = (unsigned char *)
	xmalloc (compiled_separator.allocated);
      compiled_separator.fastmap = xmalloc (256);
      compiled_separator.translate = 0;
      error_message = re_compile_pattern (separator, strlen (separator),
					  &compiled_separator);
      if (error_message)
	error (EXIT_FAILURE, 0, "%s", error_message);
    }
  else
    match_length = sentinel_length = strlen (separator);

  read_size = INITIAL_READSIZE;
  /* A precaution that will probably never be needed. */
  while (sentinel_length * 2 >= read_size)
    read_size *= 2;
  G_buffer_size = read_size * 2 + sentinel_length + 2;
  G_buffer = xmalloc (G_buffer_size);
  if (sentinel_length)
    {
      strcpy (G_buffer, separator);
      G_buffer += sentinel_length;
    }
  else
    {
      ++G_buffer;
    }

  if (optind == argc)
    {
      have_read_stdin = 1;
      /* We need binary I/O, since `tac' relies
	 on `lseek' and byte counts.  */
      SET_BINARY2 (STDIN_FILENO, STDOUT_FILENO);
      errors = tac_stdin ();
    }
  else
    {
      for (; optind < argc; ++optind)
	{
	  if (STREQ (argv[optind], "-"))
	    {
	      have_read_stdin = 1;
	      SET_BINARY2 (STDIN_FILENO, STDOUT_FILENO);
	      errors |= tac_stdin ();
	    }
	  else
	    {
	      /* Binary output will leave the lines' ends (NL or
		 CR/LF) intact when the output is a disk file.
		 Writing a file with CR/LF pairs at end of lines in
		 text mode has no visible effect on console output,
		 since two CRs in a row are just like one CR.  */
	      SET_BINARY (STDOUT_FILENO);
	      errors |= tac_file (argv[optind]);
	    }
	}
    }

  /* Flush the output buffer. */
  output ((char *) NULL, (char *) NULL);

  if (have_read_stdin && close (STDIN_FILENO) < 0)
    error (EXIT_FAILURE, errno, "-");
  exit (errors == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}
