/* dd -- convert a file while copying it.
   Copyright (C) 85, 90, 91, 1995-2002 Free Software Foundation, Inc.

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

/* Written by Paul Rubin, David MacKenzie, and Stuart Kemp. */

#include <config.h>
#include <stdio.h>

#define SWAB_ALIGN_OFFSET 2

#include <sys/types.h>
#include <signal.h>
#include <getopt.h>

#include "system.h"
#include "closeout.h"
#include "error.h"
#include "full-write.h"
#include "getpagesize.h"
#include "inttostr.h"
#include "long-options.h"
#include "quote.h"
#include "safe-read.h"
#include "xstrtol.h"

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "dd"

#define AUTHORS N_ ("Paul Rubin, David MacKenzie, and Stuart Kemp")

#ifndef SIGINFO
# define SIGINFO SIGUSR1
#endif

#ifndef S_TYPEISSHM
# define S_TYPEISSHM(Stat_ptr) 0
#endif

#define ROUND_UP_OFFSET(X, M) ((M) - 1 - (((X) + (M) - 1) % (M)))
#define PTR_ALIGN(Ptr, M) ((Ptr) \
			   + ROUND_UP_OFFSET ((char *)(Ptr) - (char *)0, (M)))

#define max(a, b) ((a) > (b) ? (a) : (b))
#define output_char(c)				\
  do						\
    {						\
      obuf[oc++] = (c);				\
      if (oc >= output_blocksize)		\
	write_output ();			\
    }						\
  while (0)

/* Default input and output blocksize. */
#define DEFAULT_BLOCKSIZE 512

/* Conversions bit masks. */
#define C_ASCII 01
#define C_EBCDIC 02
#define C_IBM 04
#define C_BLOCK 010
#define C_UNBLOCK 020
#define C_LCASE 040
#define C_UCASE 0100
#define C_SWAB 0200
#define C_NOERROR 0400
#define C_NOTRUNC 01000
#define C_SYNC 02000
/* Use separate input and output buffers, and combine partial input blocks. */
#define C_TWOBUFS 04000

/* The name this program was run with. */
char *program_name;

/* The name of the input file, or NULL for the standard input. */
static char const *input_file = NULL;

/* The name of the output file, or NULL for the standard output. */
static char const *output_file = NULL;

/* The number of bytes in which atomic reads are done. */
static size_t input_blocksize = 0;

/* The number of bytes in which atomic writes are done. */
static size_t output_blocksize = 0;

/* Conversion buffer size, in bytes.  0 prevents conversions. */
static size_t conversion_blocksize = 0;

/* Skip this many records of `input_blocksize' bytes before input. */
static uintmax_t skip_records = 0;

/* Skip this many records of `output_blocksize' bytes before output. */
static uintmax_t seek_records = 0;

/* Copy only this many records.  The default is effectively infinity.  */
static uintmax_t max_records = (uintmax_t) -1;

/* Bit vector of conversions to apply. */
static int conversions_mask = 0;

/* If nonzero, filter characters through the translation table.  */
static int translation_needed = 0;

/* Number of partial blocks written. */
static uintmax_t w_partial = 0;

/* Number of full blocks written. */
static uintmax_t w_full = 0;

/* Number of partial blocks read. */
static uintmax_t r_partial = 0;

/* Number of full blocks read. */
static uintmax_t r_full = 0;

/* Records truncated by conv=block. */
static uintmax_t r_truncate = 0;

/* Output representation of newline and space characters.
   They change if we're converting to EBCDIC.  */
static char newline_character = '\n';
static char space_character = ' ';

/* Output buffer. */
static char *obuf;

/* Current index into `obuf'. */
static size_t oc = 0;

/* Index into current line, for `conv=block' and `conv=unblock'.  */
static size_t col = 0;

struct conversion
{
  char *convname;
  int conversion;
};

static struct conversion conversions[] =
{
  {"ascii", C_ASCII | C_TWOBUFS},	/* EBCDIC to ASCII. */
  {"ebcdic", C_EBCDIC | C_TWOBUFS},	/* ASCII to EBCDIC. */
  {"ibm", C_IBM | C_TWOBUFS},	/* Slightly different ASCII to EBCDIC. */
  {"block", C_BLOCK | C_TWOBUFS},	/* Variable to fixed length records. */
  {"unblock", C_UNBLOCK | C_TWOBUFS},	/* Fixed to variable length records. */
  {"lcase", C_LCASE | C_TWOBUFS},	/* Translate upper to lower case. */
  {"ucase", C_UCASE | C_TWOBUFS},	/* Translate lower to upper case. */
  {"swab", C_SWAB | C_TWOBUFS},	/* Swap bytes of input. */
  {"noerror", C_NOERROR},	/* Ignore i/o errors. */
  {"notrunc", C_NOTRUNC},	/* Do not truncate output file. */
  {"sync", C_SYNC},		/* Pad input records to ibs with NULs. */
  {NULL, 0}
};

/* Translation table formed by applying successive transformations. */
static unsigned char trans_table[256];

static char const ascii_to_ebcdic[] =
{
  '\000', '\001', '\002', '\003', '\067', '\055', '\056', '\057',
  '\026', '\005', '\045', '\013', '\014', '\015', '\016', '\017',
  '\020', '\021', '\022', '\023', '\074', '\075', '\062', '\046',
  '\030', '\031', '\077', '\047', '\034', '\035', '\036', '\037',
  '\100', '\117', '\177', '\173', '\133', '\154', '\120', '\175',
  '\115', '\135', '\134', '\116', '\153', '\140', '\113', '\141',
  '\360', '\361', '\362', '\363', '\364', '\365', '\366', '\367',
  '\370', '\371', '\172', '\136', '\114', '\176', '\156', '\157',
  '\174', '\301', '\302', '\303', '\304', '\305', '\306', '\307',
  '\310', '\311', '\321', '\322', '\323', '\324', '\325', '\326',
  '\327', '\330', '\331', '\342', '\343', '\344', '\345', '\346',
  '\347', '\350', '\351', '\112', '\340', '\132', '\137', '\155',
  '\171', '\201', '\202', '\203', '\204', '\205', '\206', '\207',
  '\210', '\211', '\221', '\222', '\223', '\224', '\225', '\226',
  '\227', '\230', '\231', '\242', '\243', '\244', '\245', '\246',
  '\247', '\250', '\251', '\300', '\152', '\320', '\241', '\007',
  '\040', '\041', '\042', '\043', '\044', '\025', '\006', '\027',
  '\050', '\051', '\052', '\053', '\054', '\011', '\012', '\033',
  '\060', '\061', '\032', '\063', '\064', '\065', '\066', '\010',
  '\070', '\071', '\072', '\073', '\004', '\024', '\076', '\341',
  '\101', '\102', '\103', '\104', '\105', '\106', '\107', '\110',
  '\111', '\121', '\122', '\123', '\124', '\125', '\126', '\127',
  '\130', '\131', '\142', '\143', '\144', '\145', '\146', '\147',
  '\150', '\151', '\160', '\161', '\162', '\163', '\164', '\165',
  '\166', '\167', '\170', '\200', '\212', '\213', '\214', '\215',
  '\216', '\217', '\220', '\232', '\233', '\234', '\235', '\236',
  '\237', '\240', '\252', '\253', '\254', '\255', '\256', '\257',
  '\260', '\261', '\262', '\263', '\264', '\265', '\266', '\267',
  '\270', '\271', '\272', '\273', '\274', '\275', '\276', '\277',
  '\312', '\313', '\314', '\315', '\316', '\317', '\332', '\333',
  '\334', '\335', '\336', '\337', '\352', '\353', '\354', '\355',
  '\356', '\357', '\372', '\373', '\374', '\375', '\376', '\377'
};

static char const ascii_to_ibm[] =
{
  '\000', '\001', '\002', '\003', '\067', '\055', '\056', '\057',
  '\026', '\005', '\045', '\013', '\014', '\015', '\016', '\017',
  '\020', '\021', '\022', '\023', '\074', '\075', '\062', '\046',
  '\030', '\031', '\077', '\047', '\034', '\035', '\036', '\037',
  '\100', '\132', '\177', '\173', '\133', '\154', '\120', '\175',
  '\115', '\135', '\134', '\116', '\153', '\140', '\113', '\141',
  '\360', '\361', '\362', '\363', '\364', '\365', '\366', '\367',
  '\370', '\371', '\172', '\136', '\114', '\176', '\156', '\157',
  '\174', '\301', '\302', '\303', '\304', '\305', '\306', '\307',
  '\310', '\311', '\321', '\322', '\323', '\324', '\325', '\326',
  '\327', '\330', '\331', '\342', '\343', '\344', '\345', '\346',
  '\347', '\350', '\351', '\255', '\340', '\275', '\137', '\155',
  '\171', '\201', '\202', '\203', '\204', '\205', '\206', '\207',
  '\210', '\211', '\221', '\222', '\223', '\224', '\225', '\226',
  '\227', '\230', '\231', '\242', '\243', '\244', '\245', '\246',
  '\247', '\250', '\251', '\300', '\117', '\320', '\241', '\007',
  '\040', '\041', '\042', '\043', '\044', '\025', '\006', '\027',
  '\050', '\051', '\052', '\053', '\054', '\011', '\012', '\033',
  '\060', '\061', '\032', '\063', '\064', '\065', '\066', '\010',
  '\070', '\071', '\072', '\073', '\004', '\024', '\076', '\341',
  '\101', '\102', '\103', '\104', '\105', '\106', '\107', '\110',
  '\111', '\121', '\122', '\123', '\124', '\125', '\126', '\127',
  '\130', '\131', '\142', '\143', '\144', '\145', '\146', '\147',
  '\150', '\151', '\160', '\161', '\162', '\163', '\164', '\165',
  '\166', '\167', '\170', '\200', '\212', '\213', '\214', '\215',
  '\216', '\217', '\220', '\232', '\233', '\234', '\235', '\236',
  '\237', '\240', '\252', '\253', '\254', '\255', '\256', '\257',
  '\260', '\261', '\262', '\263', '\264', '\265', '\266', '\267',
  '\270', '\271', '\272', '\273', '\274', '\275', '\276', '\277',
  '\312', '\313', '\314', '\315', '\316', '\317', '\332', '\333',
  '\334', '\335', '\336', '\337', '\352', '\353', '\354', '\355',
  '\356', '\357', '\372', '\373', '\374', '\375', '\376', '\377'
};

static char const ebcdic_to_ascii[] =
{
  '\000', '\001', '\002', '\003', '\234', '\011', '\206', '\177',
  '\227', '\215', '\216', '\013', '\014', '\015', '\016', '\017',
  '\020', '\021', '\022', '\023', '\235', '\205', '\010', '\207',
  '\030', '\031', '\222', '\217', '\034', '\035', '\036', '\037',
  '\200', '\201', '\202', '\203', '\204', '\012', '\027', '\033',
  '\210', '\211', '\212', '\213', '\214', '\005', '\006', '\007',
  '\220', '\221', '\026', '\223', '\224', '\225', '\226', '\004',
  '\230', '\231', '\232', '\233', '\024', '\025', '\236', '\032',
  '\040', '\240', '\241', '\242', '\243', '\244', '\245', '\246',
  '\247', '\250', '\133', '\056', '\074', '\050', '\053', '\041',
  '\046', '\251', '\252', '\253', '\254', '\255', '\256', '\257',
  '\260', '\261', '\135', '\044', '\052', '\051', '\073', '\136',
  '\055', '\057', '\262', '\263', '\264', '\265', '\266', '\267',
  '\270', '\271', '\174', '\054', '\045', '\137', '\076', '\077',
  '\272', '\273', '\274', '\275', '\276', '\277', '\300', '\301',
  '\302', '\140', '\072', '\043', '\100', '\047', '\075', '\042',
  '\303', '\141', '\142', '\143', '\144', '\145', '\146', '\147',
  '\150', '\151', '\304', '\305', '\306', '\307', '\310', '\311',
  '\312', '\152', '\153', '\154', '\155', '\156', '\157', '\160',
  '\161', '\162', '\313', '\314', '\315', '\316', '\317', '\320',
  '\321', '\176', '\163', '\164', '\165', '\166', '\167', '\170',
  '\171', '\172', '\322', '\323', '\324', '\325', '\326', '\327',
  '\330', '\331', '\332', '\333', '\334', '\335', '\336', '\337',
  '\340', '\341', '\342', '\343', '\344', '\345', '\346', '\347',
  '\173', '\101', '\102', '\103', '\104', '\105', '\106', '\107',
  '\110', '\111', '\350', '\351', '\352', '\353', '\354', '\355',
  '\175', '\112', '\113', '\114', '\115', '\116', '\117', '\120',
  '\121', '\122', '\356', '\357', '\360', '\361', '\362', '\363',
  '\134', '\237', '\123', '\124', '\125', '\126', '\127', '\130',
  '\131', '\132', '\364', '\365', '\366', '\367', '\370', '\371',
  '\060', '\061', '\062', '\063', '\064', '\065', '\066', '\067',
  '\070', '\071', '\372', '\373', '\374', '\375', '\376', '\377'
};

void
usage (int status)
{
  if (status != 0)
    fprintf (stderr, _("Try `%s --help' for more information.\n"),
	     program_name);
  else
    {
      printf (_("Usage: %s [OPTION]...\n"), program_name);
      fputs (_("\
Copy a file, converting and formatting according to the options.\n\
\n\
  bs=BYTES        force ibs=BYTES and obs=BYTES\n\
  cbs=BYTES       convert BYTES bytes at a time\n\
  conv=KEYWORDS   convert the file as per the comma separated keyword list\n\
  count=BLOCKS    copy only BLOCKS input blocks\n\
  ibs=BYTES       read BYTES bytes at a time\n\
"), stdout);
      fputs (_("\
  if=FILE         read from FILE instead of stdin\n\
  obs=BYTES       write BYTES bytes at a time\n\
  of=FILE         write to FILE instead of stdout\n\
  seek=BLOCKS     skip BLOCKS obs-sized blocks at start of output\n\
  skip=BLOCKS     skip BLOCKS ibs-sized blocks at start of input\n\
"), stdout);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      fputs (_("\
\n\
BLOCKS and BYTES may be followed by the following multiplicative suffixes:\n\
xM M, c 1, w 2, b 512, kB 1000, K 1024, MB 1,000,000, M 1,048,576,\n\
GB 1,000,000,000, G 1,073,741,824, and so on for T, P, E, Z, Y.\n\
Each KEYWORD may be:\n\
\n\
"), stdout);
      fputs (_("\
  ascii     from EBCDIC to ASCII\n\
  ebcdic    from ASCII to EBCDIC\n\
  ibm       from ASCII to alternated EBCDIC\n\
  block     pad newline-terminated records with spaces to cbs-size\n\
  unblock   replace trailing spaces in cbs-size records with newline\n\
  lcase     change upper case to lower case\n\
"), stdout);
      fputs (_("\
  notrunc   do not truncate the output file\n\
  ucase     change lower case to upper case\n\
  swab      swap every pair of input bytes\n\
  noerror   continue after read errors\n\
  sync      pad every input block with NULs to ibs-size; when used\n\
              with block or unblock, pad with spaces rather than NULs\n\
"), stdout);
      printf (_("\nReport bugs to <%s>.\n"), PACKAGE_BUGREPORT);
    }
  exit (status);
}

static void
translate_charset (char const *new_trans)
{
  int i;

  for (i = 0; i < 256; i++)
    trans_table[i] = new_trans[trans_table[i]];
  translation_needed = 1;
}

/* Return the number of 1 bits in `i'. */

static int
bit_count (register int i)
{
  register int set_bits;

  for (set_bits = 0; i != 0; set_bits++)
    i &= i - 1;
  return set_bits;
}

static void
print_stats (void)
{
  char buf[2][INT_BUFSIZE_BOUND (uintmax_t)];
  fprintf (stderr, _("%s+%s records in\n"),
	   umaxtostr (r_full, buf[0]), umaxtostr (r_partial, buf[1]));
  fprintf (stderr, _("%s+%s records out\n"),
	   umaxtostr (w_full, buf[0]), umaxtostr (w_partial, buf[1]));
  if (r_truncate > 0)
    {
      fprintf (stderr, "%s %s\n",
	       umaxtostr (r_truncate, buf[0]),
	       (r_truncate == 1
		? _("truncated record")
		: _("truncated records")));
    }
}

static void
cleanup (void)
{
  print_stats ();
  if (close (STDIN_FILENO) < 0)
    error (EXIT_FAILURE, errno,
	   _("closing input file %s"), quote (input_file));
  if (close (STDOUT_FILENO) < 0)
    error (EXIT_FAILURE, errno,
	   _("closing output file %s"), quote (output_file));
}

static inline void
quit (int code)
{
  cleanup ();
  exit (code);
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

static RETSIGTYPE
siginfo_handler (int sig ATTRIBUTE_UNUSED)
{
  print_stats ();
}

/* Encapsulate portability mess of establishing signal handlers.  */

static void
install_handler (int sig_num, RETSIGTYPE (*sig_handler) (int sig))
{
#ifdef SA_NOCLDSTOP
  struct sigaction sigact;
  sigaction (sig_num, NULL, &sigact);
  if (sigact.sa_handler != SIG_IGN)
    {
      sigact.sa_handler = sig_handler;
      sigemptyset (&sigact.sa_mask);
      sigact.sa_flags = 0;
      sigaction (sig_num, &sigact, NULL);
    }
#else
  if (signal (sig_num, SIG_IGN) != SIG_IGN)
    signal (sig_num, sig_handler);
#endif
}

/* Open a file to a particular file descriptor.  This is like standard
   `open', except it always returns DESIRED_FD if successful.  */
static int
open_fd (int desired_fd, char const *filename, int options, mode_t mode)
{
  int fd;
  close (desired_fd);
  fd = open (filename, options, mode);
  if (fd < 0)
    return -1;

  if (fd != desired_fd)
    {
      if (dup2 (fd, desired_fd) != desired_fd)
	desired_fd = -1;
      if (close (fd) != 0)
	return -1;
    }

  return desired_fd;
}

/* Write, then empty, the output buffer `obuf'. */

static void
write_output (void)
{
  size_t nwritten = full_write (STDOUT_FILENO, obuf, output_blocksize);
  if (nwritten != output_blocksize)
    {
      error (0, errno, _("writing to %s"), quote (output_file));
      if (nwritten != 0)
	w_partial++;
      quit (1);
    }
  else
    w_full++;
  oc = 0;
}

/* Interpret one "conv=..." option.
   As a by product, this function replaces each `,' in STR with a NUL byte.  */

static void
parse_conversion (char *str)
{
  char *new;
  int i;

  do
    {
      new = strchr (str, ',');
      if (new != NULL)
	*new++ = '\0';
      for (i = 0; conversions[i].convname != NULL; i++)
	if (STREQ (conversions[i].convname, str))
	  {
	    conversions_mask |= conversions[i].conversion;
	    break;
	  }
      if (conversions[i].convname == NULL)
	{
	  error (0, 0, _("invalid conversion: %s"), quote (str));
	  usage (EXIT_FAILURE);
	}
      str = new;
  } while (new != NULL);
}

/* Return the value of STR, interpreted as a non-negative decimal integer,
   optionally multiplied by various values.
   Assign nonzero to *INVALID if STR does not represent a number in
   this format. */

static uintmax_t
parse_integer (const char *str, int *invalid)
{
  uintmax_t n;
  char *suffix;
  enum strtol_error e = xstrtoumax (str, &suffix, 10, &n, "bcEGkKMPTwYZ0");

  if (e == LONGINT_INVALID_SUFFIX_CHAR && *suffix == 'x')
    {
      uintmax_t multiplier = parse_integer (suffix + 1, invalid);

      if (multiplier != 0 && n * multiplier / multiplier != n)
	{
	  *invalid = 1;
	  return 0;
	}

      n *= multiplier;
    }
  else if (e != LONGINT_OK)
    {
      *invalid = 1;
      return 0;
    }

  return n;
}

static void
scanargs (int argc, char **argv)
{
  int i;

  --argc;
  ++argv;

  for (i = optind; i < argc; i++)
    {
      char *name, *val;

      name = argv[i];
      val = strchr (name, '=');
      if (val == NULL)
	{
	  error (0, 0, _("unrecognized option %s"), quote (name));
	  usage (EXIT_FAILURE);
	}
      *val++ = '\0';

      if (STREQ (name, "if"))
	input_file = val;
      else if (STREQ (name, "of"))
	output_file = val;
      else if (STREQ (name, "conv"))
	parse_conversion (val);
      else
	{
	  int invalid = 0;
	  uintmax_t n = parse_integer (val, &invalid);

	  if (STREQ (name, "ibs"))
	    {
	      /* Ensure that each blocksize is <= SSIZE_MAX.  */
	      invalid |= SSIZE_MAX < n;
	      input_blocksize = n;
	      invalid |= input_blocksize != n || input_blocksize == 0;
	      conversions_mask |= C_TWOBUFS;
	    }
	  else if (STREQ (name, "obs"))
	    {
	      /* Ensure that each blocksize is <= SSIZE_MAX.  */
	      invalid |= SSIZE_MAX < n;
	      output_blocksize = n;
	      invalid |= output_blocksize != n || output_blocksize == 0;
	      conversions_mask |= C_TWOBUFS;
	    }
	  else if (STREQ (name, "bs"))
	    {
	      /* Ensure that each blocksize is <= SSIZE_MAX.  */
	      invalid |= SSIZE_MAX < n;
	      output_blocksize = input_blocksize = n;
	      invalid |= output_blocksize != n || output_blocksize == 0;
	    }
	  else if (STREQ (name, "cbs"))
	    {
	      conversion_blocksize = n;
	      invalid |= (conversion_blocksize != n
			  || conversion_blocksize == 0);
	    }
	  else if (STREQ (name, "skip"))
	    skip_records = n;
	  else if (STREQ (name, "seek"))
	    seek_records = n;
	  else if (STREQ (name, "count"))
	    max_records = n;
	  else
	    {
	      error (0, 0, _("unrecognized option %s=%s"),
		     quote_n (0, name), quote_n (1, val));
	      usage (EXIT_FAILURE);
	    }

	  if (invalid)
	    error (EXIT_FAILURE, 0, _("invalid number %s"), quote (val));
	}
    }

  /* If bs= was given, both `input_blocksize' and `output_blocksize' will
     have been set to positive values.  If either has not been set,
     bs= was not given, so make sure two buffers are used. */
  if (input_blocksize == 0 || output_blocksize == 0)
    conversions_mask |= C_TWOBUFS;
  if (input_blocksize == 0)
    input_blocksize = DEFAULT_BLOCKSIZE;
  if (output_blocksize == 0)
    output_blocksize = DEFAULT_BLOCKSIZE;
  if (conversion_blocksize == 0)
    conversions_mask &= ~(C_BLOCK | C_UNBLOCK);
}

/* Fix up translation table. */

static void
apply_translations (void)
{
  int i;

#define MX(a) (bit_count (conversions_mask & (a)))
  if ((MX (C_ASCII | C_EBCDIC | C_IBM) > 1)
      || (MX (C_BLOCK | C_UNBLOCK) > 1)
      || (MX (C_LCASE | C_UCASE) > 1)
      || (MX (C_UNBLOCK | C_SYNC) > 1))
    {
      error (EXIT_FAILURE, 0, _("\
only one conv in {ascii,ebcdic,ibm}, {lcase,ucase}, {block,unblock}, {unblock,sync}"));
    }
#undef MX

  if (conversions_mask & C_ASCII)
    translate_charset (ebcdic_to_ascii);

  if (conversions_mask & C_UCASE)
    {
      for (i = 0; i < 256; i++)
	if (ISLOWER (trans_table[i]))
	  trans_table[i] = TOUPPER (trans_table[i]);
      translation_needed = 1;
    }
  else if (conversions_mask & C_LCASE)
    {
      for (i = 0; i < 256; i++)
	if (ISUPPER (trans_table[i]))
	  trans_table[i] = TOLOWER (trans_table[i]);
      translation_needed = 1;
    }

  if (conversions_mask & C_EBCDIC)
    {
      translate_charset (ascii_to_ebcdic);
      newline_character = ascii_to_ebcdic['\n'];
      space_character = ascii_to_ebcdic[' '];
    }
  else if (conversions_mask & C_IBM)
    {
      translate_charset (ascii_to_ibm);
      newline_character = ascii_to_ibm['\n'];
      space_character = ascii_to_ibm[' '];
    }
}

/* Apply the character-set translations specified by the user
   to the NREAD bytes in BUF.  */

static void
translate_buffer (char *buf, size_t nread)
{
  char *cp;
  size_t i;

  for (i = nread, cp = buf; i; i--, cp++)
    *cp = trans_table[(unsigned char) *cp];
}

/* If nonnzero, the last char from the previous call to `swab_buffer'
   is saved in `saved_char'.  */
static int char_is_saved = 0;

/* Odd char from previous call.  */
static char saved_char;

/* Swap NREAD bytes in BUF, plus possibly an initial char from the
   previous call.  If NREAD is odd, save the last char for the
   next call.   Return the new start of the BUF buffer.  */

static char *
swab_buffer (char *buf, size_t *nread)
{
  char *bufstart = buf;
  register char *cp;
  register int i;

  /* Is a char left from last time?  */
  if (char_is_saved)
    {
      *--bufstart = saved_char;
      (*nread)++;
      char_is_saved = 0;
    }

  if (*nread & 1)
    {
      /* An odd number of chars are in the buffer.  */
      saved_char = bufstart[--*nread];
      char_is_saved = 1;
    }

  /* Do the byte-swapping by moving every second character two
     positions toward the end, working from the end of the buffer
     toward the beginning.  This way we only move half of the data.  */

  cp = bufstart + *nread;	/* Start one char past the last.  */
  for (i = *nread / 2; i; i--, cp -= 2)
    *cp = *(cp - 2);

  return ++bufstart;
}

/* This is a wrapper for lseek.  It detects and warns about a kernel
   bug that makes lseek a no-op for tape devices, even though the kernel
   lseek return value suggests that the function succeeded.

   The parameters are the same as those of the lseek function, but
   with the addition of FILENAME, the name of the file associated with
   descriptor FDESC.  The file name is used solely in the warning that's
   printed when the bug is detected.  Return the same value that lseek
   would have returned, but when the lseek bug is detected, return -1
   to indicate that lseek failed.

   The offending behavior has been confirmed with an Exabyte SCSI tape
   drive accessed via /dev/nst0 on both Linux-2.2.17 and Linux-2.4.16.  */

#ifdef __linux__

# include <sys/mtio.h>

# define MT_SAME_POSITION(P, Q) \
   ((P).mt_resid == (Q).mt_resid \
    && (P).mt_fileno == (Q).mt_fileno \
    && (P).mt_blkno == (Q).mt_blkno)

static off_t
skip_via_lseek (char const *filename, int fdesc, off_t offset, int whence)
{
  struct mtget s1;
  struct mtget s2;
  off_t new_position;
  int got_original_tape_position;

  got_original_tape_position = (ioctl (fdesc, MTIOCGET, &s1) == 0);
  /* known bad device type */
  /* && s.mt_type == MT_ISSCSI2 */

  new_position = lseek (fdesc, offset, whence);
  if (0 <= new_position
      && got_original_tape_position
      && ioctl (fdesc, MTIOCGET, &s2) == 0
      && MT_SAME_POSITION (s1, s2))
    {
      error (0, 0, _("warning: working around lseek kernel bug for file (%s)\n\
  of mt_type=0x%0lx -- see <sys/mtio.h> for the list of types"),
	     filename, s2.mt_type);
      new_position = -1;
    }

  return new_position;
}
#else
# define skip_via_lseek(Filename, Fd, Offset, Whence) lseek (Fd, Offset, Whence)
#endif

/* Throw away RECORDS blocks of BLOCKSIZE bytes on file descriptor FDESC,
   which is open with read permission for FILE.  Store up to BLOCKSIZE
   bytes of the data at a time in BUF, if necessary.  RECORDS must be
   nonzero.  */

static void
skip (int fdesc, char const *file, uintmax_t records, size_t blocksize,
      char *buf)
{
  off_t offset = records * blocksize;

  /* Try lseek and if an error indicates it was an inappropriate operation --
     or if the the file offset is not representable as an off_t --
     fall back on using read.  */

  if ((uintmax_t) offset / blocksize != records
      || skip_via_lseek (file, fdesc, offset, SEEK_CUR) < 0)
    {
      while (records--)
	{
	  size_t nread = safe_read (fdesc, buf, blocksize);
	  if (nread == SAFE_READ_ERROR)
	    {
	      error (0, errno, _("reading %s"), quote (file));
	      quit (1);
	    }
	  /* POSIX doesn't say what to do when dd detects it has been
	     asked to skip past EOF, so I assume it's non-fatal.
	     FIXME: maybe give a warning.  */
	  if (nread == 0)
	    break;
	}
    }
}

/* Copy NREAD bytes of BUF, with no conversions.  */

static void
copy_simple (char const *buf, int nread)
{
  int nfree;			/* Number of unused bytes in `obuf'.  */
  const char *start = buf;	/* First uncopied char in BUF.  */

  do
    {
      nfree = output_blocksize - oc;
      if (nfree > nread)
	nfree = nread;

      memcpy (obuf + oc, start, nfree);

      nread -= nfree;		/* Update the number of bytes left to copy. */
      start += nfree;
      oc += nfree;
      if (oc >= output_blocksize)
	write_output ();
    }
  while (nread > 0);
}

/* Copy NREAD bytes of BUF, doing conv=block
   (pad newline-terminated records to `conversion_blocksize',
   replacing the newline with trailing spaces).  */

static void
copy_with_block (char const *buf, size_t nread)
{
  size_t i;

  for (i = nread; i; i--, buf++)
    {
      if (*buf == newline_character)
	{
	  if (col < conversion_blocksize)
	    {
	      size_t j;
	      for (j = col; j < conversion_blocksize; j++)
		output_char (space_character);
	    }
	  col = 0;
	}
      else
	{
	  if (col == conversion_blocksize)
	    r_truncate++;
	  else if (col < conversion_blocksize)
	    output_char (*buf);
	  col++;
	}
    }
}

/* Copy NREAD bytes of BUF, doing conv=unblock
   (replace trailing spaces in `conversion_blocksize'-sized records
   with a newline).  */

static void
copy_with_unblock (char const *buf, size_t nread)
{
  size_t i;
  char c;
  static int pending_spaces = 0;

  for (i = 0; i < nread; i++)
    {
      c = buf[i];

      if (col++ >= conversion_blocksize)
	{
	  col = pending_spaces = 0; /* Wipe out any pending spaces.  */
	  i--;			/* Push the char back; get it later. */
	  output_char (newline_character);
	}
      else if (c == space_character)
	pending_spaces++;
      else
	{
	  /* `c' is the character after a run of spaces that were not
	     at the end of the conversion buffer.  Output them.  */
	  while (pending_spaces)
	    {
	      output_char (space_character);
	      --pending_spaces;
	    }
	  output_char (c);
	}
    }
}

/* The main loop.  */

static int
dd_copy (void)
{
  char *ibuf, *bufstart;	/* Input buffer. */
  char *real_buf;		/* real buffer address before alignment */
  char *real_obuf;
  size_t nread;			/* Bytes read in the current block. */
  int exit_status = 0;
  size_t page_size = getpagesize ();
  size_t n_bytes_read;

  /* Leave at least one extra byte at the beginning and end of `ibuf'
     for conv=swab, but keep the buffer address even.  But some peculiar
     device drivers work only with word-aligned buffers, so leave an
     extra two bytes.  */

  /* Some devices require alignment on a sector or page boundary
     (e.g. character disk devices).  Align the input buffer to a
     page boundary to cover all bases.  Note that due to the swab
     algorithm, we must have at least one byte in the page before
     the input buffer;  thus we allocate 2 pages of slop in the
     real buffer.  8k above the blocksize shouldn't bother anyone.

     The page alignment is necessary on any linux system that supports
     either the SGI raw I/O patch or Steven Tweedies raw I/O patch.
     It is necessary when accessing raw (i.e. character special) disk
     devices on Unixware or other SVR4-derived system.  */

  real_buf = xmalloc (input_blocksize
		      + 2 * SWAB_ALIGN_OFFSET
		      + 2 * page_size - 1);
  ibuf = real_buf;
  ibuf += SWAB_ALIGN_OFFSET;	/* allow space for swab */

  ibuf = PTR_ALIGN (ibuf, page_size);

  if (conversions_mask & C_TWOBUFS)
    {
      /* Page-align the output buffer, too.  */
      real_obuf = xmalloc (output_blocksize + page_size - 1);
      obuf = PTR_ALIGN (real_obuf, page_size);
    }
  else
    {
      real_obuf = NULL;
      obuf = ibuf;
    }

  if (skip_records != 0)
    skip (STDIN_FILENO, input_file, skip_records, input_blocksize, ibuf);

  if (seek_records != 0)
    {
      /* FIXME: this loses for
	 % ./dd if=dd seek=1 |:
	 ./dd: standard output: Bad file descriptor
	 0+0 records in
	 0+0 records out
	 */

      skip (STDOUT_FILENO, output_file, seek_records, output_blocksize, obuf);
    }

  if (max_records == 0)
    quit (exit_status);

  while (1)
    {
      if (r_partial + r_full >= max_records)
	break;

      /* Zero the buffer before reading, so that if we get a read error,
	 whatever data we are able to read is followed by zeros.
	 This minimizes data loss. */
      if ((conversions_mask & C_SYNC) && (conversions_mask & C_NOERROR))
	memset (ibuf,
		(conversions_mask & (C_BLOCK | C_UNBLOCK)) ? ' ' : '\0',
		input_blocksize);

      nread = safe_read (STDIN_FILENO, ibuf, input_blocksize);

      if (nread == 0)
	break;			/* EOF.  */

      if (nread == SAFE_READ_ERROR)
	{
	  error (0, errno, _("reading %s"), quote (input_file));
	  if (conversions_mask & C_NOERROR)
	    {
	      print_stats ();
	      /* Seek past the bad block if possible. */
	      lseek (STDIN_FILENO, (off_t) input_blocksize, SEEK_CUR);
	      if (conversions_mask & C_SYNC)
		/* Replace the missing input with null bytes and
		   proceed normally.  */
		nread = 0;
	      else
		continue;
	    }
	  else
	    {
	      /* Write any partial block. */
	      exit_status = 2;
	      break;
	    }
	}

      n_bytes_read = nread;

      if (n_bytes_read < input_blocksize)
	{
	  r_partial++;
	  if (conversions_mask & C_SYNC)
	    {
	      if (!(conversions_mask & C_NOERROR))
		/* If C_NOERROR, we zeroed the block before reading. */
		memset (ibuf + n_bytes_read,
			(conversions_mask & (C_BLOCK | C_UNBLOCK)) ? ' ' : '\0',
			input_blocksize - n_bytes_read);
	      n_bytes_read = input_blocksize;
	    }
	}
      else
	r_full++;

      if (ibuf == obuf)		/* If not C_TWOBUFS. */
	{
	  size_t nwritten = full_write (STDOUT_FILENO, obuf, n_bytes_read);
	  if (nwritten != n_bytes_read)
	    {
	      error (0, errno, _("writing %s"), quote (output_file));
	      quit (1);
	    }
	  else if (n_bytes_read == input_blocksize)
	    w_full++;
	  else
	    w_partial++;
	  continue;
	}

      /* Do any translations on the whole buffer at once.  */

      if (translation_needed)
	translate_buffer (ibuf, n_bytes_read);

      if (conversions_mask & C_SWAB)
	bufstart = swab_buffer (ibuf, &n_bytes_read);
      else
	bufstart = ibuf;

      if (conversions_mask & C_BLOCK)
        copy_with_block (bufstart, n_bytes_read);
      else if (conversions_mask & C_UNBLOCK)
	copy_with_unblock (bufstart, n_bytes_read);
      else
	copy_simple (bufstart, n_bytes_read);
    }

  /* If we have a char left as a result of conv=swab, output it.  */
  if (char_is_saved)
    {
      if (conversions_mask & C_BLOCK)
        copy_with_block (&saved_char, 1);
      else if (conversions_mask & C_UNBLOCK)
	copy_with_unblock (&saved_char, 1);
      else
	output_char (saved_char);
    }

  if ((conversions_mask & C_BLOCK) && col > 0)
    {
      /* If the final input line didn't end with a '\n', pad
	 the output block to `conversion_blocksize' chars.  */
      size_t i;
      for (i = col; i < conversion_blocksize; i++)
	output_char (space_character);
    }

  if ((conversions_mask & C_UNBLOCK) && col == conversion_blocksize)
    /* Add a final '\n' if there are exactly `conversion_blocksize'
       characters in the final record. */
    output_char (newline_character);

  /* Write out the last block. */
  if (oc != 0)
    {
      size_t nwritten = full_write (STDOUT_FILENO, obuf, oc);
      if (nwritten != 0)
	w_partial++;
      if (nwritten != oc)
	{
	  error (0, errno, _("writing %s"), quote (output_file));
	  quit (1);
	}
    }

  free (real_buf);
  if (real_obuf)
    free (real_obuf);

  return exit_status;
}

/* This is gross, but necessary, because of the way close_stdout
   works and because this program closes STDOUT_FILENO directly.  */
static void (*closeout_func) (void) = close_stdout;

static void
close_stdout_wrapper (void)
{
  if (closeout_func)
    (*closeout_func) ();
}

int
main (int argc, char **argv)
{
  int i;
  int exit_status;

  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  /* Arrange to close stdout if parse_long_options exits.  */
  atexit (close_stdout_wrapper);

  parse_long_options (argc, argv, PROGRAM_NAME, PACKAGE, VERSION,
		      AUTHORS, usage);

  /* Don't close stdout on exit from here on.  */
  closeout_func = NULL;

  /* Initialize translation table to identity translation. */
  for (i = 0; i < 256; i++)
    trans_table[i] = i;

  /* Decode arguments. */
  scanargs (argc, argv);

  apply_translations ();

  if (input_file != NULL)
    {
      if (open_fd (STDIN_FILENO, input_file, O_RDONLY, 0) < 0)
	error (EXIT_FAILURE, errno, _("opening %s"), quote (input_file));
    }
  else
    input_file = _("standard input");

  if (output_file != NULL)
    {
      mode_t perms = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
      int opts
	= (O_CREAT
	   | (seek_records || (conversions_mask & C_NOTRUNC) ? 0 : O_TRUNC));

      /* Open the output file with *read* access only if we might
	 need to read to satisfy a `seek=' request.  If we can't read
	 the file, go ahead with write-only access; it might work.  */
      if ((! seek_records
	   || open_fd (STDOUT_FILENO, output_file, O_RDWR | opts, perms) < 0)
	  && open_fd (STDOUT_FILENO, output_file, O_WRONLY | opts, perms) < 0)
	error (EXIT_FAILURE, errno, _("opening %s"), quote (output_file));

#if HAVE_FTRUNCATE
      if (seek_records != 0 && !(conversions_mask & C_NOTRUNC))
	{
	  struct stat stdout_stat;
	  off_t o = seek_records * output_blocksize;
	  if ((uintmax_t) o / output_blocksize != seek_records)
	    error (EXIT_FAILURE, 0, _("file offset out of range"));

	  if (fstat (STDOUT_FILENO, &stdout_stat) != 0)
	    error (EXIT_FAILURE, errno, _("cannot fstat %s"),
		   quote (output_file));

	  /* Complain only when ftruncate fails on a regular file, a
	     directory, or a shared memory object, as the 2000-08
	     POSIX draft specifies ftruncate's behavior only for these
	     file types.  For example, do not complain when Linux 2.4
	     ftruncate fails on /dev/fd0.  */
	  if (ftruncate (STDOUT_FILENO, o) != 0
	      && (S_ISREG (stdout_stat.st_mode)
		  || S_ISDIR (stdout_stat.st_mode)
		  || S_TYPEISSHM (&stdout_stat)))
	    {
	      char buf[INT_BUFSIZE_BOUND (off_t)];
	      error (EXIT_FAILURE, errno,
		     _("advancing past %s bytes in output file %s"),
		     offtostr (o, buf), quote (output_file));
	    }
	}
#endif
    }
  else
    {
      output_file = _("standard output");
    }

  install_handler (SIGINT, interrupt_handler);
  install_handler (SIGQUIT, interrupt_handler);
  install_handler (SIGPIPE, interrupt_handler);
  install_handler (SIGINFO, siginfo_handler);

  exit_status = dd_copy ();

  quit (exit_status);
}
