/* echo.c, taken from Bash.
Copyright (C) 1987, 1989, 1991, 1992 Free Software Foundation, Inc.

This file is part of GNU Bash, the Bourne Again SHell.

Bash is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2, or (at your option) any later
version.

Bash is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License along
with Bash; see the file COPYING.  If not, write to the Free Software
Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.  */

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
#include "system.h"
#include "long-options.h"

/* echo [-neE] [arg ...]
Output the ARGs.  If -n is specified, the trailing newline is
suppressed.  If the -e option is given, interpretation of the
following backslash-escaped characters is turned on:
	\a	alert (bell)
	\b	backspace
	\c	suppress trailing newline
	\f	form feed
	\n	new line
	\r	carriage return
	\t	horizontal tab
	\v	vertical tab
	\\	backslash
	\num	the character whose ASCII code is NUM (octal).

You can explicitly turn off the interpretation of the above characters
on System V systems with the -E option.
*/

/* If defined, interpret backslash escapes if -e is given.  */
#define V9_ECHO

/* If defined, interpret backslash escapes unless -E is given.
   V9_ECHO must also be defined.  */
#define V9_DEFAULT

#if defined (V9_ECHO)
#  if defined (V9_DEFAULT)
#    define VALID_ECHO_OPTIONS "neE"
#  else
#    define VALID_ECHO_OPTIONS "ne"
#  endif /* !V9_DEFAULT */
#else /* !V9_ECHO */
#  define VALID_ECHO_OPTIONS "n"
#endif /* !V9_ECHO */

/* The name this program was run with. */
char *program_name;

static void
usage (status)
     int status;
{
  if (status != 0)
    fprintf (stderr, "Try `%s --help' for more information.\n",
	     program_name);
  else
    {
      printf ("Usage: %s [OPTION]... [STRING]...\n", program_name);
      printf ("\
\n\
  -n              do not output the trailing newline\n\
  -e              (unused)\n\
  -E              disable interpolation of some sequences in STRINGs\n\
      --help      display this help and exit (should be alone)\n\
      --version   output version information and exit (should be alone)\n\
\n\
Without -E, the following sequences are recognized and interpolated:\n\
\n\
  \\NNN   the character whose ASCII code is NNN (octal)\n\
  \\\\     backslash\n\
  \\a     alert (BEL)\n\
  \\b     backspace\n\
  \\c     suppress trailing newline\n\
  \\f     form feed\n\
  \\n     new line\n\
  \\r     carriage return\n\
  \\t     horizontal tab\n\
  \\v     vertical tab\n\
");
    }
  exit (status);
}

/* Print the words in LIST to standard output.  If the first word is
   `-n', then don't print a trailing newline.  We also support the
   echo syntax from Version 9 unix systems. */
void
main (argc, argv)
     int argc;
     char **argv;
{
  int display_return = 1, do_v9 = 0;

  program_name = argv[0];

  parse_long_options (argc, argv, usage);

/* System V machines already have a /bin/sh with a v9 behaviour.  We
   use the identical behaviour for these machines so that the
   existing system shell scripts won't barf. */
#if defined (V9_ECHO) && defined (V9_DEFAULT)
  do_v9 = 1;
#endif

  --argc;
  ++argv;

  while (argc > 0 && *argv[0] == '-')
    {
      register char *temp;
      register int i;

      /* If it appears that we are handling options, then make sure that
	 all of the options specified are actually valid.  Otherwise, the
	 string should just be echoed. */
      temp = argv[0] + 1;

      for (i = 0; temp[i]; i++)
	{
	  if (rindex (VALID_ECHO_OPTIONS, temp[i]) == 0)
	    goto just_echo;
	}

      if (!*temp)
	goto just_echo;

      /* All of the options in TEMP are valid options to ECHO.
	 Handle them. */
      while (*temp)
	{
	  if (*temp == 'n')
	    display_return = 0;
#if defined (V9_ECHO)
	  else if (*temp == 'e')
	    do_v9 = 1;
#if defined (V9_DEFAULT)
	  else if (*temp == 'E')
	    do_v9 = 0;
#endif /* V9_DEFAULT */
#endif /* V9_ECHO */
	  else
	    goto just_echo;

	  temp++;
	}
      argc--;
      argv++;
    }

just_echo:

  if (argc > 0)
    {
#if defined (V9_ECHO)
      if (do_v9)
	{
	  while (argc > 0)
	    {
	      register char *s = argv[0];
	      register int c;

	      while ((c = *s++))
		{
		  if (c == '\\' && *s)
		    {
		      switch (c = *s++)
			{
			case 'a': c = '\007'; break;
			case 'b': c = '\b'; break;
			case 'c': display_return = 0; continue;
			case 'f': c = '\f'; break;
			case 'n': c = '\n'; break;
			case 'r': c = '\r'; break;
			case 't': c = '\t'; break;
			case 'v': c = (int) 0x0B; break;
			case '0': case '1': case '2': case '3':
			case '4': case '5': case '6': case '7':
			  c -= '0';
			  if (*s >= '0' && *s <= '7')
			    c = c * 8 + (*s++ - '0');
			  if (*s >= '0' && *s <= '7')
			    c = c * 8 + (*s++ - '0');
			  break;
			case '\\': break;
			default:  putchar ('\\'); break;
			}
		    }
		  putchar(c);
		}
	      argc--;
	      argv++;
	      if (argc > 0)
		putchar(' ');
	    }
	}
      else
#endif /* V9_ECHO */
	{
	  while (argc > 0)
	    {
	      fputs (argv[0], stdout);
	      argc--;
	      argv++;
	      if (argc > 0)
		putchar (' ');
	    }
	}
    }
  if (display_return)
    putchar ('\n');
  exit (0);
}
