/* df - summarize free disk space
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

/* Written by David MacKenzie <djm@gnu.ai.mit.edu>.
   --human-readable and --megabyte options added by lm@sgi.com.
   --si and large file support added by eggert@twinsun.com.  */

#ifdef _AIX
 #pragma alloca
#endif

#include <config.h>
#if HAVE_INTTYPES_H
# include <inttypes.h>
#endif
#include <stdio.h>
#include <sys/types.h>
#include <getopt.h>
#include <assert.h>

#include "system.h"
#include "dirname.h"
#include "error.h"
#include "fsusage.h"
#include "human.h"
#include "mountlist.h"
#include "path-concat.h"
#include "quote.h"
#include "save-cwd.h"

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "df"

#define AUTHORS \
  "Torbjorn Granlund, David MacKenzie, Larry McVoy, and Paul Eggert"

char *xgetcwd ();

/* Name this program was run with. */
char *program_name;

/* If nonzero, show inode information. */
static int inode_format;

/* If nonzero, show even filesystems with zero size or
   uninteresting types. */
static int show_all_fs;

/* If nonzero, show only local filesystems.  */
static int show_local_fs;

/* If nonzero, output data for each filesystem corresponding to a
   command line argument -- even if it's a dummy (automounter) entry.  */
static int show_listed_fs;

/* If positive, the units to use when printing sizes;
   if negative, the human-readable base.  */
static int output_block_size;

/* If nonzero, use the POSIX output format.  */
static int posix_format;

/* If nonzero, invoke the `sync' system call before getting any usage data.
   Using this option can make df very slow, especially with many or very
   busy disks.  Note that this may make a difference on some systems --
   SunOs4.1.3, for one.  It is *not* necessary on Linux.  */
static int require_sync = 0;

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

/* If nonzero, print filesystem type as well.  */
static int print_type;

/* For long options that have no equivalent short option, use a
   non-character as a pseudo short option, starting with CHAR_MAX + 1.  */
enum
{
  SYNC_OPTION = CHAR_MAX + 1,
  NO_SYNC_OPTION
};

static struct option const long_options[] =
{
  {"all", no_argument, NULL, 'a'},
  {"block-size", required_argument, NULL, 'B'},
  {"inodes", no_argument, NULL, 'i'},
  {"human-readable", no_argument, NULL, 'h'},
  {"si", no_argument, NULL, 'H'},
  {"kilobytes", no_argument, NULL, 'k'}, /* long form is obsolescent */
  {"local", no_argument, NULL, 'l'},
  {"megabytes", no_argument, NULL, 'm'}, /* obsolescent */
  {"portability", no_argument, NULL, 'P'},
  {"print-type", no_argument, NULL, 'T'},
  {"sync", no_argument, NULL, SYNC_OPTION},
  {"no-sync", no_argument, NULL, NO_SYNC_OPTION},
  {"type", required_argument, NULL, 't'},
  {"exclude-type", required_argument, NULL, 'x'},
  {GETOPT_HELP_OPTION_DECL},
  {GETOPT_VERSION_OPTION_DECL},
  {NULL, 0, NULL, 0}
};

static void
print_header (void)
{
  printf (_("Filesystem "));

  if (print_type)
    printf (_("   Type"));
  else
    printf ("       ");

  if (inode_format)
    printf (_("    Inodes   IUsed   IFree IUse%%"));
  else if (output_block_size < 0)
    {
      if (output_block_size == -1000)
	printf (_("     Size   Used  Avail Use%%"));
      else
	printf (_("    Size  Used Avail Use%%"));
    }
  else if (posix_format)
    printf (_(" %4d-blocks      Used Available Capacity"), output_block_size);
  else
    {
      char buf[LONGEST_HUMAN_READABLE + 1];
      char *p = human_readable (output_block_size, buf, 1, -1024);

      /* Replace e.g. "1.0K" by "1K".  */
      size_t plen = strlen (p);
      if (3 <= plen && strncmp (p + plen - 3, ".0", 2) == 0)
	strcpy (p + plen - 3, p + plen - 1);

      printf (_(" %4s-blocks      Used Available Use%%"), p);
    }

  printf (_(" Mounted on\n"));
}

/* If FSTYPE is a type of filesystem that should be listed,
   return nonzero, else zero. */

static int
selected_fstype (const char *fstype)
{
  const struct fs_type_list *fsp;

  if (fs_select_list == NULL || fstype == NULL)
    return 1;
  for (fsp = fs_select_list; fsp; fsp = fsp->fs_next)
    if (STREQ (fstype, fsp->fs_name))
      return 1;
  return 0;
}

/* If FSTYPE is a type of filesystem that should be omitted,
   return nonzero, else zero. */

static int
excluded_fstype (const char *fstype)
{
  const struct fs_type_list *fsp;

  if (fs_exclude_list == NULL || fstype == NULL)
    return 0;
  for (fsp = fs_exclude_list; fsp; fsp = fsp->fs_next)
    if (STREQ (fstype, fsp->fs_name))
      return 1;
  return 0;
}

/* Like human_readable_inexact with a human_ceiling
   human_inexact_style, except return "-" if the argument is -1, and
   if NEGATIVE is 1 then N represents a negative number, expressed in
   two's complement.  */

static char const *
df_readable (int negative, uintmax_t n, char *buf,
	     int from_block_size, int t_output_block_size)
{
  if (n == -1)
    return "-";
  else
    {
      char *p = human_readable_inexact (negative ? - n : n,
					buf + negative, from_block_size,
					t_output_block_size,
					human_ceiling);
      if (negative)
	*--p = '-';
      return p;
    }
}

/* Display a space listing for the disk device with absolute path DISK.
   If MOUNT_POINT is non-NULL, it is the path of the root of the
   filesystem on DISK.
   If FSTYPE is non-NULL, it is the type of the filesystem on DISK.
   If MOUNT_POINT is non-NULL, then DISK may be NULL -- certain systems may
   not be able to produce statistics in this case.
   ME_DUMMY and ME_REMOTE are the mount entry flags.  */

static void
show_dev (const char *disk, const char *mount_point, const char *fstype,
	  int me_dummy, int me_remote)
{
  struct fs_usage fsu;
  const char *stat_file;
  char buf[3][LONGEST_HUMAN_READABLE + 2];
  int width;
  int use_width;
  int input_units;
  int output_units;
  uintmax_t total;
  uintmax_t available;
  int negate_available;
  uintmax_t available_to_root;
  uintmax_t used;
  int negate_used;
  double pct = -1;

  if (me_remote && show_local_fs)
    return;

  if (me_dummy && show_all_fs == 0 && !show_listed_fs)
    return;

  if (!selected_fstype (fstype) || excluded_fstype (fstype))
    return;

  /* If MOUNT_POINT is NULL, then the filesystem is not mounted, and this
     program reports on the filesystem that the special file is on.
     It would be better to report on the unmounted filesystem,
     but statfs doesn't do that on most systems.  */
  stat_file = mount_point ? mount_point : disk;

  if (get_fs_usage (stat_file, disk, &fsu))
    {
      error (0, errno, "%s", quote (stat_file));
      exit_status = 1;
      return;
    }

  if (fsu.fsu_blocks == 0 && !show_all_fs && !show_listed_fs)
    return;

  if (! disk)
    disk = "-";			/* unknown */
  if (! fstype)
    fstype = "-";		/* unknown */

  /* df.c reserved 5 positions for fstype,
     but that does not suffice for type iso9660 */
  if (print_type)
    {
      int disk_name_len = (int) strlen (disk);
      int fstype_len = (int) strlen (fstype);
      if (disk_name_len + fstype_len + 2 < 20)
	printf ("%s%*s  ", disk, 18 - disk_name_len, fstype);
      else if (!posix_format)
	printf ("%s\n%18s  ", disk, fstype);
      else
	printf ("%s %s", disk, fstype);
    }
  else
    {
      if ((int) strlen (disk) > 20 && !posix_format)
	printf ("%s\n%20s", disk, "");
      else
	printf ("%-20s", disk);
    }

  if (inode_format)
    {
      width = 7;
      use_width = 5;
      input_units = 1;
      output_units = output_block_size < 0 ? output_block_size : 1;
      total = fsu.fsu_files;
      available = fsu.fsu_ffree;
      negate_available = 0;
      available_to_root = available;
    }
  else
    {
      width = output_block_size < 0 ? 5 + (output_block_size == -1000) : 9;
      use_width = (posix_format && 0 <= output_block_size) ? 8 : 4;
      input_units = fsu.fsu_blocksize;
      output_units = output_block_size;
      total = fsu.fsu_blocks;
      available = fsu.fsu_bavail;
      negate_available = fsu.fsu_bavail_top_bit_set;
      available_to_root = fsu.fsu_bfree;
    }

  used = -1;
  negate_used = 0;
  if (total != -1 && available_to_root != -1)
    {
      used = total - available_to_root;
      if (total < available_to_root)
	{
	  negate_used = 1;
	  used = - used;
	}
    }

  printf (" %*s %*s %*s ",
	  width, df_readable (0, total,
			      buf[0], input_units, output_units),
	  width, df_readable (negate_used, used,
			      buf[1], input_units, output_units),
	  width, df_readable (negate_available, available,
			      buf[2], input_units, output_units));

  if (used == -1 || available == -1)
    ;
  else if (!negate_used
	   && used <= TYPE_MAXIMUM (uintmax_t) / 100
	   && used + available != 0
	   && (used + available < used) == negate_available)
    {
      uintmax_t u100 = used * 100;
      uintmax_t nonroot_total = used + available;
      pct = u100 / nonroot_total + (u100 % nonroot_total != 0);
    }
  else
    {
      /* The calculation cannot be done easily with integer
	 arithmetic.  Fall back on floating point.  This can suffer
	 from minor rounding errors, but doing it exactly requires
	 multiple precision arithmetic, and it's not worth the
	 aggravation.  */
      double u = negate_used ? - (double) - used : used;
      double a = negate_available ? - (double) - available : available;
      double nonroot_total = u + a;
      if (nonroot_total)
	{
	  double ipct;
	  pct = u * 100 / nonroot_total;
	  ipct = (long) pct;

	  /* Like `pct = ceil (dpct);', but avoid ceil so that
	     the math library needn't be linked.  */
	  if (ipct - 1 < pct && pct <= ipct + 1)
	    pct = ipct + (ipct < pct);
	}
    }

  if (0 <= pct)
    printf ("%*.0f%%", use_width - 1, pct);
  else
    printf ("%*s", use_width, "- ");

  if (mount_point)
    {
#ifdef HIDE_AUTOMOUNT_PREFIX
      /* Don't print the first directory name in MOUNT_POINT if it's an
	 artifact of an automounter.  This is a bit too aggressive to be
	 the default.  */
      if (strncmp ("/auto/", mount_point, 6) == 0)
	mount_point += 5;
      else if (strncmp ("/tmp_mnt/", mount_point, 9) == 0)
	mount_point += 8;
#endif
      printf (" %s", mount_point);
    }
  putchar ('\n');
}

/* Identify the directory, if any, that device
   DISK is mounted on, and show its disk usage.  */

static void
show_disk (const char *disk)
{
  struct mount_entry *me;

  for (me = mount_list; me; me = me->me_next)
    if (STREQ (disk, me->me_devname))
      {
	show_dev (me->me_devname, me->me_mountdir, me->me_type,
		  me->me_dummy, me->me_remote);
	return;
      }
  /* No filesystem is mounted on DISK. */
  show_dev (disk, (char *) NULL, (char *) NULL, 0, 0);
}

/* Return the root mountpoint of the filesystem on which FILE exists, in
   malloced storage.  FILE_STAT should be the result of stating FILE.  */
static char *
find_mount_point (const char *file, const struct stat *file_stat)
{
  struct saved_cwd cwd;
  struct stat last_stat;
  char *mp = 0;			/* The malloced mount point path.  */

  if (save_cwd (&cwd))
    return NULL;

  if (S_ISDIR (file_stat->st_mode))
    /* FILE is a directory, so just chdir there directly.  */
    {
      last_stat = *file_stat;
      if (chdir (file) < 0)
	return NULL;
    }
  else
    /* FILE is some other kind of file, we need to use its directory.  */
    {
      char *dir = dir_name (file);
      int rv = chdir (dir);
      free (dir);

      if (rv < 0)
	return NULL;

      if (stat (".", &last_stat) < 0)
	goto done;
    }

  /* Now walk up FILE's parents until we find another filesystem or /,
     chdiring as we go.  LAST_STAT holds stat information for the last place
     we visited.  */
  for (;;)
    {
      struct stat st;
      if (stat ("..", &st) < 0)
	goto done;
      if (st.st_dev != last_stat.st_dev || st.st_ino == last_stat.st_ino)
	/* cwd is the mount point.  */
	break;
      if (chdir ("..") < 0)
	goto done;
      last_stat = st;
    }

  /* Finally reached a mount point, see what it's called.  */
  mp = xgetcwd ();

done:
  /* Restore the original cwd.  */
  {
    int save_errno = errno;
    if (restore_cwd (&cwd, 0, mp))
      exit (EXIT_FAILURE);			/* We're scrod.  */
    free_cwd (&cwd);
    errno = save_errno;
  }

  return mp;
}

/* Figure out which device file or directory POINT is mounted on
   and show its disk usage.
   STATP is the results of `stat' on POINT.  */
static void
show_point (const char *point, const struct stat *statp)
{
  struct stat disk_stats;
  struct mount_entry *me;
  struct mount_entry *matching_dummy = NULL;
  char *needs_freeing = NULL;

  /* If POINT is an absolute path name, see if we can find the
     mount point without performing any extra stat calls at all.  */
  if (*point == '/')
    {
      for (me = mount_list; me; me = me->me_next)
	{
	  if (STREQ (me->me_mountdir, point) && !STREQ (me->me_type, "lofs"))
	    {
	      /* Prefer non-dummy entries.  */
	      if (! me->me_dummy)
		goto show_me;
	      matching_dummy = me;
	    }
	}

      if (matching_dummy)
	goto show_matching_dummy;
    }

  /* Ideally, the following mess of #if'd code would be in a separate
     file, and there'd be a single function call here.  FIXME, someday.  */

#if HAVE_REALPATH || HAVE_RESOLVEPATH || HAVE_CANONICALIZE_FILE_NAME
  /* Calculate the real absolute path for POINT, and use that to find
     the mount point.  This avoids statting unavailable mount points,
     which can hang df.  */
  {
    char const *abspoint = point;
    char *resolved;
    ssize_t resolved_len;
    struct mount_entry *best_match = NULL;

# if HAVE_CANONICALIZE_FILE_NAME
    resolved = canonicalize_file_name (abspoint);
    resolved_len = resolved ? strlen (resolved) : -1;
# else
#  if HAVE_RESOLVEPATH
    /* All known hosts with resolvepath (e.g. Solaris 7) don't turn
       relative names into absolute ones, so prepend the working
       directory if the path is not absolute.  */

    if (*point != '/')
      {
	static char const *wd;

	if (! wd)
	  {
	    struct stat pwd_stats;
	    struct stat dot_stats;

	    /* Use PWD if it is correct; this is usually cheaper than
               xgetcwd.  */
	    wd = getenv ("PWD");
	    if (! (wd
		   && stat (wd, &pwd_stats) == 0
		   && stat (".", &dot_stats) == 0
		   && SAME_INODE (pwd_stats, dot_stats)))
	      wd = xgetcwd ();
	  }

	if (wd)
	  {
	    needs_freeing = path_concat (wd, point, NULL);
	    if (needs_freeing)
	      abspoint = needs_freeing;
	  }
      }
#  endif

#  if HAVE_RESOLVEPATH
    {
      size_t resolved_size = strlen (abspoint);
      while (1)
	{
	  resolved_size = 2 * resolved_size + 1;
	  resolved = xmalloc (resolved_size);
	  resolved_len = resolvepath (abspoint, resolved, resolved_size);
	  if (resolved_len < resolved_size)
	    break;
	  free (resolved);
	}
    }
#  else
    /* Use realpath only as a last resort.
       It provides a very poor interface.  */
    resolved = xmalloc (PATH_MAX + 1);
    resolved = (char *) realpath (abspoint, resolved);
    resolved_len = resolved ? strlen (resolved) : -1;
#  endif
# endif

    if (1 <= resolved_len && resolved[0] == '/')
      {
	size_t best_match_len = 0;

	for (me = mount_list; me; me = me->me_next)
	  if (! me->me_dummy)
	    {
	      size_t len = strlen (me->me_mountdir);
	      if (best_match_len < len && len <= resolved_len
		  && (len == 1 /* root file system */
		      || ((len == resolved_len || resolved[len] == '/')
			  && strncmp (me->me_mountdir, resolved, len) == 0)))
		{
		  best_match = me;
		  best_match_len = len;
		}
	    }
      }

    if (resolved)
      free (resolved);

    if (best_match && !STREQ (best_match->me_type, "lofs")
	&& stat (best_match->me_mountdir, &disk_stats) == 0
	&& disk_stats.st_dev == statp->st_dev)
      {
	me = best_match;
	goto show_me;
      }
  }
#endif

  for (me = mount_list; me; me = me->me_next)
    {
      if (me->me_dev == (dev_t) -1)
	{
	  if (stat (me->me_mountdir, &disk_stats) == 0)
	    me->me_dev = disk_stats.st_dev;
	  else
	    {
	      error (0, errno, "%s", quote (me->me_mountdir));
	      exit_status = 1;
	      /* So we won't try and fail repeatedly. */
	      me->me_dev = (dev_t) -2;
	    }
	}

      if (statp->st_dev == me->me_dev)
	{
	  /* Skip bogus mtab entries.  */
	  if (stat (me->me_mountdir, &disk_stats) != 0
	      || disk_stats.st_dev != me->me_dev)
	    {
	      me->me_dev = (dev_t) -2;
	      continue;
	    }

	  /* Prefer non-dummy entries.  */
	  if (! me->me_dummy)
	    goto show_me;
	  matching_dummy = me;
	}
    }

  if (matching_dummy)
    goto show_matching_dummy;

  /* We couldn't find the mount entry corresponding to POINT.  Go ahead and
     print as much info as we can; methods that require the device to be
     present will fail at a later point.  */
  {
    /* Find the actual mount point.  */
    char *mp = find_mount_point (point, statp);
    if (mp)
      {
	show_dev (0, mp, 0, 0, 0);
	free (mp);
      }
    else
      error (0, errno, "%s", quote (point));
  }

  goto free_then_return;

 show_matching_dummy:
  me = matching_dummy;
 show_me:
  show_dev (me->me_devname, me->me_mountdir, me->me_type, me->me_dummy,
	    me->me_remote);
 free_then_return:
  if (needs_freeing)
    free (needs_freeing);
}

/* Determine what kind of node PATH is and show the disk usage
   for it.  STATP is the results of `stat' on PATH.  */

static void
show_entry (const char *path, const struct stat *statp)
{
  if (S_ISBLK (statp->st_mode) || S_ISCHR (statp->st_mode))
    show_disk (path);
  else
    show_point (path, statp);
}

/* Show all mounted filesystems, except perhaps those that are of
   an unselected type or are empty. */

static void
show_all_entries (void)
{
  struct mount_entry *me;

  for (me = mount_list; me; me = me->me_next)
    show_dev (me->me_devname, me->me_mountdir, me->me_type,
	      me->me_dummy, me->me_remote);
}

/* Add FSTYPE to the list of filesystem types to display. */

static void
add_fs_type (const char *fstype)
{
  struct fs_type_list *fsp;

  fsp = (struct fs_type_list *) xmalloc (sizeof (struct fs_type_list));
  fsp->fs_name = (char *) fstype;
  fsp->fs_next = fs_select_list;
  fs_select_list = fsp;
}

/* Add FSTYPE to the list of filesystem types to be omitted. */

static void
add_excluded_fs_type (const char *fstype)
{
  struct fs_type_list *fsp;

  fsp = (struct fs_type_list *) xmalloc (sizeof (struct fs_type_list));
  fsp->fs_name = (char *) fstype;
  fsp->fs_next = fs_exclude_list;
  fs_exclude_list = fsp;
}

void
usage (int status)
{
  if (status != 0)
    fprintf (stderr, _("Try `%s --help' for more information.\n"),
	     program_name);
  else
    {
      printf (_("Usage: %s [OPTION]... [FILE]...\n"), program_name);
      fputs (_("\
Show information about the filesystem on which each FILE resides,\n\
or all filesystems by default.\n\
\n\
"), stdout);
      fputs (_("\
Mandatory arguments to long options are mandatory for short options too.\n\
"), stdout);
      fputs (_("\
  -a, --all             include filesystems having 0 blocks\n\
  -B, --block-size=SIZE use SIZE-byte blocks\n\
  -h, --human-readable  print sizes in human readable format (e.g., 1K 234M 2G)\n\
  -H, --si              likewise, but use powers of 1000 not 1024\n\
"), stdout);
      fputs (_("\
  -i, --inodes          list inode information instead of block usage\n\
  -k                    like --block-size=1K\n\
  -l, --local           limit listing to local filesystems\n\
      --no-sync         do not invoke sync before getting usage info (default)\n\
"), stdout);
      fputs (_("\
  -P, --portability     use the POSIX output format\n\
      --sync            invoke sync before getting usage info\n\
  -t, --type=TYPE       limit listing to filesystems of type TYPE\n\
  -T, --print-type      print filesystem type\n\
  -x, --exclude-type=TYPE   limit listing to filesystems not of type TYPE\n\
  -v                    (ignored)\n\
"), stdout);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      fputs (_("\n\
SIZE may be (or may be an integer optionally followed by) one of following:\n\
kB 1000, K 1024, MB 1,000,000, M 1,048,576, and so on for G, T, P, E, Z, Y.\n\
"), stdout);
      printf (_("\nReport bugs to <%s>.\n"), PACKAGE_BUGREPORT);
    }
  exit (status);
}

int
main (int argc, char **argv)
{
  int c;
  struct stat *stats IF_LINT (= 0);
  int n_valid_args = 0;

  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  fs_select_list = NULL;
  fs_exclude_list = NULL;
  inode_format = 0;
  show_all_fs = 0;
  show_listed_fs = 0;

  human_block_size (getenv ("DF_BLOCK_SIZE"), 0, &output_block_size);

  print_type = 0;
  posix_format = 0;
  exit_status = 0;

  while ((c = getopt_long (argc, argv, "aB:iF:hHklmPTt:vx:", long_options, NULL))
	 != -1)
    {
      switch (c)
	{
	case 0:			/* Long option. */
	  break;
	case 'a':
	  show_all_fs = 1;
	  break;
	case 'B':
	  human_block_size (optarg, 1, &output_block_size);
	  break;
	case 'i':
	  inode_format = 1;
	  break;
	case 'h':
	  output_block_size = -1024;
	  break;
	case 'H':
	  output_block_size = -1000;
	  break;
	case 'k':
	  output_block_size = 1024;
	  break;
	case 'l':
	  show_local_fs = 1;
	  break;
	case 'm': /* obsolescent */
	  output_block_size = 1024 * 1024;
	  break;
	case 'T':
	  print_type = 1;
	  break;
	case 'P':
	  posix_format = 1;
	  break;
	case SYNC_OPTION:
	  require_sync = 1;
	  break;
	case NO_SYNC_OPTION:
	  require_sync = 0;
	  break;

	case 'F':
	  /* Accept -F as a synonym for -t for compatibility with Solaris.  */
	case 't':
	  add_fs_type (optarg);
	  break;

	case 'v':		/* For SysV compatibility. */
	  /* ignore */
	  break;
	case 'x':
	  add_excluded_fs_type (optarg);
	  break;

	case_GETOPT_HELP_CHAR;
	case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);

	default:
	  usage (EXIT_FAILURE);
	}
    }

  /* Fail if the same file system type was both selected and excluded.  */
  {
    int match = 0;
    struct fs_type_list *fs_incl;
    for (fs_incl = fs_select_list; fs_incl; fs_incl = fs_incl->fs_next)
      {
	struct fs_type_list *fs_excl;
	for (fs_excl = fs_exclude_list; fs_excl; fs_excl = fs_excl->fs_next)
	  {
	    if (STREQ (fs_incl->fs_name, fs_excl->fs_name))
	      {
		error (0, 0,
		       _("file system type %s both selected and excluded"),
		       quote (fs_incl->fs_name));
		match = 1;
		break;
	      }
	  }
      }
    if (match)
      exit (EXIT_FAILURE);
  }

  {
    int i;

    /* stat all the given entries to make sure they get automounted,
       if necessary, before reading the filesystem table.  */
    stats = (struct stat *)
      xmalloc ((argc - optind) * sizeof (struct stat));
    for (i = optind; i < argc; ++i)
      {
	if (stat (argv[i], &stats[i - optind]))
	  {
	    error (0, errno, "%s", quote (argv[i]));
	    exit_status = 1;
	    argv[i] = NULL;
	  }
	else
	  {
	    ++n_valid_args;
	  }
      }
  }

  mount_list =
    read_filesystem_list ((fs_select_list != NULL
			   || fs_exclude_list != NULL
			   || print_type
			   || show_local_fs));

  if (mount_list == NULL)
    {
      /* Couldn't read the table of mounted filesystems.
	 Fail if df was invoked with no file name arguments;
	 Otherwise, merely give a warning and proceed.  */
      const char *warning = (optind == argc ? "" : _("Warning: "));
      int status = (optind == argc ? 1 : 0);
      error (status, errno,
	     _("%scannot read table of mounted filesystems"), warning);
    }

  if (require_sync)
    sync ();

  if (optind == argc)
    {
      print_header ();
      show_all_entries ();
    }
  else
    {
      int i;

      /* Display explicitly requested empty filesystems. */
      show_listed_fs = 1;

      if (n_valid_args > 0)
	print_header ();

      for (i = optind; i < argc; ++i)
	if (argv[i])
	  show_entry (argv[i], &stats[i - optind]);
    }

  exit (exit_status);
}
