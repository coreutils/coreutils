#include <config.h>
#include <stdio.h>
#include <sys/types.h>
#include "system.h"
#include "version-etc.h"
#include "closeout.h"

#define PROGRAM_NAME "true"
#define AUTHORS "no one"

/* The name this program was run with. */
char *program_name;

void
usage (int status)
{
  printf (_("\
Usage: %s [ignored command line arguments]\n\
  or:  %s OPTION\n\
Exit with a status code indicating success.\n\
\n\
These option names may not be abbreviated.\n\
\n\
"),
	  program_name, program_name);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
  printf (_("\nReport bugs to <%s>.\n"), PACKAGE_BUGREPORT);
  exit (status);
}

int
main (int argc, char **argv)
{
  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  /* Recognize --help or --version only if it's the only command-line
     argument and if POSIXLY_CORRECT is not set.  */
  if (argc == 2 && getenv ("POSIXLY_CORRECT") == NULL)
    {
      if (STREQ (argv[1], "--help"))
	usage (EXIT_SUCCESS);

      if (STREQ (argv[1], "--version"))
	version_etc (stdout, PROGRAM_NAME, GNU_PACKAGE, VERSION, AUTHORS);
    }

  exit (EXIT_SUCCESS);
}
