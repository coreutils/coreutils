/* tac - concatenate and print files in reverse
   Copyright (C) 88, 89, 90, 91, 95, 96, 1997 Free Software Foundation, Inc.

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
#include <signal.h>
#if WITH_REGEX
# include <regex.h>
#else
# include <rx.h>
#endif
#include "system.h"
#include "error.h"

#ifndef STDC_HEADERS
char *malloc ();
char *realloc ();
#endif

#ifndef DEFAULT_TMPDIR
#define DEFAULT_TMPDIR "/tmp"
#endif

/* The number of bytes per atomic read. */
#define INITIAL_READSIZE 8192

/* The number of bytes per atomic write. */
#define WRITESIZE 8192

char *mktemp ();

int full_write ();
int safe_read ();

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
static int sentinel_length;

/* The length of a match with `separator'.  If `sentinel_length' is 0,
   `match_length' is computed every time a match succeeds;
   otherwise, it is simply the length of `separator'. */
static int match_length;

/* The input buffer. */
static char *G_buffer;

/* The number of bytes to read at once into `buffer'. */
static unsigned read_size;

/* The size of `buffer'.  This is read_size * 2 + sentinel_length + 2.
   The extra 2 bytes allow `past_end' to have a value beyond the
   end of `G_buffer' and `match_start' to run off the front of `G_buffer'. */
static unsigned G_buffer_size;

/* The compiled regular expression representing `separator'. */
static struct re_pattern_buffer compiled_separator;

/* The name of a temporary file containing a copy of pipe input. */
static char *tempfile;

/* If nonzero, display usage information and exit.  */
static int show_help;

/* If nonzero, print the version on standard output then exit.  */
static int show_version;

static struct option const longopts[] =
{
  {"before", no_argument, &separator_ends_record, 0},
  {"regex", no_argument, &sentinel_length, 0},
  {"separator", required_argument, NULL, 's'},
  {"help", no_argument, &show_help, 1},
  {"version", no_argument, &show_version, 1},
  {NULL, 0, NULL, 0}
};

static void
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
      printf (_("\
Write each FILE to standard output, last line first.\n\
With no FILE, or when FILE is -, read standard input.\n\
\n\
  -b, --before             attach the separator before instead of after\n\
  -r, --regex              interpret the separator as a regular expression\n\
  -s, --separator=STRING   use STRING as the separator instead of newline\n\
      --help               display this help and exit\n\
      --version            output version information and exit\n\
"));
      puts (_("\nReport bugs to <textutils-bugs@gnu.ai.mit.edu>."));
    }
  exit (status == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}

static void
cleanup (void)
{
  unlink (tempfile);
}

static void
cleanup_fatal (void)
{
  cleanup ();
  exit (EXIT_FAILURE);
}

static RETSIGTYPE
sighandler (int sig)
{
#ifdef SA_INTERRUPT
  struct sigaction sigact;

  sigact.sa_handler = SIG_DFL;
  sigemptyset (&sigact.sa_mask);
  sigact.sa_flags = 0;
  sigaction (sig, &sigact, NULL);
#else				/* !SA_INTERRUPT */
  signal (sig, SIG_DFL);
#endif				/* SA_INTERRUPT */
  cleanup ();
  kill (getpid (), sig);
}

/* Allocate N bytes of memory dynamically, with error checking.  */

static char *
xmalloc (unsigned int n)
{
  char *p;

  p = malloc (n);
  if (p == 0)
    {
      error (0, 0, _("virtual memory exhausted"));
      cleanup_fatal ();
    }
  return p;
}

/* Change the size of memory area P to N bytes, with error checking. */

static char *
xrealloc (char *p, unsigned int n)
{
  p = realloc (p, n);
  if (p == 0)
    {
      error (0, 0, _("virtual memory exhausted"));
      cleanup_fatal ();
    }
  return p;
}

static void
xwrite (int desc, const char *buffer, int size)
{
  if (full_write (desc, buffer, size) < 0)
    {
      error (0, errno, _("write error"));
      cleanup_fatal ();
    }
}

/* Print the characters from START to PAST_END - 1.
   If START is NULL, just flush the buffer. */

static void
output (const char *start, const char *past_end)
{
  static char buffer[WRITESIZE];
  static int bytes_in_buffer = 0;
  int bytes_to_add = past_end - start;
  int bytes_available = WRITESIZE - bytes_in_buffer;

  if (start == 0)
    {
      xwrite (STDOUT_FILENO, buffer, bytes_in_buffer);
      bytes_in_buffer = 0;
      return;
    }

  /* Write out as many full buffers as possible. */
  while (bytes_to_add >= bytes_available)
    {
      memcpy (buffer + bytes_in_buffer, start, bytes_available);
      bytes_to_add -= bytes_available;
      start += bytes_available;
      xwrite (STDOUT_FILENO, buffer, WRITESIZE);
      bytes_in_buffer = 0;
      bytes_available = WRITESIZE;
    }

  memcpy (buffer + bytes_in_buffer, start, bytes_to_add);
  bytes_in_buffer += bytes_to_add;
}

/* Print in reverse the file open on descriptor FD for reading FILE.
   Return 0 if ok, 1 if an error occurs. */

static int
tac (int fd, const char *file)
{
  /* Pointer to the location in `G_buffer' where the search for
     the next separator will begin. */
  char *match_start;
  /* Pointer to one past the rightmost character in `G_buffer' that
     has not been printed yet. */
  char *past_end;
  unsigned saved_record_size;	/* Length of the record growing in `G_buffer'. */
  off_t file_pos;		/* Offset in the file of the next read. */
  /* Nonzero if `output' has not been called yet for any file.
     Only used when the separator is attached to the preceding record. */
  int first_time = 1;
  char first_char = *separator;	/* Speed optimization, non-regexp. */
  char *separator1 = separator + 1; /* Speed optimization, non-regexp. */
  int match_length1 = match_length - 1; /* Speed optimization, non-regexp. */
  struct re_registers regs;

  /* Find the size of the input file. */
  file_pos = lseek (fd, (off_t) 0, SEEK_END);
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

  lseek (fd, file_pos, SEEK_SET);
  if (safe_read (fd, G_buffer, saved_record_size) != saved_record_size)
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
	      error (0, 0, _("error in regular expression search"));
	      cleanup_fatal ();
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
	      newbuffer = xrealloc (G_buffer - offset, G_buffer_size) + offset;
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
	  lseek (fd, file_pos, SEEK_SET);

	  /* Shift the pending record data right to make room for the new.
	     The source and destination regions probably overlap.  */
	  memmove (G_buffer + read_size, G_buffer, saved_record_size);
	  past_end = G_buffer + read_size + saved_record_size;
	  /* For non-regexp searches, avoid unneccessary scanning. */
	  if (sentinel_length)
	    match_start = G_buffer + read_size;
	  else
	    match_start = past_end;

	  if (safe_read (fd, G_buffer, read_size) != read_size)
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
	  match_start -= match_length - 1;
	}
    }
}

/* Print FILE in reverse.
   Return 0 if ok, 1 if an error occurs. */

static int
tac_file (const char *file)
{
  int fd, errors;

  fd = open (file, O_RDONLY);
  if (fd == -1)
    {
      error (0, errno, "%s", file);
      return 1;
    }
  errors = tac (fd, file);
  if (close (fd) < 0)
    {
      error (0, errno, "%s", file);
      return 1;
    }
  return errors;
}

/* Make a copy of the standard input in `tempfile'. */

static void
save_stdin (void)
{
  static char *template = NULL;
  static char *tempdir;
  int fd;
  int bytes_read;

  if (template == NULL)
    {
      tempdir = getenv ("TMPDIR");
      if (tempdir == NULL)
	tempdir = DEFAULT_TMPDIR;
      template = xmalloc (strlen (tempdir) + 11);
    }
  sprintf (template, "%s/tacXXXXXX", tempdir);
  tempfile = mktemp (template);

  fd = creat (tempfile, 0600);
  if (fd == -1)
    {
      error (0, errno, "%s", tempfile);
      cleanup_fatal ();
    }
  while ((bytes_read = safe_read (0, G_buffer, read_size)) > 0)
    if (full_write (fd, G_buffer, bytes_read) < 0)
      {
	error (0, errno, "%s", tempfile);
	cleanup_fatal ();
      }
  if (close (fd) < 0)
    {
      error (0, errno, "%s", tempfile);
      cleanup_fatal ();
    }
  if (bytes_read == -1)
    {
      error (0, errno, _("read error"));
      cleanup_fatal ();
    }
}

/* Print the standard input in reverse, saving it to temporary
   file `tempfile' first if it is a pipe.
   Return 0 if ok, 1 if an error occurs. */

static int
tac_stdin (void)
{
  /* Previous values of signal handlers. */
  RETSIGTYPE (*sigint) (), (*sighup) (), (*sigpipe) (), (*sigterm) ();
  int errors;
  struct stat stats;
#ifdef SA_INTERRUPT
    struct sigaction oldact, newact;
#endif				/* SA_INTERRUPT */

  /* No tempfile is needed for "tac < file".
     Use fstat instead of checking for errno == ESPIPE because
     lseek doesn't work on some special files but doesn't return an
     error, either. */
  if (fstat (0, &stats))
    {
      error (0, errno, _("standard input"));
      return 1;
    }
  if (S_ISREG (stats.st_mode))
    return tac (0, _("standard input"));

#ifdef SA_INTERRUPT
  newact.sa_handler = sighandler;
  sigemptyset (&newact.sa_mask);
  newact.sa_flags = 0;

  sigaction (SIGINT, NULL, &oldact);
  sigint = oldact.sa_handler;
  if (sigint != SIG_IGN)
    sigaction (SIGINT, &newact, NULL);

  sigaction (SIGHUP, NULL, &oldact);
  sighup = oldact.sa_handler;
  if (sighup != SIG_IGN)
    sigaction (SIGHUP, &newact, NULL);

  sigaction (SIGPIPE, NULL, &oldact);
  sigpipe = oldact.sa_handler;
  if (sigpipe != SIG_IGN)
    sigaction (SIGPIPE, &newact, NULL);

  sigaction (SIGTERM, NULL, &oldact);
  sigterm = oldact.sa_handler;
  if (sigterm != SIG_IGN)
    sigaction (SIGTERM, &newact, NULL);
#else				/* !SA_INTERRUPT */
  sigint = signal (SIGINT, SIG_IGN);
  if (sigint != SIG_IGN)
    signal (SIGINT, sighandler);

  sighup = signal (SIGHUP, SIG_IGN);
  if (sighup != SIG_IGN)
    signal (SIGHUP, sighandler);

  sigpipe = signal (SIGPIPE, SIG_IGN);
  if (sigpipe != SIG_IGN)
    signal (SIGPIPE, sighandler);

  sigterm = signal (SIGTERM, SIG_IGN);
  if (sigterm != SIG_IGN)
    signal (SIGTERM, sighandler);
#endif				/* SA_INTERRUPT */

  save_stdin ();

  errors = tac_file (tempfile);

  unlink (tempfile);

#ifdef SA_INTERRUPT
  newact.sa_handler = sigint;
  sigaction (SIGINT, &newact, NULL);
  newact.sa_handler = sighup;
  sigaction (SIGHUP, &newact, NULL);
  newact.sa_handler = sigterm;
  sigaction (SIGTERM, &newact, NULL);
  newact.sa_handler = sigpipe;
  sigaction (SIGPIPE, &newact, NULL);
#else				/* !SA_INTERRUPT */
  signal (SIGINT, sigint);
  signal (SIGHUP, sighup);
  signal (SIGTERM, sigterm);
  signal (SIGPIPE, sigpipe);
#endif				/* SA_INTERRUPT */

  return errors;
}

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
	default:
	  usage (1);
	}
    }

  if (show_version)
    {
      printf ("tac (%s) %s\n", GNU_PACKAGE, VERSION);
      exit (EXIT_SUCCESS);
    }

  if (show_help)
    usage (0);

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
    ++G_buffer;

  if (optind == argc)
    {
      have_read_stdin = 1;
      errors = tac_stdin ();
    }
  else
    for (; optind < argc; ++optind)
      {
	if (strcmp (argv[optind], "-") == 0)
	  {
	    have_read_stdin = 1;
	    errors |= tac_stdin ();
	  }
	else
	  errors |= tac_file (argv[optind]);
      }

  /* Flush the output buffer. */
  output ((char *) NULL, (char *) NULL);

  if (have_read_stdin && close (0) < 0)
    error (EXIT_FAILURE, errno, "-");
  if (close (1) < 0)
    error (EXIT_FAILURE, errno, _("write error"));
  exit (errors == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}
