/* df - summarize free disk space
   Copyright (C) 1991 Free Software Foundation, Inc.

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

/* Usage: df [-aikP] [-t fstype] [-x fstype] [--all] [--inodes]
   [--type fstype] [--exclude-type fstype] [--kilobytes] [--portability]
   [path...]

   Options:
   -a, --all		List all filesystems, even zero-size ones.
   -i, --inodes		List inode usage information instead of block usage.
   -k, --kilobytes	Print sizes in 1K blocks instead of 512-byte blocks.
   -P, --portability	Use the POSIX output format (one line per filesystem).
   -t, --type fstype	Limit the listing to filesystems of type `fstype'.
   -x, --exclude-type=fstype
			Limit the listing to filesystems not of type `fstype'.
			Multiple -t and/or -x options can be given.
			By default, all filesystem types are listed.

   Written by David MacKenzie <djm@gnu.ai.mit.edu> */

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
#include <getopt.h>
#include "mountlist.h"
#include "fsusage.h"
#include "system.h"
#include "version.h"

char *xmalloc ();
char *xstrdup ();
void error ();

static int selected_fstype ();
static int excluded_fstype ();
static void add_excluded_fs_type ();
static void add_fs_type ();
static void print_header ();
static void show_entry ();
static void show_all_entries ();
static void show_dev ();
static void show_disk ();
static void show_point ();
static void usage ();

/* Name this program was run with. */
char *program_name;

/* If nonzero, show inode information. */
static int inode_format;

/* If nonzero, show even filesystems with zero size or
   uninteresting types. */
static int show_all_fs;

/* If nonzero, output data for each filesystem corresponding to a
   command line argument -- even if it's a dummy (automounter) entry.  */
static int show_listed_fs;

/* If nonzero, use 1K blocks instead of 512-byte blocks. */
static int kilobyte_blocks;

/* If nonzero, use the POSIX output format.  */
static int posix_format;

/* Nonzero if errors have occurred. */
static int exit_status;

/* A filesystem type to display. */

struct fs_type_list
{
  char *fs_name;
  struct fs_type_list *fs_next;
};

/* Linked list of filesystem types to display.
   If `fs_select_list' is NULL, list all types.
   This table is generated dynamically from command-line options,
   rather than hardcoding into the program what it thinks are the
   valid filesystem types; let the user specify any filesystem type
   they want to, and if there are any filesystems of that type, they
   will be shown.

   Some filesystem types:
   4.2 4.3 ufs nfs swap ignore io vm efs dbg */

static struct fs_type_list *fs_select_list;

/* Linked list of filesystem types to omit.
   If the list is empty, don't exclude any types.  */

static struct fs_type_list *fs_exclude_list;

/* Linked list of mounted filesystems. */
static struct mount_entry *mount_list;

/* If non-zero, display usage information and exit.  */
static int show_help;

/* If non-zero, print the version on standard output and exit.  */
static int show_version;

static struct option const long_options[] =
{
  {"all", no_argument, &show_all_fs, 1},
  {"inodes", no_argument, &inode_format, 1},
  {"kilobytes", no_argument, &kilobyte_blocks, 1},
  {"portability", no_argument, &posix_format, 1},
  {"type", required_argument, 0, 't'},
  {"exclude-type", required_argument, 0, 'x'},
  {"help", no_argument, &show_help, 1},
  {"version", no_argument, &show_version, 1},
  {NULL, 0, NULL, 0}
};

void
main (argc, argv)
     int argc;
     char **argv;
{
  int i;
  struct stat *stats;

  program_name = argv[0];
  fs_select_list = NULL;
  fs_exclude_list = NULL;
  inode_format = 0;
  show_all_fs = 0;
  show_listed_fs = 0;
  kilobyte_blocks = getenv ("POSIXLY_CORRECT") == 0;
  posix_format = 0;
  exit_status = 0;

  while ((i = getopt_long (argc, argv, "aikPt:vx:", long_options, (int *) 0))
	 != EOF)
    {
      switch (i)
	{
	case 0:			/* Long option. */
	  break;
	case 'a':
	  show_all_fs = 1;
	  break;
	case 'i':
	  inode_format = 1;
	  break;
	case 'k':
	  kilobyte_blocks = 1;
	  break;
	case 'P':
	  posix_format = 1;
	  break;
	case 't':
	  add_fs_type (optarg);
	  break;
	case 'v':		/* For SysV compatibility. */
	  break;
	case 'x':
	  add_excluded_fs_type (optarg);
	  break;
	default:
	  usage (1);
	}
    }

  if (show_version)
    {
      printf ("%s\n", version_string);
      exit (0);
    }

  if (show_help)
    usage (0);

  if (optind != argc)
    {
      /* stat all the given entries to make sure they get automounted,
	 if necessary, before reading the filesystem table.  */
      stats = (struct stat *)
	xmalloc ((argc - optind) * sizeof (struct stat));
      for (i = optind; i < argc; ++i)
	if (stat (argv[i], &stats[i - optind]))
	  {
	    error (0, errno, "%s", argv[i]);
	    exit_status = 1;
	    argv[i] = NULL;
	  }
    }

  mount_list =
    read_filesystem_list ((fs_select_list != NULL || fs_exclude_list != NULL),
			  show_all_fs);

  if (mount_list == NULL)
    error (1, errno, "cannot read table of mounted filesystems");

  print_header ();
  sync ();

  if (optind == argc)
    show_all_entries ();
  else
    {
      /* Display explicitly requested empty filesystems. */
      show_listed_fs = 1;

      for (i = optind; i < argc; ++i)
	if (argv[i])
	  show_entry (argv[i], &stats[i - optind]);
    }

  exit (exit_status);
}

static void
print_header ()
{
  if (inode_format)
    printf ("Filesystem            Inodes   IUsed   IFree  %%IUsed");
  else
    printf ("Filesystem         %s  Used Available Capacity",
	    kilobyte_blocks ? "1024-blocks" : " 512-blocks");
  printf (" Mounted on\n");
}

/* Show all mounted filesystems, except perhaps those that are of
   an unselected type or are empty. */

static void
show_all_entries ()
{
  struct mount_entry *me;

  for (me = mount_list; me; me = me->me_next)
    show_dev (me->me_devname, me->me_mountdir, me->me_type);
}

/* Determine what kind of node PATH is and show the disk usage
   for it.  STATP is the results of `stat' on PATH.  */

static void
show_entry (path, statp)
     char *path;
     struct stat *statp;
{
  if (S_ISBLK (statp->st_mode) || S_ISCHR (statp->st_mode))
    show_disk (path);
  else
    show_point (path, statp);
}

/* Identify the directory, if any, that device
   DISK is mounted on, and show its disk usage.  */

static void
show_disk (disk)
     char *disk;
{
  struct mount_entry *me;

  for (me = mount_list; me; me = me->me_next)
    if (!strcmp (disk, me->me_devname))
      {
	show_dev (me->me_devname, me->me_mountdir, me->me_type);
	return;
      }
  /* No filesystem is mounted on DISK. */
  show_dev (disk, (char *) NULL, (char *) NULL);
}

/* Figure out which device file or directory POINT is mounted on
   and show its disk usage.
   STATP is the results of `stat' on POINT.  */

static void
show_point (point, statp)
     char *point;
     struct stat *statp;
{
  struct stat disk_stats;
  struct mount_entry *me;

  for (me = mount_list; me; me = me->me_next)
    {
      if (me->me_dev == (dev_t) -1)
	{
	  if (stat (me->me_mountdir, &disk_stats) == 0)
	    me->me_dev = disk_stats.st_dev;
	  else
	    {
	      error (0, errno, "%s", me->me_mountdir);
	      exit_status = 1;
	      me->me_dev = -2;	/* So we won't try and fail repeatedly. */
	    }
	}

      if (statp->st_dev == me->me_dev)
	{
	  show_dev (me->me_devname, me->me_mountdir, me->me_type);
	  return;
	}
    }
  error (0, 0, "cannot find mount point for %s", point);
  exit_status = 1;
}

/* Display a space listing for the disk device with absolute path DISK.
   If MOUNT_POINT is non-NULL, it is the path of the root of the
   filesystem on DISK.
   If FSTYPE is non-NULL, it is the type of the filesystem on DISK. */

static void
show_dev (disk, mount_point, fstype)
     char *disk;
     char *mount_point;
     char *fstype;
{
  struct fs_usage fsu;
  long blocks_used;
  long blocks_percent_used;
  long inodes_used;
  long inodes_percent_used;
  char *stat_file;

  if (!selected_fstype (fstype) || excluded_fstype (fstype))
    return;

  /* If MOUNT_POINT is NULL, then the filesystem is not mounted, and this
     program reports on the filesystem that the special file is on.
     It would be better to report on the unmounted filesystem,
     but statfs doesn't do that on most systems.  */
  stat_file = mount_point ? mount_point : disk;

  if (get_fs_usage (stat_file, disk, &fsu))
    {
      error (0, errno, "%s", stat_file);
      exit_status = 1;
      return;
    }

  if (kilobyte_blocks)
    {
      fsu.fsu_blocks /= 2;
      fsu.fsu_bfree /= 2;
      fsu.fsu_bavail /= 2;
    }

  if (fsu.fsu_blocks == 0)
    {
      if (!show_all_fs && !show_listed_fs)
	return;
      blocks_used = fsu.fsu_bavail = blocks_percent_used = 0;
    }
  else
    {
      blocks_used = fsu.fsu_blocks - fsu.fsu_bfree;
      blocks_percent_used = (long)
	(blocks_used * 100.0 / (blocks_used + fsu.fsu_bavail) + 0.5);
    }

  if (fsu.fsu_files == 0)
    {
      inodes_used = fsu.fsu_ffree = inodes_percent_used = 0;
    }
  else
    {
      inodes_used = fsu.fsu_files - fsu.fsu_ffree;
      inodes_percent_used = (long)
	(inodes_used * 100.0 / fsu.fsu_files + 0.5);
    }

  printf ("%-20s", disk);
  if (strlen (disk) > 20 && !posix_format)
    printf ("\n                    ");

  if (inode_format)
    printf (" %7ld %7ld %7ld %5ld%%",
	    fsu.fsu_files, inodes_used, fsu.fsu_ffree, inodes_percent_used);
  else
    printf (" %7ld %7ld  %7ld  %5ld%% ",
	    fsu.fsu_blocks, blocks_used, fsu.fsu_bavail, blocks_percent_used);

  if (mount_point)
    printf ("  %s", mount_point);
  putchar ('\n');
}

/* Add FSTYPE to the list of filesystem types to display. */

static void
add_fs_type (fstype)
     char *fstype;
{
  struct fs_type_list *fsp;

  fsp = (struct fs_type_list *) xmalloc (sizeof (struct fs_type_list));
  fsp->fs_name = fstype;
  fsp->fs_next = fs_select_list;
  fs_select_list = fsp;
}

/* Add FSTYPE to the list of filesystem types to be omitted. */

static void
add_excluded_fs_type (fstype)
     char *fstype;
{
  struct fs_type_list *fsp;

  fsp = (struct fs_type_list *) xmalloc (sizeof (struct fs_type_list));
  fsp->fs_name = fstype;
  fsp->fs_next = fs_exclude_list;
  fs_exclude_list = fsp;
}

/* If FSTYPE is a type of filesystem that should be listed,
   return nonzero, else zero. */

static int
selected_fstype (fstype)
     char *fstype;
{
  struct fs_type_list *fsp;

  if (fs_select_list == NULL || fstype == NULL)
    return 1;
  for (fsp = fs_select_list; fsp; fsp = fsp->fs_next)
    if (!strcmp (fstype, fsp->fs_name))
      return 1;
  return 0;
}


/* If FSTYPE is a type of filesystem that should be omitted,
   return nonzero, else zero. */

static int
excluded_fstype (fstype)
     char *fstype;
{
  struct fs_type_list *fsp;

  if (fs_exclude_list == NULL || fstype == NULL)
    return 0;
  for (fsp = fs_exclude_list; fsp; fsp = fsp->fs_next)
    if (!strcmp (fstype, fsp->fs_name))
      return 1;
  return 0;
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
      printf ("Usage: %s [OPTION] [PATH]...\n", program_name);
      printf ("\
\n\
  -a, --all                 include filesystems having 0 blocks\n\
  -i, --inodes              list inode information instead of block usage\n\
  -k, --kilobytes           use 1024 blocks, not 512 despite POSIXLY_CORRECT\n\
  -t, --type TYPE           limit the listing to TYPE filesystems type\n\
  -x, --exclude-type TYPE   limit the listing to not TYPE filesystems type\n\
  -v                        (ignored)\n\
  -P, --portability         use the POSIX output format\n\
      --help                display this help and exit\n\
      --version             output version information and exit\n\
\n\
If no PATHs are given, list all currently mounted filesystems.\n");
    }
  exit (status);
}
