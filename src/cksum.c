/* cksum -- calculate and print POSIX checksums and sizes of files
   Copyright (C) 92, 1995-2003 Free Software Foundation, Inc.

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

/* Written by Q. Frank Xia, qx@math.columbia.edu.
   Cosmetic changes and reorganization by David MacKenzie, djm@gnu.ai.mit.edu.

  Usage: cksum [file...]

  The code segment between "#ifdef CRCTAB" and "#else" is the code
  which calculates the "crctab". It is included for those who want
  verify the correctness of the "crctab". To recreate the "crctab",
  do following:

      cc -DCRCTAB -o crctab cksum.c
      crctab > crctab.h

  As Bruce Evans pointed out to me, the crctab in the sample C code
  in 4.9.10 Rationale of P1003.2/D11.2 is represented in reversed order.
  Namely, 0x01 is represented as 0x80, 0x02 is represented as 0x40, etc.
  The generating polynomial is crctab[0x80]=0xedb88320 instead of
  crctab[1]=0x04C11DB7.  But the code works only for a non-reverse order
  crctab.  Therefore, the sample implementation is wrong.

  This software is compatible with neither the System V nor the BSD
  `sum' program.  It is supposed to conform to P1003.2/D11.2,
  except foreign language interface (4.9.5.3 of P1003.2/D11.2) support.
  Any inconsistency with the standard except 4.9.5.3 is a bug.  */

#include <config.h>

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "cksum"

#define AUTHORS "Q. Frank Xia"

#include <stdio.h>
#include <sys/types.h>
#include "system.h"

#if !defined UINT_FAST32_MAX && !defined uint_fast32_t
# define uint_fast32_t unsigned int
#endif

#ifdef CRCTAB

# define BIT(x)	((uint_fast32_t) 1 << (x))
# define SBIT	BIT (31)

/* The generating polynomial is

          32   26   23   22   16   12   11   10   8   7   5   4   2   1
    G(X)=X  + X  + X  + X  + X  + X  + X  + X  + X + X + X + X + X + X + 1

  The i bit in GEN is set if X^i is a summand of G(X) except X^32.  */

# define GEN	(BIT (26) | BIT (23) | BIT (22) | BIT (16) | BIT (12) \
		 | BIT (11) | BIT (10) | BIT (8) | BIT (7) | BIT (5) \
		 | BIT (4) | BIT (2) | BIT (1) | BIT (0))

static uint_fast32_t r[8];

static void
fill_r (void)
{
  int i;

  r[0] = GEN;
  for (i = 1; i < 8; i++)
    r[i] = (r[i - 1] << 1) ^ ((r[i - 1] & SBIT) ? GEN : 0);
}

static uint_fast32_t
remainder (int m)
{
  uint_fast32_t rem = 0;
  int i;

  for (i = 0; i < 8; i++)
    if (BIT (i) & m)
      rem ^= r[i];

  return rem & 0xFFFFFFFF;	/* Make it run on 64-bit machine.  */
}

int
main (void)
{
  int i;

  fill_r ();
  printf ("static uint_fast32_t crctab[256] =\n{\n  0x0");
  for (i = 0; i < 51; i++)
    {
      printf (",\n  0x%08X, 0x%08X, 0x%08X, 0x%08X, 0x%08X",
	      remainder (i * 5 + 1), remainder (i * 5 + 2),
	      remainder (i * 5 + 3), remainder (i * 5 + 4),
	      remainder (i * 5 + 5));
    }
  printf ("\n};\n");
  exit (EXIT_SUCCESS);
}

#else /* !CRCTAB */

# include <getopt.h>
# include "closeout.h"
# include "long-options.h"
# include "error.h"
# include "inttostr.h"

/* Number of bytes to read at once.  */
# define BUFLEN (1 << 16)

/* The name this program was run with.  */
char *program_name;

static struct option const long_options[] =
{
  {0, 0, 0, 0}
};

static uint_fast32_t crctab[256] =
{
  0x0,
  0x04C11DB7, 0x09823B6E, 0x0D4326D9, 0x130476DC, 0x17C56B6B,
  0x1A864DB2, 0x1E475005, 0x2608EDB8, 0x22C9F00F, 0x2F8AD6D6,
  0x2B4BCB61, 0x350C9B64, 0x31CD86D3, 0x3C8EA00A, 0x384FBDBD,
  0x4C11DB70, 0x48D0C6C7, 0x4593E01E, 0x4152FDA9, 0x5F15ADAC,
  0x5BD4B01B, 0x569796C2, 0x52568B75, 0x6A1936C8, 0x6ED82B7F,
  0x639B0DA6, 0x675A1011, 0x791D4014, 0x7DDC5DA3, 0x709F7B7A,
  0x745E66CD, 0x9823B6E0, 0x9CE2AB57, 0x91A18D8E, 0x95609039,
  0x8B27C03C, 0x8FE6DD8B, 0x82A5FB52, 0x8664E6E5, 0xBE2B5B58,
  0xBAEA46EF, 0xB7A96036, 0xB3687D81, 0xAD2F2D84, 0xA9EE3033,
  0xA4AD16EA, 0xA06C0B5D, 0xD4326D90, 0xD0F37027, 0xDDB056FE,
  0xD9714B49, 0xC7361B4C, 0xC3F706FB, 0xCEB42022, 0xCA753D95,
  0xF23A8028, 0xF6FB9D9F, 0xFBB8BB46, 0xFF79A6F1, 0xE13EF6F4,
  0xE5FFEB43, 0xE8BCCD9A, 0xEC7DD02D, 0x34867077, 0x30476DC0,
  0x3D044B19, 0x39C556AE, 0x278206AB, 0x23431B1C, 0x2E003DC5,
  0x2AC12072, 0x128E9DCF, 0x164F8078, 0x1B0CA6A1, 0x1FCDBB16,
  0x018AEB13, 0x054BF6A4, 0x0808D07D, 0x0CC9CDCA, 0x7897AB07,
  0x7C56B6B0, 0x71159069, 0x75D48DDE, 0x6B93DDDB, 0x6F52C06C,
  0x6211E6B5, 0x66D0FB02, 0x5E9F46BF, 0x5A5E5B08, 0x571D7DD1,
  0x53DC6066, 0x4D9B3063, 0x495A2DD4, 0x44190B0D, 0x40D816BA,
  0xACA5C697, 0xA864DB20, 0xA527FDF9, 0xA1E6E04E, 0xBFA1B04B,
  0xBB60ADFC, 0xB6238B25, 0xB2E29692, 0x8AAD2B2F, 0x8E6C3698,
  0x832F1041, 0x87EE0DF6, 0x99A95DF3, 0x9D684044, 0x902B669D,
  0x94EA7B2A, 0xE0B41DE7, 0xE4750050, 0xE9362689, 0xEDF73B3E,
  0xF3B06B3B, 0xF771768C, 0xFA325055, 0xFEF34DE2, 0xC6BCF05F,
  0xC27DEDE8, 0xCF3ECB31, 0xCBFFD686, 0xD5B88683, 0xD1799B34,
  0xDC3ABDED, 0xD8FBA05A, 0x690CE0EE, 0x6DCDFD59, 0x608EDB80,
  0x644FC637, 0x7A089632, 0x7EC98B85, 0x738AAD5C, 0x774BB0EB,
  0x4F040D56, 0x4BC510E1, 0x46863638, 0x42472B8F, 0x5C007B8A,
  0x58C1663D, 0x558240E4, 0x51435D53, 0x251D3B9E, 0x21DC2629,
  0x2C9F00F0, 0x285E1D47, 0x36194D42, 0x32D850F5, 0x3F9B762C,
  0x3B5A6B9B, 0x0315D626, 0x07D4CB91, 0x0A97ED48, 0x0E56F0FF,
  0x1011A0FA, 0x14D0BD4D, 0x19939B94, 0x1D528623, 0xF12F560E,
  0xF5EE4BB9, 0xF8AD6D60, 0xFC6C70D7, 0xE22B20D2, 0xE6EA3D65,
  0xEBA91BBC, 0xEF68060B, 0xD727BBB6, 0xD3E6A601, 0xDEA580D8,
  0xDA649D6F, 0xC423CD6A, 0xC0E2D0DD, 0xCDA1F604, 0xC960EBB3,
  0xBD3E8D7E, 0xB9FF90C9, 0xB4BCB610, 0xB07DABA7, 0xAE3AFBA2,
  0xAAFBE615, 0xA7B8C0CC, 0xA379DD7B, 0x9B3660C6, 0x9FF77D71,
  0x92B45BA8, 0x9675461F, 0x8832161A, 0x8CF30BAD, 0x81B02D74,
  0x857130C3, 0x5D8A9099, 0x594B8D2E, 0x5408ABF7, 0x50C9B640,
  0x4E8EE645, 0x4A4FFBF2, 0x470CDD2B, 0x43CDC09C, 0x7B827D21,
  0x7F436096, 0x7200464F, 0x76C15BF8, 0x68860BFD, 0x6C47164A,
  0x61043093, 0x65C52D24, 0x119B4BE9, 0x155A565E, 0x18197087,
  0x1CD86D30, 0x029F3D35, 0x065E2082, 0x0B1D065B, 0x0FDC1BEC,
  0x3793A651, 0x3352BBE6, 0x3E119D3F, 0x3AD08088, 0x2497D08D,
  0x2056CD3A, 0x2D15EBE3, 0x29D4F654, 0xC5A92679, 0xC1683BCE,
  0xCC2B1D17, 0xC8EA00A0, 0xD6AD50A5, 0xD26C4D12, 0xDF2F6BCB,
  0xDBEE767C, 0xE3A1CBC1, 0xE760D676, 0xEA23F0AF, 0xEEE2ED18,
  0xF0A5BD1D, 0xF464A0AA, 0xF9278673, 0xFDE69BC4, 0x89B8FD09,
  0x8D79E0BE, 0x803AC667, 0x84FBDBD0, 0x9ABC8BD5, 0x9E7D9662,
  0x933EB0BB, 0x97FFAD0C, 0xAFB010B1, 0xAB710D06, 0xA6322BDF,
  0xA2F33668, 0xBCB4666D, 0xB8757BDA, 0xB5365D03, 0xB1F740B4
};

/* Nonzero if any of the files read were the standard input. */
static int have_read_stdin;

/* Calculate and print the checksum and length in bytes
   of file FILE, or of the standard input if FILE is "-".
   If PRINT_NAME is nonzero, print FILE next to the checksum and size.
   Return 0 if successful, -1 if an error occurs. */

static int
cksum (const char *file, int print_name)
{
  unsigned char buf[BUFLEN];
  uint_fast32_t crc = 0;
  uintmax_t length = 0;
  size_t bytes_read;
  register FILE *fp;
  char length_buf[INT_BUFSIZE_BOUND (uintmax_t)];
  char const *hp;

  if (STREQ (file, "-"))
    {
      fp = stdin;
      have_read_stdin = 1;
    }
  else
    {
      fp = fopen (file, "r");
      if (fp == NULL)
	{
	  error (0, errno, "%s", file);
	  return -1;
	}
    }

  /* Read input in BINARY mode, unless it is a console device.  */
  SET_BINARY (fileno (fp));

  while ((bytes_read = fread (buf, 1, BUFLEN, fp)) > 0)
    {
      unsigned char *cp = buf;

      if (length + bytes_read < length)
	error (EXIT_FAILURE, 0, _("%s: file too long"), file);
      length += bytes_read;
      while (bytes_read--)
	crc = (crc << 8) ^ crctab[((crc >> 24) ^ *cp++) & 0xFF];
      if (feof (fp))
	break;
    }

  if (ferror (fp))
    {
      error (0, errno, "%s", file);
      if (!STREQ (file, "-"))
	fclose (fp);
      return -1;
    }

  if (!STREQ (file, "-") && fclose (fp) == EOF)
    {
      error (0, errno, "%s", file);
      return -1;
    }

  hp = umaxtostr (length, length_buf);

  for (; length; length >>= 8)
    crc = (crc << 8) ^ crctab[((crc >> 24) ^ length) & 0xFF];

  crc = ~crc & 0xFFFFFFFF;

  if (print_name)
    printf ("%u %s %s\n", (unsigned) crc, hp, file);
  else
    printf ("%u %s\n", (unsigned) crc, hp);

  if (ferror (stdout))
    error (EXIT_FAILURE, errno, "-: %s", _("write error"));

  return 0;
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
Usage: %s [FILE]...\n\
  or:  %s [OPTION]\n\
"),
	      program_name, program_name);
      fputs (_("\
Print CRC checksum and byte counts of each FILE.\n\
\n\
"), stdout);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      printf (_("\nReport bugs to <%s>.\n"), PACKAGE_BUGREPORT);
    }
  exit (status == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}

int
main (int argc, char **argv)
{
  int i, c;
  int errors = 0;

  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  parse_long_options (argc, argv, PROGRAM_NAME, PACKAGE, VERSION,
		      AUTHORS, usage);

  have_read_stdin = 0;

  while ((c = getopt_long (argc, argv, "", long_options, NULL)) != -1)
    {
      switch (c)
	{
	case 0:
	  break;

	default:
	  usage (EXIT_FAILURE);
	}
    }

  if (optind == argc)
    errors |= cksum ("-", 0);
  else
    {
      for (i = optind; i < argc; i++)
	errors |= cksum (argv[i], 1);
    }

  if (have_read_stdin && fclose (stdin) == EOF)
    error (EXIT_FAILURE, errno, "-");
  exit (errors == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}

#endif /* !CRCTAB */
