/* tail -- output the last part of file(s)
   Copyright (C) 1989, 1990, 1991 Free Software Foundation, Inc.

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

/* Can display any amount of data, unlike the Unix version, which uses
   a fixed size buffer and therefore can only deliver a limited number
   of lines.

   Options:
   -b			Tail by N 512-byte blocks.
   -c, --bytes=N[bkm]	Tail by N bytes
			[or 512-byte blocks, kilobytes, or megabytes].
   -f, --follow		Loop forever trying to read more characters at the
			end of the file, on the assumption that the file
			is growing.  Ignored if reading from a pipe.
   -k			Tail by N kilobytes.
   -N, -l, -n, --lines=N	Tail by N lines.
   -m			Tail by N megabytes.
   -q, --quiet, --silent	Never print filename headers.
   -v, --verbose		Always print filename headers.

   If a number (N) starts with a `+', begin printing with the Nth item
   from the start of each file, instead of from the end.

   Reads from standard input if no files are given or when a filename of
   ``-'' is encountered.
   By default, filename headers are printed only more than one file
   is given.
   By default, prints the last 10 lines (tail -n 10).

   Original version by Paul Rubin <phr@ocf.berkeley.edu>.
   Extensions by David MacKenzie <djm@gnu.ai.mit.edu>.
   tail -f for multiple files by Ian Lance Taylor <ian@cygnus.com>.  */

#include <stdio.h>
#include <getopt.h>
#include <ctype.h>
#include <sys/types.h>
#include <signal.h>
#include "system.h"

#ifdef isascii
#define ISDIGIT(c) (isascii ((c)) && isdigit ((c)))
#else
#define ISDIGIT(c) (isdigit ((c)))
#endif

/* Number of items to tail. */
#define DEFAULT_NUMBER 10

/* Size of atomic reads. */
#define BUFSIZE (512 * 8)

/* Number of bytes per item we are printing.
   If 0, tail in lines. */
static int unit_size;

/* If nonzero, read from the end of one file until killed. */
static int forever;

/* If nonzero, read from the end of multiple files until killed.  */
static int forever_multiple;

/* Array of file descriptors if forever_multiple is 1.  */
static int *file_descs;

/* If nonzero, count from start of file instead of end. */
static int from_start;

/* If nonzero, print filename headers. */
static int print_headers;

/* When to print the filename banners. */
enum header_mode
{
  multiple_files, always, never
};

char *xmalloc ();
void xwrite ();
void error ();

static int file_lines ();
static int pipe_bytes ();
static int pipe_lines ();
static int start_bytes ();
static int start_lines ();
static int tail ();
static int tail_bytes ();
static int tail_file ();
static int tail_lines ();
static long atou();
static void dump_remainder ();
static void tail_forever ();
static void parse_unit ();
static void usage ();
static void write_header ();

/* The name this program was run with. */
char *program_name;

/* Nonzero if we have ever read standard input. */
static int have_read_stdin;

static struct option const long_options[] =
{
  {"bytes", required_argument, NULL, 'c'},
  {"follow", no_argument, NULL, 'f'},
  {"lines", required_argument, NULL, 'n'},
  {"quiet", no_argument, NULL, 'q'},
  {"silent", no_argument, NULL, 'q'},
  {"verbose", no_argument, NULL, 'v'},
  {NULL, 0, NULL, 0}
};

void
main (argc, argv)
     int argc;
     char **argv;
{
  enum header_mode header_mode = multiple_files;
  int exit_status = 0;
  /* If from_start, the number of items to skip before printing; otherwise,
     the number of items at the end of the file to print.  Initially, -1
     means the value has not been set. */
  long number = -1;
  int c;			/* Option character. */
  int fileind;			/* Index in ARGV of first file name.  */

  program_name = argv[0];
  have_read_stdin = 0;
  unit_size = 0;
  forever = forever_multiple = from_start = print_headers = 0;

  if (argc > 1
      && ((argv[1][0] == '-' && ISDIGIT (argv[1][1]))
	  || (argv[1][0] == '+' && (ISDIGIT (argv[1][1]) || argv[1][1] == 0))))
    {
      /* Old option syntax: a dash or plus, one or more digits (zero digits
	 are acceptable with a plus), and one or more option letters. */
      if (argv[1][0] == '+')
	from_start = 1;
      if (argv[1][1] != 0)
	{
	  for (number = 0, ++argv[1]; ISDIGIT (*argv[1]); ++argv[1])
	    number = number * 10 + *argv[1] - '0';
	  /* Parse any appended option letters. */
	  while (*argv[1])
	    {
	      switch (*argv[1])
		{
		case 'b':
		  unit_size = 512;
		  break;

		case 'c':
		  unit_size = 1;
		  break;

		case 'f':
		  forever = 1;
		  break;

		case 'k':
		  unit_size = 1024;
		  break;

		case 'l':
		  unit_size = 0;
		  break;

		case 'm':
		  unit_size = 1048576;
		  break;

		case 'q':
		  header_mode = never;
		  break;

		case 'v':
		  header_mode = always;
		  break;

		default:
		  error (0, 0, "unrecognized option `-%c'", *argv[1]);
		  usage ();
		}
	      ++argv[1];
	    }
	}
      /* Make the options we just parsed invisible to getopt. */
      argv[1] = argv[0];
      argv++;
      argc--;
    }

  while ((c = getopt_long (argc, argv, "c:n:fqv", long_options, (int *) 0))
	 != EOF)
    {
      switch (c)
	{
	case 'c':
	  unit_size = 1;
	  parse_unit (optarg);
	  goto getnum;
	case 'n':
	  unit_size = 0;
	getnum:
	  if (*optarg == '+')
	    {
	      from_start = 1;
	      ++optarg;
	    }
	  else if (*optarg == '-')
	    ++optarg;
	  number = atou (optarg);
	  if (number == -1)
	    error (1, 0, "invalid number `%s'", optarg);
	  break;

	case 'f':
	  forever = 1;
	  break;

	case 'q':
	  header_mode = never;
	  break;

	case 'v':
	  header_mode = always;
	  break;

	default:
	  usage ();
	}
    }

  if (number == -1)
    number = DEFAULT_NUMBER;

  /* To start printing with item `number' from the start of the file, skip
     `number' - 1 items.  `tail +0' is actually meaningless, but for Unix
     compatibility it's treated the same as `tail +1'. */
  if (from_start)
    {
      if (number)
	--number;
    }

  if (unit_size > 1)
    number *= unit_size;

  fileind = optind;

  if (optind < argc - 1 && forever)
    {
      forever_multiple = 1;
      forever = 0;
      file_descs = (int *) xmalloc ((argc - optind) * sizeof (int));
    }

  if (header_mode == always
      || (header_mode == multiple_files && optind < argc - 1))
    print_headers = 1;

  if (optind == argc)
    exit_status |= tail_file ("-", number, 0);

  for (; optind < argc; ++optind)
    exit_status |= tail_file (argv[optind], number, optind - fileind);

  if (forever_multiple)
    tail_forever (argv + fileind, argc - fileind);

  if (have_read_stdin && close (0) < 0)
    error (1, errno, "-");
  if (close (1) < 0)
    error (1, errno, "write error");
  exit (exit_status);
}

/* Display the last NUMBER units of file FILENAME.
   "-" for FILENAME means the standard input.
   FILENUM is this file's index in the list of files the user gave.
   Return 0 if successful, 1 if an error occurred. */

static int
tail_file (filename, number, filenum)
     char *filename;
     long number;
     int filenum;
{
  int fd, errors;

  if (!strcmp (filename, "-"))
    {
      have_read_stdin = 1;
      filename = "standard input";
      if (print_headers)
	write_header (filename);
      errors = tail (filename, 0, number);
      if (forever_multiple)
	file_descs[filenum] = errors ? -1 : 0;
    }
  else
    {
      /* Not standard input.  */
      fd = open (filename, O_RDONLY);
      if (fd == -1)
	{
	  if (forever_multiple)
	    file_descs[filenum] = -1;
	  error (0, errno, "%s", filename);
	  errors = 1;
	}
      else
	{
	  if (print_headers)
	    write_header (filename);
	  errors = tail (filename, fd, number);
	  if (forever_multiple)
	    {
	      if (errors)
		{
		  close (fd);
		  file_descs[filenum] = -1;
		}
	      else
		file_descs[filenum] = fd;
	    }
	  else
	    {
	      if (close (fd))
		{
		  error (0, errno, "%s", filename);
		  errors = 1;
		}
	    }
	}
    }

  return errors;
}

static void
write_header (filename)
     char *filename;
{
  static int first_file = 1;

  if (first_file)
    {
      xwrite (1, "==> ", 4);
      first_file = 0;
    }
  else
    xwrite (1, "\n==> ", 5);
  xwrite (1, filename, strlen (filename));
  xwrite (1, " <==\n", 5);
}

/* Display the last NUMBER units of file FILENAME, open for reading
   in FD.
   Return 0 if successful, 1 if an error occurred. */

static int
tail (filename, fd, number)
     char *filename;
     int fd;
     long number;
{
  if (unit_size)
    return tail_bytes (filename, fd, number);
  else
    return tail_lines (filename, fd, number);
}

/* Display the last part of file FILENAME, open for reading in FD,
   using NUMBER characters.
   Return 0 if successful, 1 if an error occurred. */

static int
tail_bytes (filename, fd, number)
     char *filename;
     int fd;
     long number;
{
  struct stat stats;

  /* Use fstat instead of checking for errno == ESPIPE because
     lseek doesn't work on some special files but doesn't return an
     error, either. */
  if (fstat (fd, &stats))
    {
      error (0, errno, "%s", filename);
      return 1;
    }

  if (from_start)
    {
      if (S_ISREG (stats.st_mode))
	lseek (fd, number, SEEK_SET);
      else if (start_bytes (filename, fd, number))
	return 1;
      dump_remainder (filename, fd);
    }
  else
    {
      if (S_ISREG (stats.st_mode))
	{
	  if (lseek (fd, 0L, SEEK_END) <= number)
	    /* The file is shorter than we want, or just the right size, so
	       print the whole file. */
	    lseek (fd, 0L, SEEK_SET);
	  else
	    /* The file is longer than we want, so go back. */
	    lseek (fd, -number, SEEK_END);
	  dump_remainder (filename, fd);
	}
      else
	return pipe_bytes (filename, fd, number);
    }
  return 0;
}

/* Display the last part of file FILENAME, open for reading on FD,
   using NUMBER lines.
   Return 0 if successful, 1 if an error occurred. */

static int
tail_lines (filename, fd, number)
     char *filename;
     int fd;
     long number;
{
  struct stat stats;
  long length;

  if (fstat (fd, &stats))
    {
      error (0, errno, "%s", filename);
      return 1;
    }

  if (from_start)
    {
      if (start_lines (filename, fd, number))
	return 1;
      dump_remainder (filename, fd);
    }
  else
    {
      if (S_ISREG (stats.st_mode))
	{
	  length = lseek (fd, 0L, SEEK_END);
	  if (length != 0 && file_lines (filename, fd, number, length))
	    return 1;
	  dump_remainder (filename, fd);
	}
      else
	return pipe_lines (filename, fd, number);
    }
  return 0;
}

/* Print the last NUMBER lines from the end of file FD.
   Go backward through the file, reading `BUFSIZE' bytes at a time (except
   probably the first), until we hit the start of the file or have
   read NUMBER newlines.
   POS starts out as the length of the file (the offset of the last
   byte of the file + 1).
   Return 0 if successful, 1 if an error occurred. */

static int
file_lines (filename, fd, number, pos)
     char *filename;
     int fd;
     long number;
     long pos;
{
  char buffer[BUFSIZE];
  int bytes_read;
  int i;			/* Index into `buffer' for scanning. */

  if (number == 0)
    return 0;

  /* Set `bytes_read' to the size of the last, probably partial, buffer;
     0 < `bytes_read' <= `BUFSIZE'. */
  bytes_read = pos % BUFSIZE;
  if (bytes_read == 0)
    bytes_read = BUFSIZE;
  /* Make `pos' a multiple of `BUFSIZE' (0 if the file is short), so that all
     reads will be on block boundaries, which might increase efficiency. */
  pos -= bytes_read;
  lseek (fd, pos, SEEK_SET);
  bytes_read = read (fd, buffer, bytes_read);
  if (bytes_read == -1)
    {
      error (0, errno, "%s", filename);
      return 1;
    }

  /* Count the incomplete line on files that don't end with a newline. */
  if (bytes_read && buffer[bytes_read - 1] != '\n')
    --number;

  do
    {
      /* Scan backward, counting the newlines in this bufferfull. */
      for (i = bytes_read - 1; i >= 0; i--)
	{
	  /* Have we counted the requested number of newlines yet? */
	  if (buffer[i] == '\n' && number-- == 0)
	    {
	      /* If this newline wasn't the last character in the buffer,
	         print the text after it. */
	      if (i != bytes_read - 1)
		xwrite (1, &buffer[i + 1], bytes_read - (i + 1));
	      return 0;
	    }
	}
      /* Not enough newlines in that bufferfull. */
      if (pos == 0)
	{
	  /* Not enough lines in the file; print the entire file. */
	  lseek (fd, 0L, SEEK_SET);
	  return 0;
	}
      pos -= BUFSIZE;
      lseek (fd, pos, SEEK_SET);
    }
  while ((bytes_read = read (fd, buffer, BUFSIZE)) > 0);
  if (bytes_read == -1)
    {
      error (0, errno, "%s", filename);
      return 1;
    }
  return 0;
}

/* Print the last NUMBER lines from the end of the standard input,
   open for reading as pipe FD.
   Buffer the text as a linked list of LBUFFERs, adding them as needed.
   Return 0 if successful, 1 if an error occured. */

static int
pipe_lines (filename, fd, number)
     char *filename;
     int fd;
     long number;
{
  struct linebuffer
  {
    int nbytes, nlines;
    char buffer[BUFSIZE];
    struct linebuffer *next;
  };
  typedef struct linebuffer LBUFFER;
  LBUFFER *first, *last, *tmp;
  int i;			/* Index into buffers. */
  int total_lines = 0;		/* Total number of newlines in all buffers. */
  int errors = 0;

  first = last = (LBUFFER *) xmalloc (sizeof (LBUFFER));
  first->nbytes = first->nlines = 0;
  first->next = NULL;
  tmp = (LBUFFER *) xmalloc (sizeof (LBUFFER));

  /* Input is always read into a fresh buffer. */
  while ((tmp->nbytes = read (fd, tmp->buffer, BUFSIZE)) > 0)
    {
      tmp->nlines = 0;
      tmp->next = NULL;

      /* Count the number of newlines just read. */
      for (i = 0; i < tmp->nbytes; i++)
	if (tmp->buffer[i] == '\n')
	  ++tmp->nlines;
      total_lines += tmp->nlines;

      /* If there is enough room in the last buffer read, just append the new
         one to it.  This is because when reading from a pipe, `nbytes' can
         often be very small. */
      if (tmp->nbytes + last->nbytes < BUFSIZE)
	{
	  bcopy (tmp->buffer, &last->buffer[last->nbytes], tmp->nbytes);
	  last->nbytes += tmp->nbytes;
	  last->nlines += tmp->nlines;
	}
      else
	{
	  /* If there's not enough room, link the new buffer onto the end of
	     the list, then either free up the oldest buffer for the next
	     read if that would leave enough lines, or else malloc a new one.
	     Some compaction mechanism is possible but probably not
	     worthwhile. */
	  last = last->next = tmp;
	  if (total_lines - first->nlines > number)
	    {
	      tmp = first;
	      total_lines -= first->nlines;
	      first = first->next;
	    }
	  else
	    tmp = (LBUFFER *) xmalloc (sizeof (LBUFFER));
	}
    }
  if (tmp->nbytes == -1)
    {
      error (0, errno, "%s", filename);
      errors = 1;
      free ((char *) tmp);
      goto free_lbuffers;
    }

  free ((char *) tmp);

  /* This prevents a core dump when the pipe contains no newlines. */
  if (number == 0)
    goto free_lbuffers;

  /* Count the incomplete line on files that don't end with a newline. */
  if (last->buffer[last->nbytes - 1] != '\n')
    {
      ++last->nlines;
      ++total_lines;
    }

  /* Run through the list, printing lines.  First, skip over unneeded
     buffers. */
  for (tmp = first; total_lines - tmp->nlines > number; tmp = tmp->next)
    total_lines -= tmp->nlines;

  /* Find the correct beginning, then print the rest of the file. */
  if (total_lines > number)
    {
      char *cp;

      /* Skip `total_lines' - `number' newlines.  We made sure that
         `total_lines' - `number' <= `tmp->nlines'. */
      cp = tmp->buffer;
      for (i = total_lines - number; i; --i)
	while (*cp++ != '\n')
	  /* Do nothing. */ ;
      i = cp - tmp->buffer;
    }
  else
    i = 0;
  xwrite (1, &tmp->buffer[i], tmp->nbytes - i);

  for (tmp = tmp->next; tmp; tmp = tmp->next)
    xwrite (1, tmp->buffer, tmp->nbytes);

free_lbuffers:
  while (first)
    {
      tmp = first->next;
      free ((char *) first);
      first = tmp;
    }
  return errors;
}

/* Print the last NUMBER characters from the end of pipe FD.
   This is a stripped down version of pipe_lines.
   Return 0 if successful, 1 if an error occurred. */

static int
pipe_bytes (filename, fd, number)
     char *filename;
     int fd;
     long number;
{
  struct charbuffer
  {
    int nbytes;
    char buffer[BUFSIZE];
    struct charbuffer *next;
  };
  typedef struct charbuffer CBUFFER;
  CBUFFER *first, *last, *tmp;
  int i;			/* Index into buffers. */
  int total_bytes = 0;		/* Total characters in all buffers. */
  int errors = 0;

  first = last = (CBUFFER *) xmalloc (sizeof (CBUFFER));
  first->nbytes = 0;
  first->next = NULL;
  tmp = (CBUFFER *) xmalloc (sizeof (CBUFFER));

  /* Input is always read into a fresh buffer. */
  while ((tmp->nbytes = read (fd, tmp->buffer, BUFSIZE)) > 0)
    {
      tmp->next = NULL;

      total_bytes += tmp->nbytes;
      /* If there is enough room in the last buffer read, just append the new
         one to it.  This is because when reading from a pipe, `nbytes' can
         often be very small. */
      if (tmp->nbytes + last->nbytes < BUFSIZE)
	{
	  bcopy (tmp->buffer, &last->buffer[last->nbytes], tmp->nbytes);
	  last->nbytes += tmp->nbytes;
	}
      else
	{
	  /* If there's not enough room, link the new buffer onto the end of
	     the list, then either free up the oldest buffer for the next
	     read if that would leave enough characters, or else malloc a new
	     one.  Some compaction mechanism is possible but probably not
	     worthwhile. */
	  last = last->next = tmp;
	  if (total_bytes - first->nbytes > number)
	    {
	      tmp = first;
	      total_bytes -= first->nbytes;
	      first = first->next;
	    }
	  else
	    {
	      tmp = (CBUFFER *) xmalloc (sizeof (CBUFFER));
	    }
	}
    }
  if (tmp->nbytes == -1)
    {
      error (0, errno, "%s", filename);
      errors = 1;
      free ((char *) tmp);
      goto free_cbuffers;
    }

  free ((char *) tmp);

  /* Run through the list, printing characters.  First, skip over unneeded
     buffers. */
  for (tmp = first; total_bytes - tmp->nbytes > number; tmp = tmp->next)
    total_bytes -= tmp->nbytes;

  /* Find the correct beginning, then print the rest of the file.
     We made sure that `total_bytes' - `number' <= `tmp->nbytes'. */
  if (total_bytes > number)
    i = total_bytes - number;
  else
    i = 0;
  xwrite (1, &tmp->buffer[i], tmp->nbytes - i);

  for (tmp = tmp->next; tmp; tmp = tmp->next)
    xwrite (1, tmp->buffer, tmp->nbytes);

free_cbuffers:
  while (first)
    {
      tmp = first->next;
      free ((char *) first);
      first = tmp;
    }
  return errors;
}

/* Skip NUMBER characters from the start of pipe FD, and print
   any extra characters that were read beyond that.
   Return 1 on error, 0 if ok.  */

static int
start_bytes (filename, fd, number)
     char *filename;
     int fd;
     long number;
{
  char buffer[BUFSIZE];
  int bytes_read = 0;

  while (number > 0 && (bytes_read = read (fd, buffer, BUFSIZE)) > 0)
    number -= bytes_read;
  if (bytes_read == -1)
    {
      error (0, errno, "%s", filename);
      return 1;
    }
  else if (number < 0)
    xwrite (1, &buffer[bytes_read + number], -number);
  return 0;
}

/* Skip NUMBER lines at the start of file or pipe FD, and print
   any extra characters that were read beyond that.
   Return 1 on error, 0 if ok.  */

static int
start_lines (filename, fd, number)
     char *filename;
     int fd;
     long number;
{
  char buffer[BUFSIZE];
  int bytes_read = 0;
  int bytes_to_skip = 0;

  while (number && (bytes_read = read (fd, buffer, BUFSIZE)) > 0)
    {
      bytes_to_skip = 0;
      while (bytes_to_skip < bytes_read)
	if (buffer[bytes_to_skip++] == '\n' && --number == 0)
	  break;
    }
  if (bytes_read == -1)
    {
      error (0, errno, "%s", filename);
      return 1;
    }
  else if (bytes_to_skip < bytes_read)
    xwrite (1, &buffer[bytes_to_skip], bytes_read - bytes_to_skip);
  return 0;
}

/* Display file FILENAME from the current position in FD
   to the end.  If `forever' is nonzero, keep reading from the
   end of the file until killed. */

static void
dump_remainder (filename, fd)
     char *filename;
     int fd;
{
  char buffer[BUFSIZE];
  int bytes_read;

output:
  while ((bytes_read = read (fd, buffer, BUFSIZE)) > 0)
    xwrite (1, buffer, bytes_read);
  if (bytes_read == -1)
    error (1, errno, "%s", filename);
  if (forever)
    {
      sleep (1);
      goto output;
    }
}

#ifndef SIGUSR1
#define SIGUSR1 SIGSYS
#endif

/* To support tail_forever we use a signal handler that just quietly
   exits.  We are going to fork once for each file; we send a SIGUSR1
   to kill the children if an error occurs.  */

static RETSIGTYPE
sigusr1 (sig)
     int sig;
{
  exit (0);
}

/* Print error message MESSAGE for errno ERRNUM;
   send SIGUSR1 to the KIDS processes in PIDS;
   exit with status 1.  */

static void
kill_kids (errnum, message, pids, kids)
     int errnum;
     char *message;
     int *pids;
     int kids;
{
  int i;

  error (0, errnum, message);
  for (i = 0; i < kids; i++)
    kill (pids[i], SIGUSR1);
  exit (1);
}

/* The number of bytes that a pipe can hold (atomic read or write).  */
#ifndef PIPE_BUF
#define PIPE_BUF 512
#endif

/* Tail NFILES (>1) files forever until killed.  The file names are in NAMES.
   The open file descriptors are in `file_descs'.  Fork a process for each
   file, let all the processes write to a single pipe, and then read
   the pipe.  */
/* Should we reap the zombies with wait?  */

static void
tail_forever (names, nfiles)
     char **names;
     int nfiles;
{
  int pipe_descs[2];
  int *pids;
  int i;
  char *buffer = xmalloc (PIPE_BUF); /* malloc assures `int' alignment.  */
  int bytes_read;
  int ilast;

  if (pipe (pipe_descs) < 0)
    error (1, errno, "cannot make pipe");

  pids = (int *) xmalloc (nfiles * sizeof (int));

  /* fork once for each file.  If this is too ugly for you, don't use
     tail -f on multiple files.  Maybe we could use select as an
     alternative, though it's less portable.  Is it worth the bother?  */

  signal (SIGUSR1, sigusr1);

  for (i = 0; i < nfiles; i++)
    {
      if (file_descs[i] == -1)
	continue;
      
      pids[i] = fork ();
      if (pids[i] == -1)
	kill_kids (errno, "cannot fork", pids, i);
      if (pids[i] == 0)
	{
	  /* Child.  */
	  int offset;

	  close (pipe_descs[0]);

	  /* Each child reads continually from a file and writes to
	     the pipe.  Each write to a pipe is the index of the file
	     being read, followed by the number of bytes read from the
	     file, followed by the actual bytes.  Each child is
	     careful to write no more than PIPE_BUF bytes to the pipe,
	     so that the data from the various children does not get
	     intermixed.  */

	  /* The file index for this child is always the same.  */
	  *(int *) buffer = i;

	  offset = sizeof i + sizeof bytes_read;

	  while (1)
	    {
	      while ((bytes_read = read (file_descs[i], buffer + offset,
					 PIPE_BUF - offset)) > 0)
		{
		  *(int *) (buffer + sizeof i) = bytes_read;
		  if (write (pipe_descs[1], buffer, offset + bytes_read)
		      != offset + bytes_read)
		    _exit (0);	/* Somebody killed our parent?  */
		}
	      if (bytes_read == -1)
		{
		  error (0, errno, "%s", names[i]);
		  _exit (1);
		}
	      sleep (1);
	    }
	}
    }

  /* Parent.  */

  close (pipe_descs[1]);

  /* Wait for input to come in on the pipe.  Read the file index
     and the number of bytes.  Then read that many bytes and print
     them out.  Repeat until all the children have closed the pipe.  */

  ilast = -1;

  while ((bytes_read = read (pipe_descs[0], buffer,
			     sizeof i + sizeof bytes_read)) > 0)
    {
      int igot;			/* Index of latest process that wrote.  */

      if (bytes_read != sizeof i + sizeof bytes_read)
	kill_kids (errno, "read error", pids, nfiles); /* Yikes.  */

      /* Extract the file index and the number of bytes.  */
      igot = *(int *) buffer;
      bytes_read = *(int *) (buffer + sizeof i);

      if (print_headers && igot != ilast)
	write_header (names[igot]);
      ilast = igot;

      errno = 0;
      if (read (pipe_descs[0], buffer, bytes_read) != bytes_read)
	kill_kids (errno, "read error", pids, nfiles);
      if (write (1, buffer, bytes_read) != bytes_read)
	kill_kids (errno, "write error", pids, nfiles);
    }
	
  for (i = 0; i < nfiles; i++)
    kill (pids[i], SIGUSR1);

  free (buffer);
}

static void
parse_unit (str)
     char *str;
{
  int arglen = strlen (str);

  if (arglen == 0)
    return;

  switch (str[arglen - 1])
    {
    case 'b':
      unit_size = 512;
      str[arglen - 1] = '\0';
      break;
    case 'k':
      unit_size = 1024;
      str[arglen - 1] = '\0';
      break;
    case 'm':
      unit_size = 1048576;
      str[arglen - 1] = '\0';
      break;
    }
}

/* Convert STR, a string of ASCII digits, into an unsigned integer.
   Return -1 if STR does not represent a valid unsigned integer. */

static long
atou (str)
     char *str;
{
  unsigned long value;

  for (value = 0; ISDIGIT (*str); ++str)
    value = value * 10 + *str - '0';
  return *str ? -1 : value;
}

static void
usage ()
{
  fprintf (stderr, "\
Usage: %s [-c [+]N[bkm]] [-n [+]N] [-fqv] [--bytes=[+]N[bkm]] [--lines=[+]N]\n\
       [--follow] [--quiet] [--silent] [--verbose] [file...]\n\
       %s [{-,+}Nbcfklmqv] [file...]\n", program_name, program_name);
  exit (1);
}
