/* stty -- change and print terminal line settings
   Copyright (C) 1990, 1991 Free Software Foundation, Inc.

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

/* Usage: stty [-ag] [--all] [--save] [setting...]

   Options:
   -a, --all	Write all current settings to stdout in human-readable form.
   -g, --save	Write all current settings to stdout in stty-readable form.

   If no args are given, write to stdout the baud rate and settings that
   have been changed from their defaults.  Mode reading and changes
   are done on stdin.

   David MacKenzie <djm@gnu.ai.mit.edu> */

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
#include <termios.h>
#ifdef GWINSZ_IN_SYS_IOCTL
#include <sys/ioctl.h>
#endif
#ifdef WINSIZE_IN_PTEM
#include <sys/stream.h>
#include <sys/ptem.h>
#endif
#include <getopt.h>
#ifdef __STDC__
#include <stdarg.h>
#define VA_START(args, lastarg) va_start(args, lastarg)
#else
#include <varargs.h>
#define VA_START(args, lastarg) va_start(args)
#endif

#include "system.h"
#include "version.h"

#if defined(GWINSZ_BROKEN)	/* Such as for SCO UNIX 3.2.2. */
#undef TIOCGWINSZ
#endif

#ifndef _POSIX_VDISABLE
#define _POSIX_VDISABLE ((unsigned char) 0)
#endif

#define	Control(c) ((c) & 0x1f)
/* Canonical values for control characters. */
#ifndef CINTR
#define	CINTR Control ('c')
#endif
#ifndef CQUIT
#define	CQUIT 28
#endif
#ifndef CERASE
#define	CERASE 127
#endif
#ifndef CKILL
#define	CKILL Control ('u')
#endif
#ifndef CEOF
#define	CEOF Control ('d')
#endif
#ifndef CEOL
#define	CEOL _POSIX_VDISABLE
#endif
#ifndef CSTART
#define	CSTART Control ('q')
#endif
#ifndef CSTOP
#define	CSTOP Control ('s')
#endif
#ifndef CSUSP
#define	CSUSP Control ('z')
#endif
#if defined(VEOL2) && !defined(CEOL2)
#define	CEOL2 _POSIX_VDISABLE
#endif
#if defined(VSWTCH) && !defined(CSWTCH)
#define	CSWTCH _POSIX_VDISABLE
#endif
#if defined(VDSUSP) && !defined (CDSUSP)
#define	CDSUSP Control ('y')
#endif
#if !defined(VREPRINT) && defined(VRPRNT) /* Irix 4.0.5 */
#define VREPRINT VRPRNT
#endif
#if defined(VREPRINT) && !defined(CRPRNT)
#define	CRPRNT Control ('r')
#endif
#if defined(VWERASE) && !defined(CWERASE)
#define	CWERASE Control ('w')
#endif
#if defined(VLNEXT) && !defined(CLNEXT)
#define	CLNEXT Control ('v')
#endif
#if defined(VDISCARD) && !defined(VFLUSHO)
#define VFLUSHO VDISCARD
#endif
#if defined(VFLUSHO) && !defined(CFLUSHO)
#define CFLUSHO Control ('o')
#endif
#if defined(VSTATUS) && !defined(CSTATUS)
#define CSTATUS Control ('t')
#endif

static char *visible ();
static unsigned long baud_to_value ();
static int recover_mode ();
static int screen_columns ();
static int set_mode ();
static long integer_arg ();
static speed_t string_to_baud ();
static tcflag_t *mode_type_flag ();
static void display_all ();
static void display_changed ();
static void display_recoverable ();
static void display_settings ();
static void display_speed ();
static void display_window_size ();
static void sane_mode ();
static void set_control_char ();
static void set_speed ();
static void set_window_size ();

void error ();

/* Which speeds to set.  */
enum speed_setting
{
  input_speed, output_speed, both_speeds
};

/* What to output and how.  */
enum output_type
{
  changed, all, recoverable	/* Default, -a, -g.  */
};

/* Which member(s) of `struct termios' a mode uses.  */
enum mode_type
{
  control, input, output, local, combination
};

/* Flags for `struct mode_info'. */
#define SANE_SET 1		/* Set in `sane' mode. */
#define SANE_UNSET 2		/* Unset in `sane' mode. */
#define REV 4			/* Can be turned off by prepending `-'. */
#define OMIT 8			/* Don't display value. */

/* Each mode.  */
struct mode_info
{
  char *name;			/* Name given on command line.  */
  enum mode_type type;		/* Which structure element to change. */
  char flags;			/* Setting and display options.  */
  unsigned long bits;		/* Bits to set for this mode.  */
  unsigned long mask;		/* Other bits to turn off for this mode.  */
};

static struct mode_info mode_info[] =
{
  {"parenb", control, REV, PARENB, 0},
  {"parodd", control, REV, PARODD, 0},
  {"cs5", control, 0, CS5, CSIZE},
  {"cs6", control, 0, CS6, CSIZE},
  {"cs7", control, 0, CS7, CSIZE},
  {"cs8", control, 0, CS8, CSIZE},
  {"hupcl", control, REV, HUPCL, 0},
  {"hup", control, REV | OMIT, HUPCL, 0},
  {"cstopb", control, REV, CSTOPB, 0},
  {"cread", control, SANE_SET | REV, CREAD, 0},
  {"clocal", control, REV, CLOCAL, 0},
#ifdef CRTSCTS
  {"crtscts", control, REV, CRTSCTS, 0},
#endif

  {"ignbrk", input, SANE_UNSET | REV, IGNBRK, 0},
  {"brkint", input, SANE_SET | REV, BRKINT, 0},
  {"ignpar", input, REV, IGNPAR, 0},
  {"parmrk", input, REV, PARMRK, 0},
  {"inpck", input, REV, INPCK, 0},
  {"istrip", input, REV, ISTRIP, 0},
  {"inlcr", input, SANE_UNSET | REV, INLCR, 0},
  {"igncr", input, SANE_UNSET | REV, IGNCR, 0},
  {"icrnl", input, SANE_SET | REV, ICRNL, 0},
  {"ixon", input, REV, IXON, 0},
  {"ixoff", input, SANE_UNSET | REV, IXOFF, 0},
  {"tandem", input, REV | OMIT, IXOFF, 0},
#ifdef IUCLC
  {"iuclc", input, SANE_UNSET | REV, IUCLC, 0},
#endif
#ifdef IXANY
  {"ixany", input, SANE_UNSET | REV, IXANY, 0},
#endif
#ifdef IMAXBEL
  {"imaxbel", input, SANE_SET | REV, IMAXBEL, 0},
#endif

  {"opost", output, SANE_SET | REV, OPOST, 0},
#ifdef OLCUC
  {"olcuc", output, SANE_UNSET | REV, OLCUC, 0},
#endif
#ifdef OCRNL
  {"ocrnl", output, SANE_UNSET | REV, OCRNL, 0},
#endif
#ifdef ONLCR
  {"onlcr", output, SANE_SET | REV, ONLCR, 0},
#endif
#ifdef ONOCR
  {"onocr", output, SANE_UNSET | REV, ONOCR, 0},
#endif
#ifdef ONLRET
  {"onlret", output, SANE_UNSET | REV, ONLRET, 0},
#endif
#ifdef OFILL
  {"ofill", output, SANE_UNSET | REV, OFILL, 0},
#endif
#ifdef OFDEL
  {"ofdel", output, SANE_UNSET | REV, OFDEL, 0},
#endif
#ifdef NLDLY
  {"nl1", output, SANE_UNSET, NL1, NLDLY},
  {"nl0", output, SANE_SET, NL0, NLDLY},
#endif
#ifdef CRDLY
  {"cr3", output, SANE_UNSET, CR3, CRDLY},
  {"cr2", output, SANE_UNSET, CR2, CRDLY},
  {"cr1", output, SANE_UNSET, CR1, CRDLY},
  {"cr0", output, SANE_SET, CR0, CRDLY},
#endif
#ifdef TABDLY
  {"tab3", output, SANE_UNSET, TAB3, TABDLY},
  {"tab2", output, SANE_UNSET, TAB2, TABDLY},
  {"tab1", output, SANE_UNSET, TAB1, TABDLY},
  {"tab0", output, SANE_SET, TAB0, TABDLY},
#endif
#ifdef BSDLY
  {"bs1", output, SANE_UNSET, BS1, BSDLY},
  {"bs0", output, SANE_SET, BS0, BSDLY},
#endif
#ifdef VTDLY
  {"vt1", output, SANE_UNSET, VT1, VTDLY},
  {"vt0", output, SANE_SET, VT0, VTDLY},
#endif
#ifdef FFDLY
  {"ff1", output, SANE_UNSET, FF1, FFDLY},
  {"ff0", output, SANE_SET, FF0, FFDLY},
#endif

  {"isig", local, SANE_SET | REV, ISIG, 0},
  {"icanon", local, SANE_SET | REV, ICANON, 0},
#ifdef IEXTEN
  {"iexten", local, SANE_SET | REV, IEXTEN, 0},
#endif
  {"echo", local, SANE_SET | REV, ECHO, 0},
  {"echoe", local, SANE_SET | REV, ECHOE, 0},
  {"crterase", local, REV | OMIT, ECHOE, 0},
  {"echok", local, SANE_SET | REV, ECHOK, 0},
  {"echonl", local, SANE_UNSET | REV, ECHONL, 0},
  {"noflsh", local, SANE_UNSET | REV, NOFLSH, 0},
#ifdef XCASE
  {"xcase", local, SANE_UNSET | REV, XCASE, 0},
#endif
#ifdef TOSTOP
  {"tostop", local, SANE_UNSET | REV, TOSTOP, 0},
#endif
#ifdef ECHOPRT
  {"echoprt", local, SANE_UNSET | REV, ECHOPRT, 0},
  {"prterase", local, REV | OMIT, ECHOPRT, 0},
#endif
#ifdef ECHOCTL
  {"echoctl", local, SANE_SET | REV, ECHOCTL, 0},
  {"ctlecho", local, REV | OMIT, ECHOCTL, 0},
#endif
#ifdef ECHOKE
  {"echoke", local, SANE_SET | REV, ECHOKE, 0},
  {"crtkill", local, REV | OMIT, ECHOKE, 0},
#endif

  {"evenp", combination, REV | OMIT, 0, 0},
  {"parity", combination, REV | OMIT, 0, 0},
  {"oddp", combination, REV | OMIT, 0, 0},
  {"nl", combination, REV | OMIT, 0, 0},
  {"ek", combination, OMIT, 0, 0},
  {"sane", combination, OMIT, 0, 0},
  {"cooked", combination, REV | OMIT, 0, 0},
  {"raw", combination, REV | OMIT, 0, 0},
  {"pass8", combination, REV | OMIT, 0, 0},
  {"litout", combination, REV | OMIT, 0, 0},
  {"cbreak", combination, REV | OMIT, 0, 0},
#ifdef IXANY
  {"decctlq", combination, REV | OMIT, 0, 0},
#endif
#ifdef TABDLY
  {"tabs", combination, REV | OMIT, 0, 0},
#endif
#if defined(XCASE) && defined(IUCLC) && defined(OLCUC)
  {"lcase", combination, REV | OMIT, 0, 0},
  {"LCASE", combination, REV | OMIT, 0, 0},
#endif
  {"crt", combination, OMIT, 0, 0},
  {"dec", combination, OMIT, 0, 0},

  {NULL, control, 0, 0, 0}
};

/* Control character settings.  */
struct control_info
{
  char *name;			/* Name given on command line.  */
  unsigned char saneval;	/* Value to set for `stty sane'.  */
  int offset;			/* Offset in c_cc.  */
};

/* Control characters. */

static struct control_info control_info[] =
{
  {"intr", CINTR, VINTR},
  {"quit", CQUIT, VQUIT},
  {"erase", CERASE, VERASE},
  {"kill", CKILL, VKILL},
  {"eof", CEOF, VEOF},
  {"eol", CEOL, VEOL},
#ifdef VEOL2
  {"eol2", CEOL2, VEOL2},
#endif
#ifdef VSWTCH
  {"swtch", CSWTCH, VSWTCH},
#endif
  {"start", CSTART, VSTART},
  {"stop", CSTOP, VSTOP},
  {"susp", CSUSP, VSUSP},
#ifdef VDSUSP
  {"dsusp", CDSUSP, VDSUSP},
#endif
#ifdef VREPRINT
  {"rprnt", CRPRNT, VREPRINT},
#endif
#ifdef VWERASE
  {"werase", CWERASE, VWERASE},
#endif
#ifdef VLNEXT
  {"lnext", CLNEXT, VLNEXT},
#endif
#ifdef VFLUSHO
  {"flush", CFLUSHO, VFLUSHO},
#endif
#ifdef VSTATUS
  {"status", CSTATUS, VSTATUS},
#endif


  /* These must be last because of the display routines. */
  {"min", 1, VMIN},
  {"time", 0, VTIME},
  {NULL, 0, 0}
};

/* The width of the screen, for output wrapping. */
static int max_col;

/* Current position, to know when to wrap. */
static int current_col;

/* If non-zero, display usage information and exit.  */
static int show_help;

/* If non-zero, print the version on standard output and exit.  */
static int show_version;

static struct option longopts[] =
{
  {"all", no_argument, NULL, 'a'},
  {"help", no_argument, &show_help, 1},
  {"save", no_argument, NULL, 'g'},
  {"version", no_argument, &show_version, 1},
  {NULL, 0, NULL, 0}
};

/* The name this program was run with. */
char *program_name;

/* Print format string MESSAGE and optional args.
   Wrap to next line first if it won't fit.
   Print a space first unless MESSAGE will start a new line. */

/* VARARGS */
static void
#ifdef __STDC__
wrapf (char *message, ...)
#else
wrapf (message, va_alist)
     char *message;
     va_dcl
#endif
{
  va_list args;
  char buf[1024];		/* Plenty long for our needs. */
  int buflen;

  VA_START (args, message);
  vsprintf (buf, message, args);
  va_end (args);
  buflen = strlen (buf);
  if (current_col + (current_col > 0) + buflen >= max_col)
    {
      putchar ('\n');
      current_col = 0;
    }
  if (current_col > 0)
    {
      putchar (' ');
      current_col++;
    }
  fputs (buf, stdout);
  current_col += buflen;
}

static void
usage (status)
     int status;
{
  if (status != 0)
    fprintf (stderr, "Try `%s --help' for more information.\n",
	     program_name);
  else
    {
      printf ("Usage: %s [OPTION]... [SETTING]...\n", program_name);
      printf ("\
\n\
  -a, --all       print all current settings in human-readable form\n\
  -g, --save      print all current settings in a stty-readable form\n\
      --help      display this help and exit\n\
      --version   output version information and exit\n\
\n\
Optionnal - before SETTING indicates negation.  An * marks non-POSIX\n\
settings.  The underlying system defines which settings are available.\n\
");
      printf ("\
\n\
Special characters:\n\
* dsusp CHAR    CHAR will send a terminal stop signal once input flushed\n\
  eof CHAR      CHAR will send an end of file (terminate the input)\n\
  eol CHAR      CHAR will end the line\n\
* eol2 CHAR     alternate CHAR for ending the line\n\
  erase CHAR    CHAR will erase the last character typed\n\
  intr CHAR     CHAR will send an interrupt signal\n\
  kill CHAR     CHAR will erase the current line\n\
* lnext CHAR    CHAR will enter the next character quoted\n\
  quit CHAR     CHAR will send a quit signal\n\
* rprnt CHAR    CHAR will redraw the current line\n\
  start CHAR    CHAR will restart the output after stopping it\n\
  stop CHAR     CHAR will stop the output\n\
  susp CHAR     CHAR will send a terminal stop signal\n\
* swtch CHAR    CHAR will switch to a different shell layer\n\
* werase CHAR   CHAR will erase the last word typed\n\
");
      printf ("\
\n\
Special settings:\n\
  N             set the input and output speeds to N bauds\n\
* cols N        tell the kernel that the terminal has N columns\n\
* columns N     same as cols N\n\
  ispeed N      set the input speed to N\n\
* line N        use line discipline N\n\
  min N         with -icanon, set N characters minimum for a completed read\n\
  ospeed N      set the output speed to N\n\
* rows N        tell the kernel that the terminal has N rows\n\
* size          print the number of rows and columns according to the kernel\n\
  speed         print the terminal speed\n\
  time N        with -icanon, set read timeout of N tenths of a second\n\
");
      printf ("\
\n\
Control settings:\n\
  [-]clocal     disable modem control signals\n\
  [-]cread      allow input to be received\n\
* [-]crtscts    enable RTS/CTS handshaking\n\
  csN           set character size to N bits, N in [5..8]\n\
  [-]cstopb     use two stop bits per character (one with `-')\n\
  [-]hup        send a hangup signal when the last process closes the tty\n\
  [-]hupcl      same as [-]hup\n\
  [-]parenb     generate parity bit in output and expect parity bit in input\n\
  [-]parodd     set odd parity (even with `-')\n\
");
      printf ("\
\n\
Input settings:\n\
  [-]brkint     breaks cause an interrupt signal\n\
  [-]icrnl      translate carriage return to newline\n\
  [-]ignbrk     ignore breaks\n\
  [-]igncr      ignore carriage return\n\
  [-]ignpar     ignore parity errors\n\
* [-]imaxbel    beep and do not flush a full input buffer on a character\n\
  [-]inlcr      translate newline to carriage return\n\
  [-]inpck      enable input parity checking\n\
  [-]istrip     clear high (8th) bit of input characters\n\
* [-]iuclc      translate uppercase characters to lowercase\n\
* [-]ixany      let any character restart output, not only start character\n\
  [-]ixoff      enable sending of start/stop characters\n\
  [-]ixon       enable XON/XOFF flow control\n\
  [-]parmrk     mark parity errors (with a 255-0-character sequence)\n\
  [-]tandem     same as [-]ixoff\n\
");
      printf ("\
\n\
Output settings:\n\
* bsN           backspace delay style, N in [0..1]\n\
* crN           carriage return delay style, N in [0..3]\n\
* ffN           form feed delay style, N in [0..1]\n\
* nlN           newline delay style, N in [0..1]\n\
* [-]ocrnl      translate carriage return to newline\n\
* [-]ofdel      use delete characters for fill instead of null characters\n\
* [-]ofill      use fill (padding) characters instead of timing for delays\n\
* [-]olcuc      translate lowercase characters to uppercase\n\
* [-]onlcr      translate newline to carriage return-newline\n\
* [-]onlret     newline performs a carriage return\n\
* [-]onocr      do not print carriage returns in the first column\n\
  [-]opost      postprocess output\n\
* tabN          horizontal tab delay style, N in [0..3]\n\
* tabs          same as tab0\n\
* -tabs         same as tab3\n\
* vtN           vertical tab delay style, N in [0..1]\n\
");
      printf ("\
\n\
Local settings:\n\
  [-]crterase   echo erase characters as backspace-space-backspace\n\
* crtkill       kill all line by obeying the echoprt and echoe settings\n\
* -crtkill      kill all line by obeying the echoctl and echok settings\n\
* [-]ctlecho    echo control characters in hat notation (`^c')\n\
  [-]echo       echo input characters\n\
* [-]echoctl    same as [-]ctlecho\n\
  [-]echoe      same as [-]crterase\n\
  [-]echok      echo a newline after a kill character\n\
* [-]echoke     same as [-]crtkill\n\
  [-]echonl     echo newline even if not echoing other characters\n\
* [-]echoprt    echo erased characters backward, between `\\' and '/'\n\
  [-]icanon     enable erase, kill, werase, and rprnt special characters\n\
  [-]iexten     enable non-POSIX special characters\n\
  [-]isig       enable interrupt, quit, and suspend special characters\n\
  [-]noflsh     disable flushing after interrupt and quit special characters\n\
* [-]prterase   same as [-]echoprt\n\
* [-]tostop     stop background jobs that try to write to the terminal\n\
* [-]xcase      with icanon, escape with `\\' for uppercase characters\n\
");
      printf ("\
\n\
Combination settings:\n\
* [-]LCASE      same as [-]lcase\n\
  cbreak        same as -icanon\n\
  -cbreak       same as icanon\n\
  cooked        same as brkint ignpar istrip icrnl ixon opost isig\n\
                icanon, eof and eol characters to their default values\n\
  -cooked       same as raw\n\
  crt           same as echoe echoctl echoke\n\
  dec           same as echoe echoctl echoke -ixany intr ^c erase 0177\n\
                kill ^u\n\
* [-]decctlq    same as [-]ixany\n\
  ek            erase and kill characters to their default values\n\
  evenp         same as parenb -parodd cs7\n\
  -evenp        same as -parenb cs8\n\
* [-]lcase      same as xcase iuclc olcuc\n\
  litout        same as -parenb -istrip -opost cs8\n\
  -litout       same as parenb istrip opost cs7\n\
  nl            same as -icrnl -onlcr\n\
  -nl           same as icrnl -inlcr -igncr onlcr -ocrnl -onlret\n\
  oddp          same as parenb parodd cs7\n\
  -oddp         same as -parenb cs8\n\
  [-]parity     same as [-]evenp\n\
  pass8         same as -parenb -istrip cs8\n\
  -pass8        same as parenb istrip cs7\n\
  raw           same as -ignbrk -brkint -ignpar -parmrk -inpck -istrip\n\
                -inlcr -igncr -icrnl  -ixon  -ixoff  -iuclc  -ixany\n\
                -imaxbel -opost -isig -icanon -xcase min 1 time 0\n\
  -raw          same as cooked\n\
  sane          same as cread -ignbrk brkint -inlcr -igncr icrnl\n\
                -ixoff -iucl -ixany imaxbel opost -olcuc -ocrnl onlcr\n\
                -onocr -onlret -ofill -ofdel nl0 cr0 tab0 bs0 vt0 ff0\n\
                isig icanon iexten echo echoe echok -echonl -noflsh\n\
                -xcase -tostop -echoprt echoctl echoke, all special\n\
                characters to their default values.\n\
");
      printf ("\
\n\
Handle the tty line connected to standard input.  Without arguments,\n\
prints baud rate, line discipline, and deviations from stty sane.  In\n\
settings, CHAR is taken literally, or coded as in ^c, 0x37, 0177 or\n\
127; special values ^- or undef used to disable special characters.\n\
");
    }
  exit (status);
}

void
main (argc, argv)
     int argc;
     char **argv;
{
  struct termios mode;
  struct termios new_mode;
  enum output_type output_type = changed;
  int optc;

  program_name = argv[0];
  opterr = 0;

  while ((optc = getopt_long (argc, argv, "ag", longopts, (int *) 0)) != EOF)
    {
      switch (optc)
	{
	case 0:
	  break;

	case 'a':
	  output_type = all;
	  break;

	case 'g':
	  output_type = recoverable;
	  break;

	default:
	  goto done;
	}
    }

done:;

  if (show_version)
    {
      printf ("%s\n", version_string);
      exit (0);
    }

  if (show_help)
    usage (0);

  /* Initialize to all zeroes so there is no risk memcmp will report a
     spurious difference in uninitialized portion of the structure.  */
  bzero (&mode, sizeof (mode));
  if (tcgetattr (0, &mode))
    error (1, errno, "standard input");

  max_col = screen_columns ();
  current_col = 0;

  if (optind == argc)
    {
      if (optc == '?')
	{
	  error (0, 0, "invalid argument `%s'", argv[--optind]);
	  usage (1);
	}
      display_settings (output_type, &mode);
      exit (0);
    }

  while (optind < argc)
    {
      int match_found = 0;
      int reversed = 0;
      int i;

      if (argv[optind][0] == '-')
	{
	  ++argv[optind];
	  reversed = 1;
	}
      for (i = 0; mode_info[i].name != NULL; ++i)
	{
	  if (!strcmp (argv[optind], mode_info[i].name))
	    {
	      match_found = set_mode (&mode_info[i], reversed, &mode);
	      break;
	    }
	}
      if (match_found == 0 && reversed)
	{
	  error (0, 0, "invalid argument `%s'", --argv[optind]);
	  usage (1);
	}
      if (match_found == 0)
	{
	  for (i = 0; control_info[i].name != NULL; ++i)
	    {
	      if (!strcmp (argv[optind], control_info[i].name))
		{
		  if (optind == argc - 1)
		    {
		      error (0, 0, "missing argument to `%s'", argv[optind]);
		      usage (1);
		    }
		  match_found = 1;
		  ++optind;
		  set_control_char (&control_info[i], argv[optind], &mode);
		  break;
		}
	    }
	}
      if (match_found == 0)
	{
	  if (!strcmp (argv[optind], "ispeed"))
	    {
	      if (optind == argc - 1)
		{
		  error (0, 0, "missing argument to `%s'", argv[optind]);
		  usage (1);
		}
	      ++optind;
	      set_speed (input_speed, argv[optind], &mode);
	    }
	  else if (!strcmp (argv[optind], "ospeed"))
	    {
	      if (optind == argc - 1)
		{
		  error (0, 0, "missing argument to `%s'", argv[optind]);
		  usage (1);
		}
	      ++optind;
	      set_speed (output_speed, argv[optind], &mode);
	    }
#ifdef TIOCGWINSZ
	  else if (!strcmp (argv[optind], "rows"))
	    {
	      if (optind == argc - 1)
		{
		  error (0, 0, "missing argument to `%s'", argv[optind]);
		  usage (1);
		}
	      ++optind;
	      set_window_size ((int) integer_arg (argv[optind]), -1);
	    }
	  else if (!strcmp (argv[optind], "cols")
		   || !strcmp (argv[optind], "columns"))
	    {
	      if (optind == argc - 1)
		{
		  error (0, 0, "missing argument to `%s'", argv[optind]);
		  usage (1);
		}
	      ++optind;
	      set_window_size (-1, (int) integer_arg (argv[optind]));
	    }
	  else if (!strcmp (argv[optind], "size"))
	    display_window_size (0);
#endif
#ifdef HAVE_C_LINE
	  else if (!strcmp (argv[optind], "line"))
	    {
	      if (optind == argc - 1)
		{
		  error (0, 0, "missing argument to `%s'", argv[optind]);
		  usage (1);
		}
	      ++optind;
	      mode.c_line = integer_arg (argv[optind]);
	    }
#endif
	  else if (!strcmp (argv[optind], "speed"))
	    display_speed (&mode, 0);
	  else if (string_to_baud (argv[optind]) != (speed_t) -1)
	    set_speed (both_speeds, argv[optind], &mode);
	  else if (recover_mode (argv[optind], &mode) == 0)
	    {
	      error (0, 0, "invalid argument `%s'", argv[optind]);
	      usage (1);
	    }
	}
      optind++;
    }

  if (tcsetattr (0, TCSADRAIN, &mode))
    error (1, errno, "standard input");

  /* POSIX (according to Zlotnick's book) tcsetattr returns zero if it
     performs *any* of the requested operations.  This means it can report
     `success' when it has actually failed to perform some proper subset
     of the requested operations.  To detect this partial failure, get the
     current terminal attributes and compare them to the requested ones.  */

  /* Initialize to all zeroes so there is no risk memcmp will report a
     spurious difference in uninitialized portion of the structure.  */
  bzero (&new_mode, sizeof (new_mode));
  if (tcgetattr (0, &new_mode))
    error (1, errno, "standard input");

  /* Normally, one shouldn't use memcmp to compare structures that
     may have `holes' containing uninitialized data, but we have been
     careful to initialize the storage of these two variables to all
     zeroes.  One might think it more efficient simply to compare the
     modified fields, but that would require enumerating those fields --
     and not all systems have the same fields in this structure.  */

  if (memcmp (&mode, &new_mode, sizeof (mode)) != 0)
    error (1, 0,
	   "standard input: unable to perform all requested operations");

  exit (0);
}

/* Return 0 if not applied because not reversible; otherwise return 1. */

static int
set_mode (info, reversed, mode)
     struct mode_info *info;
     int reversed;
     struct termios *mode;
{
  tcflag_t *bitsp;

  if (reversed && (info->flags & REV) == 0)
    return 0;

  bitsp = mode_type_flag (info->type, mode);

  if (bitsp == NULL)
    {
      /* Combination mode. */
      if (!strcmp (info->name, "evenp") || !strcmp (info->name, "parity"))
	{
	  if (reversed)
	    mode->c_cflag = (mode->c_cflag & ~PARENB & ~CSIZE) | CS8;
	  else
	    mode->c_cflag = (mode->c_cflag & ~PARODD & ~CSIZE) | PARENB | CS7;
	}
      else if (!strcmp (info->name, "oddp"))
	{
	  if (reversed)
	    mode->c_cflag = (mode->c_cflag & ~PARENB & ~CSIZE) | CS8;
	  else
	    mode->c_cflag = (mode->c_cflag & ~CSIZE) | CS7 | PARODD | PARENB;
	}
      else if (!strcmp (info->name, "nl"))
	{
	  if (reversed)
	    {
	      mode->c_iflag = (mode->c_iflag | ICRNL) & ~INLCR & ~IGNCR;
	      mode->c_oflag = (mode->c_oflag
#ifdef ONLCR
		| ONLCR
#endif
			      )
#ifdef OCRNL
		  & ~OCRNL
#endif
#ifdef ONLRET
		  & ~ONLRET
#endif
		    ;
	    }
	  else
	    {
	      mode->c_iflag = mode->c_iflag & ~ICRNL;
#ifdef ONLCR
	      mode->c_oflag = mode->c_oflag & ~ONLCR;
#endif
	    }
	}
      else if (!strcmp (info->name, "ek"))
	{
	  mode->c_cc[VERASE] = CERASE;
	  mode->c_cc[VKILL] = CKILL;
	}
      else if (!strcmp (info->name, "sane"))
	sane_mode (mode);
      else if (!strcmp (info->name, "cbreak"))
	{
	  if (reversed)
	    mode->c_lflag |= ICANON;
	  else
	    mode->c_lflag &= ~ICANON;
	}
      else if (!strcmp (info->name, "pass8"))
	{
	  if (reversed)
	    {
	      mode->c_cflag = (mode->c_cflag & ~CSIZE) | CS7 | PARENB;
	      mode->c_iflag |= ISTRIP;
	    }
	  else
	    {
	      mode->c_cflag = (mode->c_cflag & ~PARENB & ~CSIZE) | CS8;
	      mode->c_iflag &= ~ISTRIP;
	    }
	}
      else if (!strcmp (info->name, "litout"))
	{
	  if (reversed)
	    {
	      mode->c_cflag = (mode->c_cflag & ~CSIZE) | CS7 | PARENB;
	      mode->c_iflag |= ISTRIP;
	      mode->c_oflag |= OPOST;
	    }
	  else
	    {
	      mode->c_cflag = (mode->c_cflag & ~PARENB & ~CSIZE) | CS8;
	      mode->c_iflag &= ~ISTRIP;
	      mode->c_oflag &= ~OPOST;
	    }
	}
      else if (!strcmp (info->name, "raw") || !strcmp (info->name, "cooked"))
	{
	  if ((info->name[0] == 'r' && reversed)
	      || (info->name[0] == 'c' && !reversed))
	    {
	      /* Cooked mode. */
	      mode->c_iflag |= BRKINT | IGNPAR | ISTRIP | ICRNL | IXON;
	      mode->c_oflag |= OPOST;
	      mode->c_lflag |= ISIG | ICANON;
#if VMIN == VEOF
	      mode->c_cc[VEOF] = CEOF;
#endif
#if VTIME == VEOL
	      mode->c_cc[VEOL] = CEOL;
#endif
	    }
	  else
	    {
	      /* Raw mode. */
	      mode->c_iflag = 0;
	      mode->c_oflag &= ~OPOST;
	      mode->c_lflag &= ~(ISIG | ICANON
#ifdef XCASE
				 | XCASE
#endif
				 );
	      mode->c_cc[VMIN] = 1;
	      mode->c_cc[VTIME] = 0;
	    }
	}
#ifdef IXANY
      else if (!strcmp (info->name, "decctlq"))
	{
	  if (reversed)
	    mode->c_iflag |= IXANY;
	  else
	    mode->c_iflag &= ~IXANY;
	}
#endif
#ifdef TABDLY
      else if (!strcmp (info->name, "tabs"))
	{
	  if (reversed)
	    mode->c_oflag = (mode->c_oflag & ~TABDLY) | TAB3;
	  else
	    mode->c_oflag = (mode->c_oflag & ~TABDLY) | TAB0;
	}
#endif
#if defined(XCASE) && defined(IUCLC) && defined(OLCUC)
      else if (!strcmp (info->name, "lcase")
	       || !strcmp (info->name, "LCASE"))
	{
	  if (reversed)
	    {
	      mode->c_lflag &= ~XCASE;
	      mode->c_iflag &= ~IUCLC;
	      mode->c_oflag &= ~OLCUC;
	    }
	  else
	    {
	      mode->c_lflag |= XCASE;
	      mode->c_iflag |= IUCLC;
	      mode->c_oflag |= OLCUC;
	    }
	}
#endif
      else if (!strcmp (info->name, "crt"))
	mode->c_lflag |= ECHOE
#ifdef ECHOCTL
	  | ECHOCTL
#endif
#ifdef ECHOKE
	    | ECHOKE
#endif
	      ;
      else if (!strcmp (info->name, "dec"))
	{
	  mode->c_cc[VINTR] = 3; /* ^C */
	  mode->c_cc[VERASE] = 127; /* DEL */
	  mode->c_cc[VKILL] = 21; /* ^U */
	  mode->c_lflag |= ECHOE
#ifdef ECHOCTL
	    | ECHOCTL
#endif
#ifdef ECHOKE
	      | ECHOKE
#endif
		;
#ifdef IXANY
	  mode->c_iflag &= ~IXANY;
#endif
	}
    }
  else if (reversed)
    *bitsp = *bitsp & ~info->mask & ~info->bits;
  else
    *bitsp = (*bitsp & ~info->mask) | info->bits;

  return 1;
}

static void
set_control_char (info, arg, mode)
     struct control_info *info;
     char *arg;
     struct termios *mode;
{
  unsigned char value;

  if (!strcmp (info->name, "min") || !strcmp (info->name, "time"))
    value = integer_arg (arg);
  else if (arg[0] == '\0' || arg[1] == '\0')
    value = arg[0];
  else if (!strcmp (arg, "^-") || !strcmp (arg, "undef"))
    value = _POSIX_VDISABLE;
  else if (arg[0] == '^' && arg[1] != '\0')	/* Ignore any trailing junk. */
    {
      if (arg[1] == '?')
	value = 127;
      else
	value = arg[1] & ~0140;	/* Non-letters get weird results. */
    }
  else
    value = integer_arg (arg);
  mode->c_cc[info->offset] = value;
}

static void
set_speed (type, arg, mode)
     enum speed_setting type;
     char *arg;
     struct termios *mode;
{
  speed_t baud;

  baud = string_to_baud (arg);
  if (type == input_speed || type == both_speeds)
    cfsetispeed (mode, baud);
  if (type == output_speed || type == both_speeds)
    cfsetospeed (mode, baud);
}

#ifdef TIOCGWINSZ
static void
set_window_size (rows, cols)
     int rows, cols;
{
  struct winsize win;

  if (ioctl (0, TIOCGWINSZ, (char *) &win))
    error (1, errno, "standard input");
  if (rows >= 0)
    win.ws_row = rows;
  if (cols >= 0)
    win.ws_col = cols;
  if (ioctl (0, TIOCSWINSZ, (char *) &win))
    error (1, errno, "standard input");
}

static void
display_window_size (fancy)
     int fancy;
{
  struct winsize win;

  if (ioctl (0, TIOCGWINSZ, (char *) &win))
    error (1, errno, "standard input");
  wrapf (fancy ? "rows %d; columns %d;" : "%d %d\n", win.ws_row, win.ws_col);
  if (!fancy)
    current_col = 0;
}
#endif

static int
screen_columns ()
{
#ifdef TIOCGWINSZ
  struct winsize win;

  if (ioctl (0, TIOCGWINSZ, (char *) &win))
    error (1, errno, "standard input");
  if (win.ws_col > 0)
    return win.ws_col;
#endif
  if (getenv ("COLUMNS"))
    return atoi (getenv ("COLUMNS"));
  return 80;
}

static tcflag_t *
mode_type_flag (type, mode)
     enum mode_type type;
     struct termios *mode;
{
  switch (type)
    {
    case control:
      return &mode->c_cflag;

    case input:
      return &mode->c_iflag;

    case output:
      return &mode->c_oflag;

    case local:
      return &mode->c_lflag;

    case combination:
      return NULL;

    default:
      abort ();
    }
}

static void
display_settings (output_type, mode)
     enum output_type output_type;
     struct termios *mode;
{
  switch (output_type)
    {
    case changed:
      display_changed (mode);
      break;

    case all:
      display_all (mode);
      break;

    case recoverable:
      display_recoverable (mode);
      break;
    }
}

static void
display_changed (mode)
     struct termios *mode;
{
  int i;
  int empty_line;
  tcflag_t *bitsp;
  unsigned long mask;
  enum mode_type prev_type = control;

  display_speed (mode, 1);
#ifdef HAVE_C_LINE
  wrapf ("line = %d;", mode->c_line);
#endif
  putchar ('\n');
  current_col = 0;

  empty_line = 1;
  for (i = 0; strcmp (control_info[i].name, "min"); ++i)
    {
      if (mode->c_cc[control_info[i].offset] == control_info[i].saneval)
	continue;
      empty_line = 0;
      wrapf ("%s = %s;", control_info[i].name,
	     visible (mode->c_cc[control_info[i].offset]));
    }
  if ((mode->c_lflag & ICANON) == 0)
    {
      wrapf ("min = %d; time = %d;\n", (int) mode->c_cc[VMIN],
	     (int) mode->c_cc[VTIME]);
    }
  else if (empty_line == 0)
    putchar ('\n');
  current_col = 0;

  empty_line = 1;
  for (i = 0; mode_info[i].name != NULL; ++i)
    {
      if (mode_info[i].flags & OMIT)
	continue;
      if (mode_info[i].type != prev_type)
	{
	  if (empty_line == 0)
	    {
	      putchar ('\n');
	      current_col = 0;
	      empty_line = 1;
	    }
	  prev_type = mode_info[i].type;
	}

      bitsp = mode_type_flag (mode_info[i].type, mode);
      mask = mode_info[i].mask ? mode_info[i].mask : mode_info[i].bits;
      if ((*bitsp & mask) == mode_info[i].bits)
	{
	  if (mode_info[i].flags & SANE_UNSET)
	    {
	      wrapf ("%s", mode_info[i].name);
	      empty_line = 0;
	    }
	}
      else if ((mode_info[i].flags & (SANE_SET | REV)) == (SANE_SET | REV))
	{
	  wrapf ("-%s", mode_info[i].name);
	  empty_line = 0;
	}
    }
  if (empty_line == 0)
    putchar ('\n');
  current_col = 0;
}

static void
display_all (mode)
     struct termios *mode;
{
  int i;
  tcflag_t *bitsp;
  unsigned long mask;
  enum mode_type prev_type = control;

  display_speed (mode, 1);
#ifdef TIOCGWINSZ
  display_window_size (1);
#endif
#ifdef HAVE_C_LINE
  wrapf ("line = %d;", mode->c_line);
#endif
  putchar ('\n');
  current_col = 0;

  for (i = 0; strcmp (control_info[i].name, "min"); ++i)
    {
      wrapf ("%s = %s;", control_info[i].name,
	     visible (mode->c_cc[control_info[i].offset]));
    }
  wrapf ("min = %d; time = %d;\n", mode->c_cc[VMIN], mode->c_cc[VTIME]);
  current_col = 0;

  for (i = 0; mode_info[i].name != NULL; ++i)
    {
      if (mode_info[i].flags & OMIT)
	continue;
      if (mode_info[i].type != prev_type)
	{
	  putchar ('\n');
	  current_col = 0;
	  prev_type = mode_info[i].type;
	}

      bitsp = mode_type_flag (mode_info[i].type, mode);
      mask = mode_info[i].mask ? mode_info[i].mask : mode_info[i].bits;
      if ((*bitsp & mask) == mode_info[i].bits)
	wrapf ("%s", mode_info[i].name);
      else if (mode_info[i].flags & REV)
	wrapf ("-%s", mode_info[i].name);
    }
  putchar ('\n');
  current_col = 0;
}

static void
display_speed (mode, fancy)
     struct termios *mode;
     int fancy;
{
  if (cfgetispeed (mode) == 0 || cfgetispeed (mode) == cfgetospeed (mode))
    wrapf (fancy ? "speed %lu baud;" : "%lu\n",
	   baud_to_value (cfgetospeed (mode)));
  else
    wrapf (fancy ? "ispeed %lu baud; ospeed %lu baud;" : "%lu %lu\n",
	   baud_to_value (cfgetispeed (mode)),
	   baud_to_value (cfgetospeed (mode)));
  if (!fancy)
    current_col = 0;
}

static void
display_recoverable (mode)
     struct termios *mode;
{
  int i;

  printf ("%lx:%lx:%lx:%lx",
	  (unsigned long) mode->c_iflag, (unsigned long) mode->c_oflag,
	  (unsigned long) mode->c_cflag, (unsigned long) mode->c_lflag);
  for (i = 0; i < NCCS; ++i)
    printf (":%x", (unsigned int) mode->c_cc[i]);
  putchar ('\n');
}

static int
recover_mode (arg, mode)
     char *arg;
     struct termios *mode;
{
  int i, n;
  unsigned int chr;
  unsigned long iflag, oflag, cflag, lflag;

  /* Scan into temporaries since it is too much trouble to figure out
     the right format for `tcflag_t'.  */
  if (sscanf (arg, "%lx:%lx:%lx:%lx%n",
	      &iflag, &oflag, &cflag, &lflag, &n) != 4)
    return 0;
  mode->c_iflag = iflag;
  mode->c_oflag = oflag;
  mode->c_cflag = cflag;
  mode->c_lflag = lflag;
  arg += n;
  for (i = 0; i < NCCS; ++i)
    {
      if (sscanf (arg, ":%x%n", &chr, &n) != 1)
	return 0;
      mode->c_cc[i] = chr;
      arg += n;
    }
  return 1;
}

struct speed_map
{
  char *string;			/* ASCII representation. */
  speed_t speed;		/* Internal form. */
  unsigned long value;		/* Numeric value. */
};

struct speed_map speeds[] =
{
  {"0", B0, 0},
  {"50", B50, 50},
  {"75", B75, 75},
  {"110", B110, 110},
  {"134", B134, 134},
  {"134.5", B134, 134},
  {"150", B150, 150},
  {"200", B200, 200},
  {"300", B300, 300},
  {"600", B600, 600},
  {"1200", B1200, 1200},
  {"1800", B1800, 1800},
  {"2400", B2400, 2400},
  {"4800", B4800, 4800},
  {"9600", B9600, 9600},
  {"19200", B19200, 19200},
  {"38400", B38400, 38400},
  {"exta", B19200, 19200},
  {"extb", B38400, 38400},
#ifdef B57600
  {"57600", B57600, 57600},
#endif
#ifdef B115200
  {"115200", B115200, 115200},
#endif
  {NULL, 0, 0}
};

static speed_t
string_to_baud (arg)
     char *arg;
{
  int i;

  for (i = 0; speeds[i].string != NULL; ++i)
    if (!strcmp (arg, speeds[i].string))
      return speeds[i].speed;
  return (speed_t) -1;
}

static unsigned long
baud_to_value (speed)
     speed_t speed;
{
  int i;

  for (i = 0; speeds[i].string != NULL; ++i)
    if (speed == speeds[i].speed)
      return speeds[i].value;
  return 0;
}

static void
sane_mode (mode)
     struct termios *mode;
{
  int i;
  tcflag_t *bitsp;

  for (i = 0; control_info[i].name; ++i)
    {
#if VMIN == VEOF
      if (!strcmp (control_info[i].name, "min"))
	break;
#endif
      mode->c_cc[control_info[i].offset] = control_info[i].saneval;
    }

  for (i = 0; mode_info[i].name != NULL; ++i)
    {
      if (mode_info[i].flags & SANE_SET)
	{
	  bitsp = mode_type_flag (mode_info[i].type, mode);
	  *bitsp = (*bitsp & ~mode_info[i].mask) | mode_info[i].bits;
	}
      else if (mode_info[i].flags & SANE_UNSET)
	{
	  bitsp = mode_type_flag (mode_info[i].type, mode);
	  *bitsp = *bitsp & ~mode_info[i].mask & ~mode_info[i].bits;
	}
    }
}

/* Return a string that is the printable representation of character CH.  */
/* Adapted from `cat' by Torbjorn Granlund.  */

static char *
visible (ch)
     unsigned char ch;
{
  static char buf[10];
  char *bpout = buf;

  if (ch == _POSIX_VDISABLE)
    return "<undef>";

  if (ch >= 32)
    {
      if (ch < 127)
	*bpout++ = ch;
      else if (ch == 127)
	{
	  *bpout++ = '^';
	  *bpout++ = '?';
	}
      else
	{
	  *bpout++ = 'M',
	    *bpout++ = '-';
	  if (ch >= 128 + 32)
	    {
	      if (ch < 128 + 127)
		*bpout++ = ch - 128;
	      else
		{
		  *bpout++ = '^';
		  *bpout++ = '?';
		}
	    }
	  else
	    {
	      *bpout++ = '^';
	      *bpout++ = ch - 128 + 64;
	    }
	}
    }
  else
    {
      *bpout++ = '^';
      *bpout++ = ch + 64;
    }
  *bpout = '\0';
  return buf;
}

/* Parse string S as an integer, using decimal radix by default,
   but allowing octal and hex numbers as in C.  */
/* From `od' by Richard Stallman.  */

static long
integer_arg (s)
     char *s;
{
  long value;
  int radix = 10;
  char *p = s;
  int c;

  if (*p != '0')
    radix = 10;
  else if (*++p == 'x')
    {
      radix = 16;
      p++;
    }
  else
    radix = 8;

  value = 0;
  while (((c = *p++) >= '0' && c <= '9')
	 || (radix == 16 && (c & ~40) >= 'A' && (c & ~40) <= 'Z'))
    {
      value *= radix;
      if (c >= '0' && c <= '9')
	value += c - '0';
      else
	value += (c & ~40) - 'A';
    }

  if (c == 'b')
    value *= 512;
  else if (c == 'B')
    value *= 1024;
  else
    p--;

  if (*p)
    {
      error (0, 0, "invalid integer argument `%s'", s);
      usage (1);
    }
  return value;
}
