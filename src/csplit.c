/* csplit - split a file into sections determined by context lines
   Copyright (C) 91, 1995-2002 Free Software Foundation, Inc.

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

/* Written by Stuart Kemp, cpsrk@groper.jcu.edu.au.
   Modified by David MacKenzie, djm@gnu.ai.mit.edu. */

#include <config.h>

#include <stdio.h>
#include <getopt.h>
#include <sys/types.h>
#include <signal.h>

#include "system.h"
#include "closeout.h"

#include <regex.h>

#include "error.h"
#include "inttostr.h"
#include "safe-read.h"
#include "xstrtol.h"

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "csplit"

#define AUTHORS N_ ("Stuart Kemp and David MacKenzie")

#ifndef TRUE
# define FALSE 0
# define TRUE 1
#endif

/* Increment size of area for control records. */
#define ALLOC_SIZE 20

/* The default prefix for output file names. */
#define DEFAULT_PREFIX	"xx"

typedef int boolean;

/* A compiled pattern arg. */
struct control
{
  char *regexpr;		/* Non-compiled regular expression. */
  struct re_pattern_buffer re_compiled;	/* Compiled regular expression. */
  int offset;			/* Offset from regexp to split at. */
  uintmax_t lines_required;	/* Number of lines required. */
  uintmax_t repeat;		/* Repeat count. */
  int repeat_forever;		/* Non-zero if `*' used as a repeat count. */
  int argnum;			/* ARGV index. */
  boolean ignore;		/* If true, produce no output (for regexp). */
};

/* Initial size of data area in buffers. */
#define START_SIZE	8191

/* Increment size for data area. */
#define INCR_SIZE	2048

/* Number of lines kept in each node in line list. */
#define CTRL_SIZE	80

#ifdef DEBUG
/* Some small values to test the algorithms. */
# define START_SIZE	200
# define INCR_SIZE	10
# define CTRL_SIZE	1
#endif

/* A string with a length count. */
struct cstring
{
  unsigned int len;
  char *str;
};

/* Pointers to the beginnings of lines in the buffer area.
   These structures are linked together if needed. */
struct line
{
  unsigned used;		/* Number of offsets used in this struct. */
  unsigned insert_index;	/* Next offset to use when inserting line. */
  unsigned retrieve_index;	/* Next index to use when retrieving line. */
  struct cstring starts[CTRL_SIZE]; /* Lines in the data area. */
  struct line *next;		/* Next in linked list. */
};

/* The structure to hold the input lines.
   Contains a pointer to the data area and a list containing
   pointers to the individual lines. */
struct buffer_record
{
  unsigned bytes_alloc;		/* Size of the buffer area. */
  unsigned bytes_used;		/* Bytes used in the buffer area. */
  unsigned start_line;		/* First line number in this buffer. */
  unsigned first_available;	/* First line that can be retrieved. */
  unsigned num_lines;		/* Number of complete lines in this buffer. */
  char *buffer;			/* Data area. */
  struct line *line_start;	/* Head of list of pointers to lines. */
  struct line *curr_line;	/* The line start record currently in use. */
  struct buffer_record *next;
};

static void close_output_file (void);
static void create_output_file (void);
static void delete_all_files (void);
static void save_line_to_file (const struct cstring *line);
void usage (int status);

/* The name this program was run with. */
char *program_name;

/* Convert the number of 8-bit bytes of a binary representation to
   the number of characters required to represent the same quantity
   as an unsigned octal.  For example, a 32-bit (4-byte) quantity may
   require a field width as wide as 11 characters.  */
static const unsigned int bytes_to_octal_digits[] =
{0, 3, 6, 8, 11, 14, 16, 19, 22, 25, 27, 30, 32, 35, 38, 41, 43};

/* Input file descriptor. */
static int input_desc = 0;

/* List of available buffers. */
static struct buffer_record *free_list = NULL;

/* Start of buffer list. */
static struct buffer_record *head = NULL;

/* Partially read line. */
static char *hold_area = NULL;

/* Number of chars in `hold_area'. */
static unsigned hold_count = 0;

/* Number of the last line in the buffers. */
static unsigned last_line_number = 0;

/* Number of the line currently being examined. */
static unsigned current_line = 0;

/* If TRUE, we have read EOF. */
static boolean have_read_eof = FALSE;

/* Name of output files. */
static char *filename_space = NULL;

/* Prefix part of output file names. */
static char *prefix = NULL;

/* Suffix part of output file names. */
static char *suffix = NULL;

/* Number of digits to use in output file names. */
static int digits = 2;

/* Number of files created so far. */
static unsigned int files_created = 0;

/* Number of bytes written to current file. */
static unsigned int bytes_written;

/* Output file pointer. */
static FILE *output_stream = NULL;

/* Output file name. */
static char *output_filename = NULL;

/* Perhaps it would be cleaner to pass arg values instead of indexes. */
static char **global_argv;

/* If TRUE, do not print the count of bytes in each output file. */
static boolean suppress_count;

/* If TRUE, remove output files on error. */
static boolean remove_files;

/* If TRUE, remove all output files which have a zero length. */
static boolean elide_empty_files;

/* The compiled pattern arguments, which determine how to split
   the input file. */
static struct control *controls;

/* Number of elements in `controls'. */
static unsigned int control_used;

static struct option const longopts[] =
{
  {"digits", required_argument, NULL, 'n'},
  {"quiet", no_argument, NULL, 'q'},
  {"silent", no_argument, NULL, 's'},
  {"keep-files", no_argument, NULL, 'k'},
  {"elide-empty-files", no_argument, NULL, 'z'},
  {"prefix", required_argument, NULL, 'f'},
  {"suffix-format", required_argument, NULL, 'b'},
  {GETOPT_HELP_OPTION_DECL},
  {GETOPT_VERSION_OPTION_DECL},
  {NULL, 0, NULL, 0}
};

/* Optionally remove files created so far; then exit.
   Called when an error detected. */

static void
cleanup (void)
{
  if (output_stream)
    close_output_file ();

  if (remove_files)
    delete_all_files ();
}

static void
cleanup_fatal (void)
{
  cleanup ();
  exit (EXIT_FAILURE);
}

static RETSIGTYPE
interrupt_handler (int sig)
{
#ifdef SA_NOCLDSTOP
  struct sigaction sigact;

  sigact.sa_handler = SIG_DFL;
  sigemptyset (&sigact.sa_mask);
  sigact.sa_flags = 0;
  sigaction (sig, &sigact, NULL);
#else
  signal (sig, SIG_DFL);
#endif
  cleanup ();
  raise (sig);
}

/* Keep track of NUM chars of a partial line in buffer START.
   These chars will be retrieved later when another large buffer is read.
   It is not necessary to create a new buffer for these chars; instead,
   we keep a pointer to the existing buffer.  This buffer *is* on the
   free list, and when the next buffer is obtained from this list
   (even if it is this one), these chars will be placed at the
   start of the new buffer. */

static void
save_to_hold_area (char *start, unsigned int num)
{
  hold_area = start;
  hold_count = num;
}

/* Read up to MAX_N_BYTES chars from the input stream into DEST.
   Return the number of chars read. */
/* FIXME: MAX_N_BYTES should be of type size_t, but if you pull
   that thread, you'll find there are many other `unsigned' types
   in this file that should also be changed.  */

static size_t
read_input (char *dest, int max_n_bytes)
{
  size_t bytes_read;

  if (max_n_bytes == 0)
    return 0;

  bytes_read = safe_read (input_desc, dest, max_n_bytes);

  if (bytes_read == 0)
    have_read_eof = TRUE;

  if (bytes_read == SAFE_READ_ERROR)
    {
      error (0, errno, _("read error"));
      cleanup_fatal ();
    }

  return bytes_read;
}

/* Initialize existing line record P. */

static void
clear_line_control (struct line *p)
{
  p->used = 0;
  p->insert_index = 0;
  p->retrieve_index = 0;
}

/* Initialize all line records in B. */

static void
clear_all_line_control (struct buffer_record *b)
{
  struct line *l;

  for (l = b->line_start; l; l = l->next)
    clear_line_control (l);
}

/* Return a new, initialized line record. */

static struct line *
new_line_control (void)
{
  struct line *p;

  p = (struct line *) xmalloc (sizeof (struct line));

  p->next = NULL;
  clear_line_control (p);

  return p;
}

/* Record LINE_START, which is the address of the start of a line
   of length LINE_LEN in the large buffer, in the lines buffer of B. */

static void
keep_new_line (struct buffer_record *b, char *line_start, int line_len)
{
  struct line *l;

  /* If there is no existing area to keep line info, get some. */
  if (b->line_start == NULL)
    b->line_start = b->curr_line = new_line_control ();

  /* If existing area for lines is full, get more. */
  if (b->curr_line->used == CTRL_SIZE)
    {
      b->curr_line->next = new_line_control ();
      b->curr_line = b->curr_line->next;
    }

  l = b->curr_line;

  /* Record the start of the line, and update counters. */
  l->starts[l->insert_index].str = line_start;
  l->starts[l->insert_index].len = line_len;
  l->used++;
  l->insert_index++;
}

/* Scan the buffer in B for newline characters
   and record the line start locations and lengths in B.
   Return the number of lines found in this buffer.

   There may be an incomplete line at the end of the buffer;
   a pointer is kept to this area, which will be used when
   the next buffer is filled. */

static unsigned int
record_line_starts (struct buffer_record *b)
{
  char *line_start;		/* Start of current line. */
  char *line_end;		/* End of each line found. */
  unsigned int bytes_left;	/* Length of incomplete last line. */
  unsigned int lines;		/* Number of lines found. */
  unsigned int line_length;	/* Length of each line found. */

  if (b->bytes_used == 0)
    return 0;

  lines = 0;
  line_start = b->buffer;
  bytes_left = b->bytes_used;

  for (;;)
    {
      line_end = memchr (line_start, '\n', bytes_left);
      if (line_end == NULL)
	break;
      line_length = line_end - line_start + 1;
      keep_new_line (b, line_start, line_length);
      bytes_left -= line_length;
      line_start = line_end + 1;
      lines++;
    }

  /* Check for an incomplete last line. */
  if (bytes_left)
    {
      if (have_read_eof)
	{
	  keep_new_line (b, line_start, bytes_left);
	  lines++;
	}
      else
	save_to_hold_area (line_start, bytes_left);
    }

  b->num_lines = lines;
  b->first_available = b->start_line = last_line_number + 1;
  last_line_number += lines;

  return lines;
}

/* Return a new buffer with room to store SIZE bytes, plus
   an extra byte for safety. */

static struct buffer_record *
create_new_buffer (unsigned int size)
{
  struct buffer_record *new_buffer;

  new_buffer = (struct buffer_record *)
    xmalloc (sizeof (struct buffer_record));

  new_buffer->buffer = (char *) xmalloc (size + 1);

  new_buffer->bytes_alloc = size;
  new_buffer->line_start = new_buffer->curr_line = NULL;

  return new_buffer;
}

/* Return a new buffer of at least MINSIZE bytes.  If a buffer of at
   least that size is currently free, use it, otherwise create a new one. */

static struct buffer_record *
get_new_buffer (unsigned int min_size)
{
  struct buffer_record *p, *q;
  struct buffer_record *new_buffer; /* Buffer to return. */
  unsigned int alloc_size;	/* Actual size that will be requested. */

  alloc_size = START_SIZE;
  while (min_size > alloc_size)
    alloc_size += INCR_SIZE;

  if (free_list == NULL)
    new_buffer = create_new_buffer (alloc_size);
  else
    {
      /* Use first-fit to find a buffer. */
      p = new_buffer = NULL;
      q = free_list;

      do
	{
	  if (q->bytes_alloc >= min_size)
	    {
	      if (p == NULL)
		free_list = q->next;
	      else
		p->next = q->next;
	      break;
	    }
	  p = q;
	  q = q->next;
	}
      while (q);

      new_buffer = (q ? q : create_new_buffer (alloc_size));

      new_buffer->curr_line = new_buffer->line_start;
      clear_all_line_control (new_buffer);
    }

  new_buffer->num_lines = 0;
  new_buffer->bytes_used = 0;
  new_buffer->start_line = new_buffer->first_available = last_line_number + 1;
  new_buffer->next = NULL;

  return new_buffer;
}

/* Add buffer BUF to the list of free buffers. */

static void
free_buffer (struct buffer_record *buf)
{
  buf->next = free_list;
  free_list = buf;
}

/* Append buffer BUF to the linked list of buffers that contain
   some data yet to be processed. */

static void
save_buffer (struct buffer_record *buf)
{
  struct buffer_record *p;

  buf->next = NULL;
  buf->curr_line = buf->line_start;

  if (head == NULL)
    head = buf;
  else
    {
      for (p = head; p->next; p = p->next)
	/* Do nothing. */ ;
      p->next = buf;
    }
}

/* Fill a buffer of input.

   Set the initial size of the buffer to a default.
   Fill the buffer (from the hold area and input stream)
   and find the individual lines.
   If no lines are found (the buffer is too small to hold the next line),
   release the current buffer (whose contents would have been put in the
   hold area) and repeat the process with another large buffer until at least
   one entire line has been read.

   Return TRUE if a new buffer was obtained, otherwise false
   (in which case end-of-file must have been encountered). */

static boolean
load_buffer (void)
{
  struct buffer_record *b;
  unsigned int bytes_wanted = START_SIZE; /* Minimum buffer size. */
  unsigned int bytes_avail;		/* Size of new buffer created. */
  unsigned int lines_found;		/* Number of lines in this new buffer. */
  char *p;			/* Place to load into buffer. */

  if (have_read_eof)
    return FALSE;

  /* We must make the buffer at least as large as the amount of data
     in the partial line left over from the last call. */
  if (bytes_wanted < hold_count)
    bytes_wanted = hold_count;

  do
    {
      b = get_new_buffer (bytes_wanted);
      bytes_avail = b->bytes_alloc; /* Size of buffer returned. */
      p = b->buffer;

      /* First check the `holding' area for a partial line. */
      if (hold_count)
	{
	  if (p != hold_area)
	    memcpy (p, hold_area, hold_count);
	  p += hold_count;
	  b->bytes_used += hold_count;
	  bytes_avail -= hold_count;
	  hold_count = 0;
	}

      b->bytes_used += (unsigned int) read_input (p, bytes_avail);

      lines_found = record_line_starts (b);
      bytes_wanted = b->bytes_alloc * 2;
      if (!lines_found)
	free_buffer (b);
    }
  while (!lines_found && !have_read_eof);

  if (lines_found)
    save_buffer (b);

  return lines_found != 0;
}

/* Return the line number of the first line that has not yet been retrieved. */

static unsigned int
get_first_line_in_buffer (void)
{
  if (head == NULL && !load_buffer ())
    error (EXIT_FAILURE, errno, _("input disappeared"));

  return head->first_available;
}

/* Return a pointer to the logical first line in the buffer and make the
   next line the logical first line.
   Return NULL if there is no more input. */

static struct cstring *
remove_line (void)
{
  struct cstring *line;		/* Return value. */
  struct line *l;		/* For convenience. */

  if (head == NULL && !load_buffer ())
    return NULL;

  if (current_line < head->first_available)
    current_line = head->first_available;

  ++(head->first_available);

  l = head->curr_line;

  line = &l->starts[l->retrieve_index];

  /* Advance index to next line. */
  if (++l->retrieve_index == l->used)
    {
      /* Go on to the next line record. */
      head->curr_line = l->next;
      if (head->curr_line == NULL || head->curr_line->used == 0)
	{
	  /* Go on to the next data block. */
	  struct buffer_record *b = head;
	  head = head->next;
	  free_buffer (b);
	}
    }

  return line;
}

/* Search the buffers for line LINENUM, reading more input if necessary.
   Return a pointer to the line, or NULL if it is not found in the file. */

static struct cstring *
find_line (unsigned int linenum)
{
  struct buffer_record *b;

  if (head == NULL && !load_buffer ())
    return NULL;

  if (linenum < head->start_line)
    return NULL;

  for (b = head;;)
    {
      if (linenum < b->start_line + b->num_lines)
	{
	  /* The line is in this buffer. */
	  struct line *l;
	  unsigned int offset;	/* How far into the buffer the line is. */

	  l = b->line_start;
	  offset = linenum - b->start_line;
	  /* Find the control record. */
	  while (offset >= CTRL_SIZE)
	    {
	      l = l->next;
	      offset -= CTRL_SIZE;
	    }
	  return &l->starts[offset];
	}
      if (b->next == NULL && !load_buffer ())
	return NULL;
      b = b->next;		/* Try the next data block. */
    }
}

/* Return TRUE if at least one more line is available for input. */

static boolean
no_more_lines (void)
{
  return (find_line (current_line + 1) == NULL) ? TRUE : FALSE;
}

/* Set the name of the input file to NAME and open it. */

static void
set_input_file (const char *name)
{
  if (STREQ (name, "-"))
    input_desc = 0;
  else
    {
      input_desc = open (name, O_RDONLY);
      if (input_desc < 0)
	error (EXIT_FAILURE, errno, "%s", name);
    }
}

/* Write all lines from the beginning of the buffer up to, but
   not including, line LAST_LINE, to the current output file.
   If IGNORE is TRUE, do not output lines selected here.
   ARGNUM is the index in ARGV of the current pattern. */

static void
write_to_file (unsigned int last_line, boolean ignore, int argnum)
{
  struct cstring *line;
  unsigned int first_line;		/* First available input line. */
  unsigned int lines;		/* Number of lines to output. */
  unsigned int i;

  first_line = get_first_line_in_buffer ();

  if (first_line > last_line)
    {
      error (0, 0, _("%s: line number out of range"), global_argv[argnum]);
      cleanup_fatal ();
    }

  lines = last_line - first_line;

  for (i = 0; i < lines; i++)
    {
      line = remove_line ();
      if (line == NULL)
	{
	  error (0, 0, _("%s: line number out of range"), global_argv[argnum]);
	  cleanup_fatal ();
	}
      if (!ignore)
	save_line_to_file (line);
    }
}

/* Output any lines left after all regexps have been processed. */

static void
dump_rest_of_file (void)
{
  struct cstring *line;

  while ((line = remove_line ()) != NULL)
    save_line_to_file (line);
}

/* Handle an attempt to read beyond EOF under the control of record P,
   on iteration REPETITION if nonzero. */

static void
handle_line_error (const struct control *p, int repetition)
{
  char buf[INT_BUFSIZE_BOUND (uintmax_t)];

  fprintf (stderr, _("%s: `%s': line number out of range"),
	   program_name, umaxtostr (p->lines_required, buf));
  if (repetition)
    fprintf (stderr, _(" on repetition %d\n"), repetition);
  else
    fprintf (stderr, "\n");

  cleanup_fatal ();
}

/* Determine the line number that marks the end of this file,
   then get those lines and save them to the output file.
   P is the control record.
   REPETITION is the repetition number. */

static void
process_line_count (const struct control *p, int repetition)
{
  unsigned int linenum;
  uintmax_t last_line_to_save = p->lines_required * (repetition + 1);
  struct cstring *line;

  create_output_file ();

  linenum = get_first_line_in_buffer ();

  while (linenum++ < last_line_to_save)
    {
      line = remove_line ();
      if (line == NULL)
	handle_line_error (p, repetition);
      save_line_to_file (line);
    }

  close_output_file ();

  /* Ensure that the line number specified is not 1 greater than
     the number of lines in the file. */
  if (no_more_lines ())
    handle_line_error (p, repetition);
}

static void
regexp_error (struct control *p, int repetition, boolean ignore)
{
  fprintf (stderr, _("%s: `%s': match not found"),
	   program_name, global_argv[p->argnum]);

  if (repetition)
    fprintf (stderr, _(" on repetition %d\n"), repetition);
  else
    fprintf (stderr, "\n");

  if (!ignore)
    {
      dump_rest_of_file ();
      close_output_file ();
    }
  cleanup_fatal ();
}

/* Read the input until a line matches the regexp in P, outputting
   it unless P->IGNORE is TRUE.
   REPETITION is this repeat-count; 0 means the first time. */

static void
process_regexp (struct control *p, int repetition)
{
  struct cstring *line;		/* From input file. */
  unsigned int line_len;	/* To make "$" in regexps work. */
  unsigned int break_line;	/* First line number of next file. */
  boolean ignore = p->ignore;	/* If TRUE, skip this section. */
  int ret;

  if (!ignore)
    create_output_file ();

  /* If there is no offset for the regular expression, or
     it is positive, then it is not necessary to buffer the lines. */

  if (p->offset >= 0)
    {
      for (;;)
	{
	  line = find_line (++current_line);
	  if (line == NULL)
	    {
	      if (p->repeat_forever)
		{
		  if (!ignore)
		    {
		      dump_rest_of_file ();
		      close_output_file ();
		    }
		  exit (EXIT_SUCCESS);
		}
	      else
		regexp_error (p, repetition, ignore);
	    }
	  line_len = line->len;
	  if (line->str[line_len - 1] == '\n')
	    line_len--;
	  ret = re_search (&p->re_compiled, line->str, line_len,
			   0, line_len, (struct re_registers *) 0);
	  if (ret == -2)
	    {
	      error (0, 0, _("error in regular expression search"));
	      cleanup_fatal ();
	    }
	  if (ret == -1)
	    {
	      line = remove_line ();
	      if (!ignore)
		save_line_to_file (line);
	    }
	  else
	    break;
	}
    }
  else
    {
      /* Buffer the lines. */
      for (;;)
	{
	  line = find_line (++current_line);
	  if (line == NULL)
	    {
	      if (p->repeat_forever)
		{
		  if (!ignore)
		    {
		      dump_rest_of_file ();
		      close_output_file ();
		    }
		  exit (EXIT_SUCCESS);
		}
	      else
		regexp_error (p, repetition, ignore);
	    }
	  line_len = line->len;
	  if (line->str[line_len - 1] == '\n')
	    line_len--;
	  ret = re_search (&p->re_compiled, line->str, line_len,
			   0, line_len, (struct re_registers *) 0);
	  if (ret == -2)
	    {
	      error (0, 0, _("error in regular expression search"));
	      cleanup_fatal ();
	    }
	  if (ret >= 0)
	    break;
	}
    }

  /* Account for any offset from this regexp. */
  break_line = current_line + p->offset;

  write_to_file (break_line, ignore, p->argnum);

  if (!ignore)
    close_output_file ();

  if (p->offset > 0)
    current_line = break_line;
}

/* Split the input file according to the control records we have built. */

static void
split_file (void)
{
  unsigned int i, j;

  for (i = 0; i < control_used; i++)
    {
      if (controls[i].regexpr)
	{
	  for (j = 0; (controls[i].repeat_forever
		       || j <= controls[i].repeat); j++)
	    process_regexp (&controls[i], j);
	}
      else
	{
	  for (j = 0; (controls[i].repeat_forever
		       || j <= controls[i].repeat); j++)
	    process_line_count (&controls[i], j);
	}
    }

  create_output_file ();
  dump_rest_of_file ();
  close_output_file ();
}

/* Return the name of output file number NUM. */

static char *
make_filename (unsigned int num)
{
  strcpy (filename_space, prefix);
  if (suffix)
    sprintf (filename_space+strlen(prefix), suffix, num);
  else
    sprintf (filename_space+strlen(prefix), "%0*d", digits, num);
  return filename_space;
}

/* Create the next output file. */

static void
create_output_file (void)
{
  output_filename = make_filename (files_created);
  output_stream = fopen (output_filename, "w");
  if (output_stream == NULL)
    {
      error (0, errno, "%s", output_filename);
      cleanup_fatal ();
    }
  files_created++;
  bytes_written = 0;
}

/* Delete all the files we have created. */

static void
delete_all_files (void)
{
  unsigned int i;
  char *name;

  for (i = 0; i < files_created; i++)
    {
      name = make_filename (i);
      if (unlink (name))
	error (0, errno, "%s", name);
    }
}

/* Close the current output file and print the count
   of characters in this file. */

static void
close_output_file (void)
{
  if (output_stream)
    {
      if (ferror (output_stream) || fclose (output_stream) == EOF)
	{
	  error (0, errno, _("write error for `%s'"), output_filename);
	  output_stream = NULL;
	  cleanup_fatal ();
	}
      if (bytes_written == 0 && elide_empty_files)
	{
	  if (unlink (output_filename))
	    error (0, errno, "%s", output_filename);
	  files_created--;
	}
      else
	{
	  /* FIXME: if we write to stdout here, we have to close stdout
	     and check for errors.  */
	  if (!suppress_count)
	    fprintf (stdout, "%d\n", bytes_written);
	}
      output_stream = NULL;
    }
}

/* Save line LINE to the output file and
   increment the character count for the current file. */

static void
save_line_to_file (const struct cstring *line)
{
  fwrite (line->str, sizeof (char), line->len, output_stream);
  bytes_written += line->len;
}

/* Return a new, initialized control record. */

static struct control *
new_control_record (void)
{
  static unsigned control_allocated = 0; /* Total space allocated. */
  struct control *p;

  if (control_allocated == 0)
    {
      control_allocated = ALLOC_SIZE;
      controls = (struct control *)
	xmalloc (sizeof (struct control) * control_allocated);
    }
  else if (control_used == control_allocated)
    {
      control_allocated += ALLOC_SIZE;
      controls = (struct control *)
	xrealloc ((char *) controls,
		  sizeof (struct control) * control_allocated);
    }
  p = &controls[control_used++];
  p->regexpr = NULL;
  p->repeat = 0;
  p->repeat_forever = 0;
  p->lines_required = 0;
  p->offset = 0;
  return p;
}

/* Check if there is a numeric offset after a regular expression.
   STR is the entire command line argument.
   P is the control record for this regular expression.
   NUM is the numeric part of STR. */

static void
check_for_offset (struct control *p, const char *str, const char *num)
{
  unsigned long val;

  if (*num != '-' && *num != '+')
    error (EXIT_FAILURE, 0, _("%s: `+' or `-' expected after delimeter"), str);

  if (xstrtoul (num + 1, NULL, 10, &val, "") != LONGINT_OK
      || val > UINT_MAX)
    error (EXIT_FAILURE, 0, _("%s: integer expected after `%c'"), str, *num);
  p->offset = (unsigned int) val;

  if (*num == '-')
    p->offset = -p->offset;
}

/* Given that the first character of command line arg STR is '{',
   make sure that the rest of the string is a valid repeat count
   and store its value in P.
   ARGNUM is the ARGV index of STR. */

static void
parse_repeat_count (int argnum, struct control *p, char *str)
{
  uintmax_t val;
  char *end;

  end = str + strlen (str) - 1;
  if (*end != '}')
    error (EXIT_FAILURE, 0, _("%s: `}' is required in repeat count"), str);
  *end = '\0';

  if (str+1 == end-1 && *(str+1) == '*')
    p->repeat_forever = 1;
  else
    {
      if (xstrtoumax (str + 1, NULL, 10, &val, "") != LONGINT_OK)
	{
	  error (EXIT_FAILURE, 0,
		 _("%s}: integer required between `{' and `}'"),
		 global_argv[argnum]);
	}
      p->repeat = val;
    }

  *end = '}';
}

/* Extract the regular expression from STR and check for a numeric offset.
   STR should start with the regexp delimiter character.
   Return a new control record for the regular expression.
   ARGNUM is the ARGV index of STR.
   Unless IGNORE is TRUE, mark these lines for output. */

static struct control *
extract_regexp (int argnum, boolean ignore, char *str)
{
  int len;			/* Number of chars in this regexp. */
  char delim = *str;
  char *closing_delim;
  struct control *p;
  const char *err;

  closing_delim = strrchr (str + 1, delim);
  if (closing_delim == NULL)
    error (EXIT_FAILURE, 0,
	   _("%s: closing delimeter `%c' missing"), str, delim);

  len = closing_delim - str - 1;
  p = new_control_record ();
  p->argnum = argnum;
  p->ignore = ignore;

  p->regexpr = (char *) xmalloc ((unsigned) (len + 1));
  strncpy (p->regexpr, str + 1, len);
  p->re_compiled.allocated = len * 2;
  p->re_compiled.buffer = (unsigned char *) xmalloc (p->re_compiled.allocated);
  p->re_compiled.fastmap = xmalloc (256);
  p->re_compiled.translate = 0;
  err = re_compile_pattern (p->regexpr, len, &p->re_compiled);
  if (err)
    {
      error (0, 0, _("%s: invalid regular expression: %s"), str, err);
      cleanup_fatal ();
    }

  if (closing_delim[1])
    check_for_offset (p, str, closing_delim + 1);

  return p;
}

/* Extract the break patterns from args START through ARGC - 1 of ARGV.
   After each pattern, check if the next argument is a repeat count. */

static void
parse_patterns (int argc, int start, char **argv)
{
  int i;			/* Index into ARGV. */
  struct control *p;		/* New control record created. */
  uintmax_t val;
  static uintmax_t last_val = 0;

  for (i = start; i < argc; i++)
    {
      if (*argv[i] == '/' || *argv[i] == '%')
	{
	  p = extract_regexp (i, *argv[i] == '%', argv[i]);
	}
      else
	{
	  p = new_control_record ();
	  p->argnum = i;

	  if (xstrtoumax (argv[i], NULL, 10, &val, "") != LONGINT_OK)
	    error (EXIT_FAILURE, 0, _("%s: invalid pattern"), argv[i]);
	  if (val == 0)
	    error (EXIT_FAILURE, 0,
		   _("%s: line number must be greater than zero"),
		   argv[i]);
	  if (val < last_val)
	    {
	      char buf[INT_BUFSIZE_BOUND (uintmax_t)];
	      error (EXIT_FAILURE, 0,
	       _("line number `%s' is smaller than preceding line number, %s"),
		     argv[i], umaxtostr (last_val, buf));
	    }

	  if (val == last_val)
	    error (0, 0,
	   _("warning: line number `%s' is the same as preceding line number"),
		   argv[i]);

	  last_val = val;

	  p->lines_required = val;
	}

      if (i + 1 < argc && *argv[i + 1] == '{')
	{
	  /* We have a repeat count. */
	  i++;
	  parse_repeat_count (i, p, argv[i]);
	}
    }
}

static unsigned
get_format_flags (char **format_ptr)
{
  unsigned count = 0;

  for (; **format_ptr; (*format_ptr)++)
    {
      switch (**format_ptr)
	{
	case '-':
	  break;

	case '+':
	case ' ':
	  count++;
	  break;

	case '#':
	  count += 2;	/* Allow for 0x prefix preceeding an `x' conversion.  */
	  break;

	default:
	  return count;
	}
    }
  return count;
}

static unsigned
get_format_width (char **format_ptr)
{
  unsigned count = 0;
  char *start;
  int ch_save;

  start = *format_ptr;
  for (; ISDIGIT (**format_ptr); (*format_ptr)++)
    continue;

  ch_save = **format_ptr;
  **format_ptr = '\0';
  /* In the case where no minimum field width is explicitly specified,
     allow for enough octal digits to represent the value of LONG_MAX.  */
  count = ((*format_ptr == start)
	   ? bytes_to_octal_digits[sizeof (long)]
	   /* FIXME: don't use atoi, it may silently overflow.
	      Besides, we know the result is non-negative, so shouldn't
	      need that cast.  */
	   : (unsigned) atoi (start));
  **format_ptr = ch_save;
  return count;
}

static unsigned
get_format_prec (char **format_ptr)
{
  unsigned count = 0;
  char *start;
  int ch_save;
  int is_negative;

  if (**format_ptr != '.')
    return 0;
  (*format_ptr)++;

  if (**format_ptr == '-' || **format_ptr == '+')
    {
      is_negative = (**format_ptr == '-');
      (*format_ptr)++;
    }
  else
    {
      is_negative = 0;
    }

  start = *format_ptr;
  for (; ISDIGIT (**format_ptr); (*format_ptr)++)
    continue;

  /* ANSI 4.9.6.1 says that if the precision is negative, it's as good as
     not there. */
  if (is_negative)
    start = *format_ptr;

  ch_save = **format_ptr;
  **format_ptr = '\0';
  count = (*format_ptr == start) ? 11 : atoi (start);
  **format_ptr = ch_save;

  return count;
}

static void
get_format_conv_type (char **format_ptr)
{
  int ch = *((*format_ptr)++);

  switch (ch)
    {
    case 'd':
    case 'i':
    case 'o':
    case 'u':
    case 'x':
    case 'X':
      break;

    case 0:
      error (EXIT_FAILURE, 0, _("missing conversion specifier in suffix"));
      break;

    default:
      if (ISPRINT (ch))
        error (EXIT_FAILURE, 0,
	       _("invalid conversion specifier in suffix: %c"), ch);
      else
	error (EXIT_FAILURE, 0,
	       _("invalid conversion specifier in suffix: \\%.3o"), ch);
    }
}

static unsigned
max_out (char *format)
{
  unsigned out_count = 0;
  unsigned percents = 0;

  for (; *format; )
    {
      int ch = *format++;

      if (ch != '%')
        out_count++;
      else
	{
	  percents++;
	  out_count += get_format_flags (&format);
	  {
	    int width = get_format_width (&format);
	    int prec = get_format_prec (&format);

	    out_count += MAX (width, prec);
	  }
	  get_format_conv_type (&format);
	}
    }

  if (percents == 0)
    error (EXIT_FAILURE, 0,
	   _("missing %% conversion specification in suffix"));
  else if (percents > 1)
    error (EXIT_FAILURE, 0,
	   _("too many %% conversion specifications in suffix"));

  return out_count;
}

int
main (int argc, char **argv)
{
  int optc;
  unsigned long val;
#ifdef SA_NOCLDSTOP
  struct sigaction oldact, newact;
#endif

  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  global_argv = argv;
  controls = NULL;
  control_used = 0;
  suppress_count = FALSE;
  remove_files = TRUE;
  prefix = DEFAULT_PREFIX;

  /* Change the way xmalloc and xrealloc fail.  */
  xalloc_fail_func = cleanup;

#ifdef SA_NOCLDSTOP
  newact.sa_handler = interrupt_handler;
  sigemptyset (&newact.sa_mask);
  newact.sa_flags = 0;

  sigaction (SIGHUP, NULL, &oldact);
  if (oldact.sa_handler != SIG_IGN)
    sigaction (SIGHUP, &newact, NULL);

  sigaction (SIGINT, NULL, &oldact);
  if (oldact.sa_handler != SIG_IGN)
    sigaction (SIGINT, &newact, NULL);

  sigaction (SIGQUIT, NULL, &oldact);
  if (oldact.sa_handler != SIG_IGN)
    sigaction (SIGQUIT, &newact, NULL);

  sigaction (SIGTERM, NULL, &oldact);
  if (oldact.sa_handler != SIG_IGN)
    sigaction (SIGTERM, &newact, NULL);
#else
  if (signal (SIGHUP, SIG_IGN) != SIG_IGN)
    signal (SIGHUP, interrupt_handler);
  if (signal (SIGINT, SIG_IGN) != SIG_IGN)
    signal (SIGINT, interrupt_handler);
  if (signal (SIGQUIT, SIG_IGN) != SIG_IGN)
    signal (SIGQUIT, interrupt_handler);
  if (signal (SIGTERM, SIG_IGN) != SIG_IGN)
    signal (SIGTERM, interrupt_handler);
#endif

  while ((optc = getopt_long (argc, argv, "f:b:kn:sqz", longopts, NULL)) != -1)
    switch (optc)
      {
      case 0:
	break;

      case 'f':
	prefix = optarg;
	break;

      case 'b':
	suffix = optarg;
	break;

      case 'k':
	remove_files = FALSE;
	break;

      case 'n':
	if (xstrtoul (optarg, NULL, 10, &val, "") != LONGINT_OK
	    || val > INT_MAX)
	  error (EXIT_FAILURE, 0, _("%s: invalid number"), optarg);
	digits = (int) val;
	break;

      case 's':
      case 'q':
	suppress_count = TRUE;
	break;

      case 'z':
	elide_empty_files = TRUE;
	break;

      case_GETOPT_HELP_CHAR;

      case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);

      default:
	usage (EXIT_FAILURE);
      }

  if (argc - optind < 2)
    {
      error (0, 0, _("too few arguments"));
      usage (EXIT_FAILURE);
    }

  if (suffix)
    filename_space = (char *) xmalloc (strlen (prefix) + max_out (suffix) + 2);
  else
    filename_space = (char *) xmalloc (strlen (prefix) + digits + 2);

  set_input_file (argv[optind++]);

  parse_patterns (argc, optind, argv);

  split_file ();

  if (close (input_desc) < 0)
    {
      error (0, errno, _("read error"));
      cleanup_fatal ();
    }

  exit (EXIT_SUCCESS);
}

void
usage (int status)
{
  if (status != 0)
    fprintf (stderr, _("Try `%s --help' for more information.\n"),
	     program_name);
  else
    {
      printf (_("\
Usage: %s [OPTION]... FILE PATTERN...\n\
"),
	      program_name);
      fputs (_("\
Output pieces of FILE separated by PATTERN(s) to files `xx01', `xx02', ...,\n\
and output byte counts of each piece to standard output.\n\
\n\
"), stdout);
      fputs (_("\
Mandatory arguments to long options are mandatory for short options too.\n\
"), stdout);
      fputs (_("\
  -b, --suffix-format=FORMAT use sprintf FORMAT instead of %d\n\
  -f, --prefix=PREFIX        use PREFIX instead of `xx'\n\
  -k, --keep-files           do not remove output files on errors\n\
"), stdout);
      fputs (_("\
  -n, --digits=DIGITS        use specified number of digits instead of 2\n\
  -s, --quiet, --silent      do not print counts of output file sizes\n\
  -z, --elide-empty-files    remove empty output files\n\
"), stdout);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      fputs (_("\
\n\
Read standard input if FILE is -.  Each PATTERN may be:\n\
"), stdout);
      fputs (_("\
\n\
  INTEGER            copy up to but not including specified line number\n\
  /REGEXP/[OFFSET]   copy up to but not including a matching line\n\
  %REGEXP%[OFFSET]   skip to, but not including a matching line\n\
  {INTEGER}          repeat the previous pattern specified number of times\n\
  {*}                repeat the previous pattern as many times as possible\n\
\n\
A line OFFSET is a required `+' or `-' followed by a positive integer.\n\
"), stdout);
      printf (_("\nReport bugs to <%s>.\n"), PACKAGE_BUGREPORT);
    }
  exit (status == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}
