/* `ln' program to create links between files.
   Copyright (C) 1986, 1989, 1990, 1991 Free Software Foundation, Inc.

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

/* Written by Mike Parker and David MacKenzie. */

#ifdef _AIX
 #pragma alloca
#endif
#include <stdio.h>
#include <sys/types.h>
#include <getopt.h>
#include "system.h"
#include "backupfile.h"

int link ();			/* Some systems don't declare this anywhere. */

#ifdef S_ISLNK
int symlink ();
#endif

char *basename ();
enum backup_type get_version ();
int do_link ();
int isdir ();
int yesno ();
void error ();
void usage ();

/* A pointer to the function used to make links.  This will point to either
   `link' or `symlink'. */
int (*linkfunc) ();

/* If nonzero, make symbolic links; otherwise, make hard links.  */
int symbolic_link;

/* If nonzero, ask the user before removing existing files.  */
int interactive;

/* If nonzero, remove existing files unconditionally.  */
int remove_existing_files;

/* If nonzero, list each file as it is moved. */
int verbose;

/* If nonzero, allow the superuser to make hard links to directories. */
int hard_dir_link;

/* The name by which the program was run, for error messages.  */
char *program_name;

struct option long_options[] = 
{
  {"backup", 0, NULL, 'b'},
  {"directory", 0, &hard_dir_link, 1},
  {"force", 0, NULL, 'f'},
  {"interactive", 0, NULL, 'i'},
  {"suffix", 1, NULL, 'S'},
  {"symbolic", 0, &symbolic_link, 1},
  {"verbose", 0, &verbose, 1},
  {"version-control", 1, NULL, 'V'},
  {NULL, 0, NULL, 0}
};

void
main (argc, argv)
     int argc;
     char **argv;
{
  int c;
  int errors;
  int make_backups = 0;
  char *version;

  version = getenv ("SIMPLE_BACKUP_SUFFIX");
  if (version)
    simple_backup_suffix = version;
  version = getenv ("VERSION_CONTROL");
  program_name = argv[0];
  linkfunc = link;
  symbolic_link = remove_existing_files = interactive = verbose
    = hard_dir_link = 0;
  errors = 0;

  while ((c = getopt_long (argc, argv, "bdfisvFS:V:", long_options, (int *) 0))
	 != EOF)
    {
      switch (c)
	{
	case 0:			/* Long-named option. */
 	  break;
	case 'b':
	  make_backups = 1;
	  break;
	case 'd':
	case 'F':
	  hard_dir_link = 1;
	  break;
	case 'f':
	  remove_existing_files = 1;
	  interactive = 0;
	  break;
	case 'i':
	  remove_existing_files = 0;
	  interactive = 1;
	  break;
	case 's':
#ifdef S_ISLNK
	  symbolic_link = 1;
#else
	  error (0, 0, "symbolic links not supported; making hard links");
#endif
	  break;
	case 'v':
	  verbose = 1;
	  break;
	case 'S':
	  simple_backup_suffix = optarg;
	  break;
	case 'V':
	  version = optarg;
	  break;
	default:
	  usage ();
	  break;
	}
    }
  if (optind == argc)
    usage ();

  if (make_backups)
    backup_type = get_version (version);

#ifdef S_ISLNK
  if (symbolic_link)
    linkfunc = symlink;
#endif

  if (optind == argc - 1)
    errors = do_link (argv[optind], ".");
  else if (optind == argc - 2)
    {
      errors = do_link (argv[optind], argv[optind + 1]);
    }
  else
    {
      char *to;

      to = argv[argc - 1];
      if (!isdir (to))
	error (1, 0, "when making multiple links, last argument must be a directory");
      for (; optind < argc - 1; ++optind)
	errors += do_link (argv[optind], to);
    }

  exit (errors != 0);
}

/* Make a link DEST to existing file SOURCE.
   If DEST is a directory, put the link to SOURCE in that directory.
   Return 1 if there is an error, otherwise 0.  */

int
do_link (source, dest)
     char *source;
     char *dest;
{
  struct stat dest_stats;
  char *dest_backup = NULL;

  /* isdir uses stat instead of lstat.
     On SVR4, link does not follow symlinks, so this check disallows
     making hard links to symlinks that point to directories.  Big deal.
     On other systems, link follows symlinks, so this check is right.  */
  if (!symbolic_link && !hard_dir_link && isdir (source))
    {
      error (0, 0, "%s: hard link not allowed for directory", source);
      return 1;
    }
  if (isdir (dest))
    {
      /* Target is a directory; build the full filename. */
      char *new_dest;
      char *source_base;

      source_base = basename (source);
      new_dest = (char *)
	alloca (strlen (source_base) + 1 + strlen (dest) + 1);
      sprintf (new_dest, "%s/%s", dest, source_base);
      dest = new_dest;
    }

  if (lstat (dest, &dest_stats) == 0)
    {
      if (S_ISDIR (dest_stats.st_mode))
	{
	  error (0, 0, "%s: cannot overwrite directory", dest);
	  return 1;
	}
      if (interactive)
	{
	  fprintf (stderr, "%s: replace `%s'? ", program_name, dest);
	  if (!yesno ())
	    return 0;
	}
      else if (!remove_existing_files)
	{
	  error (0, 0, "%s: File exists", dest);
	  return 1;
	}

      if (backup_type != none)
	{
	  char *tmp_backup = find_backup_file_name (dest);
	  if (tmp_backup == NULL)
	    error (1, 0, "virtual memory exhausted");
	  dest_backup = alloca (strlen (tmp_backup) + 1);
	  strcpy (dest_backup, tmp_backup);
	  free (tmp_backup);
	  if (rename (dest, dest_backup))
	    {
	      if (errno != ENOENT)
		{
		  error (0, errno, "cannot backup `%s'", dest);
		  return 1;
		}
	      else
		dest_backup = NULL;
	    }
	}
      else if (unlink (dest) && errno != ENOENT)
	{
	  error (0, errno, "cannot remove old link to `%s'", dest);
	  return 1;
	}
    }
  else if (errno != ENOENT)
    {
      error (0, errno, "%s", dest);
      return 1;
    }
       
  if (verbose)
    printf ("%s -> %s\n", source, dest);

  if ((*linkfunc) (source, dest) == 0)
    {
      return 0;
    }

  error (0, errno, "cannot %slink `%s' to `%s'",
#ifdef S_ISLNK
	     linkfunc == symlink ? "symbolic " : "",
#else
	     "",
#endif
	     source, dest);

  if (dest_backup)
    {
      if (rename (dest_backup, dest))
	error (0, errno, "cannot un-backup `%s'", dest);
    }
  return 1;
}

void
usage ()
{
  fprintf (stderr, "\
Usage: %s [options] source [dest]\n\
       %s [options] source... directory\n\
Options:\n\
       [-bdfisvF] [-S backup-suffix] [-V {numbered,existing,simple}]\n\
       [--version-control={numbered,existing,simple}] [--backup] [--directory]\n\
       [--force] [--interactive] [--symbolic] [--verbose]\n\
       [--suffix=backup-suffix]\n",
	   program_name, program_name);
  exit (1);
}
