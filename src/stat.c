#include <config.h>
#include <stdio.h>
#include <sys/stat.h>
#ifdef HAVE_SYS_SYSMACROS_H
# include <sys/sysmacros.h>
#endif
#include <pwd.h>
#include <grp.h>
#include <unistd.h>
#include <time.h>
#include <sys/vfs.h>

#include <getopt.h>
#include "error.h"
#include "filemode.h"
#include "gs.h"
#include "quotearg.h"
#include "system.h"
#include "xreadlink.h"

#define PROGRAM_NAME "stat"

#define AUTHORS "Michael Meskes"

static struct option const long_options[] = {
  {"link", no_argument, 0, 'l'},
  {"filesystem", no_argument, 0, 'f'},
  {"terse", no_argument, 0, 't'},
  {GETOPT_HELP_OPTION_DECL},
  {GETOPT_VERSION_OPTION_DECL},
  {NULL, 0, NULL, 0}
};

static char *program_name;

/* stat the filesystem and print what we find */
static void
do_statfs (char const *filename, int terse)
{
  struct statfs statfsbuf;
  char const *name;

  if (statfs (filename, &statfsbuf) == -1)
    {
      error (0, errno, "%s", quotearg_colon (filename));
      return;
    }

  if (terse != 0)
    {
#ifdef __USE_FILE_OFFSET64
# define STATFSFMT "%s %x %x %lu %lx %lld %lld %lld %ld %lld %lld\n"
#else
# define STATFSFMT "%s %x %x %d %x %ld %ld %ld %d %ld %ld\n"
#endif
      printf (STATFSFMT,
	      filename,
	      statfsbuf.f_fsid.__val[0],
	      statfsbuf.f_fsid.__val[1],
	      statfsbuf.f_namelen,
	      statfsbuf.f_type,
	      statfsbuf.f_blocks,
	      statfsbuf.f_bfree,
	      statfsbuf.f_bavail,
	      statfsbuf.f_bsize, statfsbuf.f_files, statfsbuf.f_ffree);

      return;
    }

  printf ("  File: \"%s\"\n", filename);
#ifdef __USE_FILE_OFFSET64
  printf ("    ID: %-8x %-8x Namelen: %-7ld Type: ",
	  statfsbuf.f_fsid.__val[0],
	  statfsbuf.f_fsid.__val[1], statfsbuf.f_namelen);
#else
  printf ("    ID: %-8x %-8x Namelen: %-7d Type: ",
	  statfsbuf.f_fsid.__val[0],
	  statfsbuf.f_fsid.__val[1], statfsbuf.f_namelen);
#endif

  name = fs_name (statfsbuf.f_type);
  if (name)
    printf ("%s\n", name);
  else
    printf ("UNKNOWN (0x%lx)\n", (long) statfsbuf.f_type);

#ifdef __USE_FILE_OFFSET64
  printf
    ("Blocks: Total: %-10lld Free: %-10lld Available: %-10lld Size:%ld \n",
     statfsbuf.f_blocks, statfsbuf.f_bfree, statfsbuf.f_bavail,
     statfsbuf.f_bsize);
  printf ("Inodes: Total: %-10lld Free: %-10lld\n", statfsbuf.f_files,
	  statfsbuf.f_ffree);
#else
  printf ("Blocks: Total: %-10ld Free: %-10ld Available: %-10ld  Size:%d\n",
	  statfsbuf.f_blocks,
	  statfsbuf.f_bfree, statfsbuf.f_bavail, statfsbuf.f_bsize);
  printf ("Inodes: Total: %-10ld Free: %-10ld\n",
	  statfsbuf.f_files, statfsbuf.f_ffree);
#endif
}

/* stat the file and print what we find */
static void
do_stat (char const *filename, int follow_links, int terse)
{
  struct stat statbuf;
  int err = (follow_links
	     ? stat (filename, &statbuf)
	     : lstat (filename, &statbuf));
  char modebuf[11];
  struct passwd *pw_ent;
  struct group *gw_ent;

  if (err == -1)
    {
      error (0, errno, "%s", quotearg_colon (filename));
      return;
    }

  if (terse != 0)
    {
      printf ("%s %u %u %x %d %d %x %d %d %x %x %d %d %d %d\n",
	      filename,
	      (unsigned int) statbuf.st_size,
	      (unsigned int) statbuf.st_blocks,
	      statbuf.st_mode,
	      statbuf.st_uid,
	      statbuf.st_gid,
	      (int) statbuf.st_dev,
	      (int) statbuf.st_ino,
	      (int) statbuf.st_nlink,
	      major (statbuf.st_rdev),
	      minor (statbuf.st_rdev),
	      (int) statbuf.st_atime,
	      (int) statbuf.st_mtime,
	      (int) statbuf.st_ctime, (int) statbuf.st_blksize);
      return;
    }

  if (S_ISLNK (statbuf.st_mode))
    {
      char *linkname = xreadlink (filename);
      if (linkname)
	printf ("  File: \"%s\" -> \"%s\"\n", filename, linkname);

      free (linkname);
      if (linkname == NULL)
	{
	  error (0, errno, _("cannot read symbolic link %s"),
		 quotearg_colon (filename));
	  return;
	}

    }
  else
    printf ("  File: \"%s\"\n", filename);

  printf ("  Size: %-10u\tBlocks: %-10u IO Block: %-6d ",
	  (unsigned int) statbuf.st_size,
	  (unsigned int) statbuf.st_blocks, (int) statbuf.st_blksize);

  switch (statbuf.st_mode & S_IFMT)
    {
    case S_IFDIR:
      printf ("Directory\n");
      break;
    case S_IFCHR:
      printf ("Character Device\n");
      break;
    case S_IFBLK:
      printf ("Block Device\n");
      break;
    case S_IFREG:
      printf ("Regular File\n");
      break;
    case S_IFLNK:
      printf ("Symbolic Link\n");
      break;
    case S_IFSOCK:
      printf ("Socket\n");
      break;
    case S_IFIFO:
      printf ("Fifo File\n");
      break;
    default:
      printf ("Unknown\n");
    }

  printf ("Device: %xh/%dd\tInode: %-10d  Links: %-5d",
	  (int) statbuf.st_dev,
	  (int) statbuf.st_dev, (int) statbuf.st_ino, (int) statbuf.st_nlink);

  if (S_ISBLK (statbuf.st_mode) || S_ISCHR (statbuf.st_mode))
    printf (" Device type: %x,%x\n",
	    major (statbuf.st_rdev), minor (statbuf.st_rdev));
  else
    printf ("\n");

  mode_string (statbuf.st_mode, modebuf);
  modebuf[10] = 0;
  printf ("Access: (%04o/%10.10s)", statbuf.st_mode & 07777, modebuf);

  setpwent ();
  setgrent ();
  pw_ent = getpwuid (statbuf.st_uid);
  gw_ent = getgrgid (statbuf.st_gid);
  printf ("  Uid: (%5d/%8s)   Gid: (%5d/%8s)\n",
	  statbuf.st_uid,
	  (pw_ent != 0L) ? pw_ent->pw_name : "UNKNOWN",
	  statbuf.st_gid, (gw_ent != 0L) ? gw_ent->gr_name : "UNKNOWN");

  printf ("Access: %s", ctime (&statbuf.st_atime));
  printf ("Modify: %s", ctime (&statbuf.st_mtime));
  printf ("Change: %s\n", ctime (&statbuf.st_ctime));
}

void
usage (int status)
{
  if (status != 0)
    fprintf (stderr, _("Try %s --help' for more information.\n"),
	     program_name);
  else
    {
      printf (_("Usage: %s [OPTION] FILE...\n"), program_name);
      fputs (_("\
Display file or filesystem status.\n\
\n\
  -l, --link		follow links\n\
  -f, --filesystem	display filesystem status instead of file status\n\
  -t, --terse		print the information in terse form\n\
"), stdout);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      puts (_("\nReport bugs to <bug-fileutils@gnu.org>."));
    }
  exit (status);
}

int
main (int argc, char **argv)
{
  int c;
  int i;
  int follow_links = 0;
  int fs = 0;
  int terse = 0;

  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);
  while ((c = getopt_long (argc, argv, "lft", long_options, NULL)) != -1)
    {
      switch (c)
	{
	case 'l':
	  follow_links = 1;
	  break;
	case 'f':
	  fs = 1;
	  break;
	case 't':
	  terse = 1;
	  break;
	  case_GETOPT_HELP_CHAR;

	  case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);

	default:
	  usage (EXIT_FAILURE);
	}
    }

  if (argc == optind)
    {
      error (0, 0, _("too few arguments"));
      usage (EXIT_FAILURE);
    }

  for (i = optind; i < argc; i++)
    {
      if (fs == 0)
	do_stat (argv[i], follow_links, terse);
      else
	do_statfs (argv[i], terse);
    }

  exit (EXIT_SUCCESS);
}
