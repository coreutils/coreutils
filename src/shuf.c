/* Shuffle lines of text.

   Copyright (C) 2006-2023 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.

   Written by Paul Eggert.  */

#include <config.h>

#include <sys/types.h>
#include "system.h"

#include "fadvise.h"
#include "getopt.h"
#include "linebuffer.h"
#include "quote.h"
#include "randint.h"
#include "randperm.h"
#include "read-file.h"
#include "stdio--.h"
#include "xstrtol.h"

/* The official name of this program (e.g., no 'g' prefix).  */
#define PROGRAM_NAME "shuf"

#define AUTHORS proper_name ("Paul Eggert")

/* For reservoir-sampling, allocate the reservoir lines in batches.  */
enum { RESERVOIR_LINES_INCREMENT = 1024 };

/* reservoir-sampling introduces CPU overhead for small inputs.
   So only enable it for inputs >= this limit.
   This limit was determined using these commands:
     $ for p in $(seq 7); do src/seq $((10**$p)) > 10p$p.in; done
     $ for p in $(seq 7); do time shuf-nores -n10 10p$p.in >/dev/null; done
     $ for p in $(seq 7); do time shuf -n10 10p$p.in >/dev/null; done  .*/
enum { RESERVOIR_MIN_INPUT = 8192 * 1024 };


void
usage (int status)
{
  if (status != EXIT_SUCCESS)
    emit_try_help ();
  else
    {
      printf (_("\
Usage: %s [OPTION]... [FILE]\n\
  or:  %s -e [OPTION]... [ARG]...\n\
  or:  %s -i LO-HI [OPTION]...\n\
"),
              program_name, program_name, program_name);
      fputs (_("\
Write a random permutation of the input lines to standard output.\n\
"), stdout);

      emit_stdin_note ();
      emit_mandatory_arg_note ();

      fputs (_("\
  -e, --echo                treat each ARG as an input line\n\
  -i, --input-range=LO-HI   treat each number LO through HI as an input line\n\
  -n, --head-count=COUNT    output at most COUNT lines\n\
  -o, --output=FILE         write result to FILE instead of standard output\n\
      --random-source=FILE  get random bytes from FILE\n\
  -r, --repeat              output lines can be repeated\n\
"), stdout);
      fputs (_("\
  -z, --zero-terminated     line delimiter is NUL, not newline\n\
"), stdout);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      emit_ancillary_info (PROGRAM_NAME);
    }

  exit (status);
}

/* For long options that have no equivalent short option, use a
   non-character as a pseudo short option, starting with CHAR_MAX + 1.  */
enum
{
  RANDOM_SOURCE_OPTION = CHAR_MAX + 1
};

static struct option const long_opts[] =
{
  {"echo", no_argument, nullptr, 'e'},
  {"input-range", required_argument, nullptr, 'i'},
  {"head-count", required_argument, nullptr, 'n'},
  {"output", required_argument, nullptr, 'o'},
  {"random-source", required_argument, nullptr, RANDOM_SOURCE_OPTION},
  {"repeat", no_argument, nullptr, 'r'},
  {"zero-terminated", no_argument, nullptr, 'z'},
  {GETOPT_HELP_OPTION_DECL},
  {GETOPT_VERSION_OPTION_DECL},
  {0, 0, 0, 0},
};

static void
input_from_argv (char **operand, int n_operands, char eolbyte)
{
  char *p;
  size_t size = n_operands;
  int i;

  for (i = 0; i < n_operands; i++)
    size += strlen (operand[i]);
  p = xmalloc (size);

  for (i = 0; i < n_operands; i++)
    {
      char *p1 = stpcpy (p, operand[i]);
      operand[i] = p;
      p = p1;
      *p++ = eolbyte;
    }

  operand[n_operands] = p;
}

/* Return the start of the next line after LINE, which is guaranteed
   to end in EOLBYTE.  */

static char *
next_line (char *line, char eolbyte)
{
  char *p = rawmemchr (line, eolbyte);
  return p + 1;
}

/* Return the size of the input if possible or OFF_T_MAX if not.  */

static off_t
input_size (void)
{
  off_t file_size;

  struct stat stat_buf;
  if (fstat (STDIN_FILENO, &stat_buf) != 0)
    return OFF_T_MAX;
  if (usable_st_size (&stat_buf))
    file_size = stat_buf.st_size;
  else
    return OFF_T_MAX;

  off_t input_offset = lseek (STDIN_FILENO, 0, SEEK_CUR);
  if (input_offset < 0)
    return OFF_T_MAX;

  file_size -= input_offset;

  return file_size;
}

/* Read all lines and store up to K permuted lines in *OUT_RSRV.
   Return the number of lines read, up to a maximum of K.  */

static size_t
read_input_reservoir_sampling (FILE *in, char eolbyte, size_t k,
                               struct randint_source *s,
                               struct linebuffer **out_rsrv)
{
  randint n_lines = 0;
  size_t n_alloc_lines = MIN (k, RESERVOIR_LINES_INCREMENT);
  struct linebuffer *line = nullptr;
  struct linebuffer *rsrv;

  rsrv = xcalloc (n_alloc_lines, sizeof (struct linebuffer));

  /* Fill the first K lines, directly into the reservoir.  */
  while (n_lines < k
         && (line =
             readlinebuffer_delim (&rsrv[n_lines], in, eolbyte)) != nullptr)
    {
      n_lines++;

      /* Enlarge reservoir.  */
      if (n_lines >= n_alloc_lines)
        {
          n_alloc_lines += RESERVOIR_LINES_INCREMENT;
          rsrv = xnrealloc (rsrv, n_alloc_lines, sizeof (struct linebuffer));
          memset (&rsrv[n_lines], 0,
                  RESERVOIR_LINES_INCREMENT * sizeof (struct linebuffer));
        }
    }

  /* last line wasn't null - so there may be more lines to read.  */
  if (line != nullptr)
    {
      struct linebuffer dummy;
      initbuffer (&dummy);  /* space for lines not put in reservoir.  */

      /* Choose the fate of the next line, with decreasing probability (as
         n_lines increases in size).

         If the line will be used, store it directly in the reservoir.
         Otherwise, store it in dummy space.

         With 'struct linebuffer', storing into existing buffer will reduce
         re-allocations (will only re-allocate if the new line is longer than
         the currently allocated space).  */
      do
        {
          randint j = randint_choose (s, n_lines + 1);  /* 0 .. n_lines.  */
          line = (j < k) ? (&rsrv[j]) : (&dummy);
        }
      while (readlinebuffer_delim (line, in, eolbyte) != nullptr && n_lines++);

      if (! n_lines)
        error (EXIT_FAILURE, EOVERFLOW, _("too many input lines"));

      freebuffer (&dummy);
    }

  /* no more input lines, or an input error.  */
  if (ferror (in))
    error (EXIT_FAILURE, errno, _("read error"));

  *out_rsrv = rsrv;
  return MIN (k, n_lines);
}

static int
write_permuted_output_reservoir (size_t n_lines, struct linebuffer *lines,
                                 size_t const *permutation)
{
  for (size_t i = 0; i < n_lines; i++)
    {
      const struct linebuffer *p = &lines[permutation[i]];
      if (fwrite (p->buffer, sizeof (char), p->length, stdout) != p->length)
        return -1;
    }

  return 0;
}

/* Read data from file IN.  Input lines are delimited by EOLBYTE;
   silently append a trailing EOLBYTE if the file ends in some other
   byte.  Store a pointer to the resulting array of lines into *PLINE.
   Return the number of lines read.  Report an error and exit on
   failure.  */

static size_t
read_input (FILE *in, char eolbyte, char ***pline)
{
  char *p;
  char *buf = nullptr;
  size_t used;
  char *lim;
  char **line;
  size_t n_lines;

  /* TODO: We should limit the amount of data read here,
     to less than RESERVOIR_MIN_INPUT.  I.e., adjust fread_file() to support
     taking a byte limit.  We'd then need to ensure we handle a line spanning
     this boundary.  With that in place we could set use_reservoir_sampling
     when used==RESERVOIR_MIN_INPUT, and have read_input_reservoir_sampling()
     call a wrapper function to populate a linebuffer from the internal pline
     or if none left, stdin.  Doing that would give better performance by
     avoiding the reservoir CPU overhead when reading < RESERVOIR_MIN_INPUT
     from a pipe, and allow us to dispense with the input_size() function.  */
  if (!(buf = fread_file (in, 0, &used)))
    error (EXIT_FAILURE, errno, _("read error"));

  if (used && buf[used - 1] != eolbyte)
    buf[used++] = eolbyte;

  lim = buf + used;

  n_lines = 0;
  for (p = buf; p < lim; p = next_line (p, eolbyte))
    n_lines++;

  *pline = line = xnmalloc (n_lines + 1, sizeof *line);

  line[0] = p = buf;
  for (size_t i = 1; i <= n_lines; i++)
    line[i] = p = next_line (p, eolbyte);

  return n_lines;
}

/* Output N_LINES lines to stdout from LINE array,
   chosen by the indices in PERMUTATION.
   PERMUTATION and LINE must have at least N_LINES elements.
   Strings in LINE must include the line-terminator character.  */
static int
write_permuted_lines (size_t n_lines, char *const *line,
                      size_t const *permutation)
{
  for (size_t i = 0; i < n_lines; i++)
    {
      char *const *p = line + permutation[i];
      size_t len = p[1] - p[0];
      if (fwrite (p[0], sizeof *p[0], len, stdout) != len)
        return -1;
    }

  return 0;
}

/* Output N_LINES of numbers to stdout, from PERMUTATION array.
   PERMUTATION must have at least N_LINES elements.  */
static int
write_permuted_numbers (size_t n_lines, size_t lo_input,
                        size_t const *permutation, char eolbyte)
{
  for (size_t i = 0; i < n_lines; i++)
    {
      unsigned long int n = lo_input + permutation[i];
      if (printf ("%lu%c", n, eolbyte) < 0)
        return -1;
    }

  return 0;
}

/* Output COUNT numbers to stdout, chosen randomly from range
   LO_INPUT through HI_INPUT.  */
static int
write_random_numbers (struct randint_source *s, size_t count,
                      size_t lo_input, size_t hi_input, char eolbyte)
{
  const randint range = hi_input - lo_input + 1;

  for (size_t i = 0; i < count; i++)
    {
      unsigned long int j = lo_input + randint_choose (s, range);
      if (printf ("%lu%c", j, eolbyte) < 0)
        return -1;
    }

  return 0;
}

/* Output COUNT lines to stdout from LINES array.
   LINES must have at least N_LINES elements in it.
   Strings in LINES_ must include the line-terminator character.  */
static int
write_random_lines (struct randint_source *s, size_t count,
                    char *const *lines, size_t n_lines)
{
  for (size_t i = 0; i < count; i++)
    {
      const randint j = randint_choose (s, n_lines);
      char *const *p = lines + j;
      size_t len = p[1] - p[0];
      if (fwrite (p[0], sizeof *p[0], len, stdout) != len)
        return -1;
    }

  return 0;
}

int
main (int argc, char **argv)
{
  bool echo = false;
  bool input_range = false;
  size_t lo_input = SIZE_MAX;
  size_t hi_input = 0;
  size_t head_lines = SIZE_MAX;
  char const *outfile = nullptr;
  char *random_source = nullptr;
  char eolbyte = '\n';
  char **input_lines = nullptr;
  bool use_reservoir_sampling = false;
  bool repeat = false;

  int optc;
  int n_operands;
  char **operand;
  size_t n_lines;
  char **line = nullptr;
  struct linebuffer *reservoir = nullptr;
  struct randint_source *randint_source;
  size_t *permutation = nullptr;
  int i;

  initialize_main (&argc, &argv);
  set_program_name (argv[0]);
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  while ((optc = getopt_long (argc, argv, "ei:n:o:rz", long_opts, nullptr))
         != -1)
    switch (optc)
      {
      case 'e':
        echo = true;
        break;

      case 'i':
        {
          if (input_range)
            error (EXIT_FAILURE, 0, _("multiple -i options specified"));
          input_range = true;

          uintmax_t u;
          char *lo_end;
          strtol_error err = xstrtoumax (optarg, &lo_end, 10, &u, nullptr);
          if (err == LONGINT_OK)
            {
              lo_input = u;
              if (lo_input != u)
                err = LONGINT_OVERFLOW;
              else if (*lo_end != '-')
                err = LONGINT_INVALID;
              else
                {
                  err = xstrtoumax (lo_end + 1, nullptr, 10, &u, "");
                  if (err == LONGINT_OK)
                    {
                      hi_input = u;
                      if (hi_input != u)
                        err = LONGINT_OVERFLOW;
                    }
                }
            }

          n_lines = hi_input - lo_input + 1;

          if (err != LONGINT_OK || (lo_input <= hi_input) == (n_lines == 0))
            error (EXIT_FAILURE, err == LONGINT_OVERFLOW ? EOVERFLOW : 0,
                   "%s: %s", _("invalid input range"), quote (optarg));
        }
        break;

      case 'n':
        {
          uintmax_t argval;
          strtol_error e = xstrtoumax (optarg, nullptr, 10, &argval, "");

          if (e == LONGINT_OK)
            head_lines = MIN (head_lines, argval);
          else if (e != LONGINT_OVERFLOW)
            error (EXIT_FAILURE, 0, _("invalid line count: %s"),
                   quote (optarg));
        }
        break;

      case 'o':
        if (outfile && !STREQ (outfile, optarg))
          error (EXIT_FAILURE, 0, _("multiple output files specified"));
        outfile = optarg;
        break;

      case RANDOM_SOURCE_OPTION:
        if (random_source && !STREQ (random_source, optarg))
          error (EXIT_FAILURE, 0, _("multiple random sources specified"));
        random_source = optarg;
        break;

      case 'r':
        repeat = true;
        break;

      case 'z':
        eolbyte = '\0';
        break;

      case_GETOPT_HELP_CHAR;
      case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);
      default:
        usage (EXIT_FAILURE);
      }

  n_operands = argc - optind;
  operand = argv + optind;

  /* Check invalid usage.  */
  if (echo && input_range)
    {
      error (0, 0, _("cannot combine -e and -i options"));
      usage (EXIT_FAILURE);
    }
  if (input_range ? 0 < n_operands : !echo && 1 < n_operands)
    {
      error (0, 0, _("extra operand %s"), quote (operand[!input_range]));
      usage (EXIT_FAILURE);
    }

  /* Prepare input.  */
  if (head_lines == 0)
    {
      n_lines = 0;
      line = nullptr;
    }
  else if (echo)
    {
      input_from_argv (operand, n_operands, eolbyte);
      n_lines = n_operands;
      line = operand;
    }
  else if (input_range)
    {
      n_lines = hi_input - lo_input + 1;
      line = nullptr;
    }
  else
    {
      /* If an input file is specified, re-open it as stdin.  */
      if (n_operands == 1
          && ! (STREQ (operand[0], "-")
                || freopen (operand[0], "r", stdin)))
        error (EXIT_FAILURE, errno, "%s", quotef (operand[0]));

      fadvise (stdin, FADVISE_SEQUENTIAL);

      if (repeat || head_lines == SIZE_MAX
          || input_size () <= RESERVOIR_MIN_INPUT)
        {
          n_lines = read_input (stdin, eolbyte, &input_lines);
          line = input_lines;
        }
      else
        {
          use_reservoir_sampling = true;
          n_lines = SIZE_MAX;   /* unknown number of input lines, for now.  */
        }
    }

  /* The adjusted head line count; can be less than HEAD_LINES if the
     input is small and if not repeating.  */
  size_t ahead_lines = repeat || head_lines < n_lines ? head_lines : n_lines;

  randint_source = randint_all_new (random_source,
                                    (use_reservoir_sampling || repeat
                                     ? SIZE_MAX
                                     : randperm_bound (ahead_lines, n_lines)));
  if (! randint_source)
    error (EXIT_FAILURE, errno, "%s",
           quotef (random_source ? random_source : "getrandom"));

  if (use_reservoir_sampling)
    {
      /* Instead of reading the entire file into 'line',
         use reservoir-sampling to store just AHEAD_LINES random lines.  */
      n_lines = read_input_reservoir_sampling (stdin, eolbyte, ahead_lines,
                                               randint_source, &reservoir);
      ahead_lines = n_lines;
    }

  /* Close stdin now, rather than earlier, so that randint_all_new
     doesn't have to worry about opening something other than
     stdin.  */
  if (! (head_lines == 0 || echo || input_range || fclose (stdin) == 0))
    error (EXIT_FAILURE, errno, _("read error"));

  if (!repeat)
    permutation = randperm_new (randint_source, ahead_lines, n_lines);

  if (outfile && ! freopen (outfile, "w", stdout))
    error (EXIT_FAILURE, errno, "%s", quotef (outfile));

  /* Generate output according to requested method */
  if (repeat)
    {
      if (head_lines == 0)
        i = 0;
      else
        {
          if (n_lines == 0)
            error (EXIT_FAILURE, 0, _("no lines to repeat"));
          if (input_range)
            i = write_random_numbers (randint_source, ahead_lines,
                                      lo_input, hi_input, eolbyte);
          else
            i = write_random_lines (randint_source, ahead_lines, line, n_lines);
        }
    }
  else
    {
      if (use_reservoir_sampling)
        i = write_permuted_output_reservoir (n_lines, reservoir, permutation);
      else if (input_range)
        i = write_permuted_numbers (ahead_lines, lo_input,
                                    permutation, eolbyte);
      else
        i = write_permuted_lines (ahead_lines, line, permutation);
    }

  if (i != 0)
    write_error ();

  main_exit (EXIT_SUCCESS);
}
