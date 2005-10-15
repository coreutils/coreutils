/* df - summarize free disk space
   Copyright (C) 91, 1995-2005 Free Software Foundation, Inc.

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
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

/* Written by David MacKenzie <djm@gnu.ai.mit.edu>.
   --human-readable and --megabyte options added by lm@sgi.com.
   --si and large file support added by eggert@twinsun.com.  */

#include <config.h>
#include <stdio.h>
#include <sys/types.h>
#include <getopt.h>
#include <assert.h>

#include "system.h"
#include "canonicalize.h"
#include "dirname.h"
#include "error.h"
#include "fsusage.h"
#include "human.h"
#include "inttostr.h"
#include "mountlist.h"
#include "quote.h"
#include "save-cwd.h"
#include "xgetcwd.h"

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "df"

#define AUTHORS \
  "Torbjorn Granlund", "David MacKenzie", "Paul Eggert"

/* Name this program was run with. */
char *program_name;

/* If true, show inode information. */
static bool inode_format;

/* If true, show even file systems with zero size or
   uninteresting types. */
static bool show_all_fs;

/* If true, show only local file systems.  */
static bool show_local_fs;

/* If true, output data for each file system corresponding to a
   command line argument -- even if it's a dummy (automounter) entry.  */
static bool show_listed_fs;

/* Human-readable options for output.  */
static int human_output_opts;

/* The units to use when printing sizes.  */
static uintmax_t output_block_size;

/* If true, use the POSIX output format.  */
static bool posix_format;

/* If true, invoke the `sync' system call before getting any usage data.
   Using this option can make df very slow, especially with many or very
   busy disks.  Note that this may make a difference on some systems --
   SunOS 4.1.3, for one.  It is *not* necessary on Linux.  */
static bool require_sync;

/* Desired exit status.  */
static int exit_status;

/* A file system type to display. */

struct fs_type_list
{
  char *fs_name;
  struct fs_type_list *fs_next;
};

/* Linked list of file system types to display.
   If `fs_select_list' is NULL, list all types.
   This table is generated dynamically from command-line options,
   rather than hardcoding into the program what it thinks are the
   valid file system types; let the user specify any file system type
   they want to, and if there are any file systems of that type, they
   will be shown.

   Some file system types:
   4.2 4.3 ufs nfs swap ignore io vm efs dbg */

static struct fs_type_list *fs_select_list;

/* Linked list of file system types to omit.
   If the list is empty, don't exclude any types.  */

static struct fs_type_list *fs_exclude_list;

/* Linked list of mounted file systems. */
static struct mount_entry *mount_list;

/* If true, print file system type as well.  */
static bool print_type;

/* For long options that have no equivalent short option, use a
   non-character as a pseudo short option, starting with CHAR_MAX + 1.  */
enum
{
  NO_SYNC_OPTION = CHAR_MAX + 1,
  /* FIXME: --kilobytes is deprecated (but not -k); remove in late 2006 */
  KILOBYTES_LONG_OPTION,
  SYNC_OPTION
};

static struct option const long_options[] =
{
  {"all", no_argument, NULL, 'a'},
  {"block-size", required_argument, NULL, 'B'},
  {"inodes", no_argument, NULL, 'i'},
  {"human-readable", no_argument, NULL, 'h'},
  {"si", no_argument, NULL, 'H'},
  {"kilobytes", no_argument, NULL, KILOBYTES_LONG_OPTION},
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
  char buf[MAX (LONGEST_HUMAN_READABLE + 1, INT_BUFSIZE_BOUND (uintmax_t))];

  if (print_type)
    fputs (_("Filesystem    Type"), stdout);
  else
    fputs (_("Filesystem        "), stdout);

  if (inode_format)
    printf (_("    Inodes   IUsed   IFree IUse%%"));
  else if (human_output_opts & human_autoscale)
    {
      if (human_output_opts & human_base_1024)
	printf (_("    Size  Used Avail Use%%"));
      else
	printf (_("     Size   Used  Avail Use%%"));
    }
  else if (posix_format)
    printf (_(" %4s-blocks      Used Available Capacity"),
	    umaxtostr (output_block_size, buf));
  else
    {
      int opts = (human_suppress_point_zero
		  | human_autoscale | human_SI
		  | (human_output_opts
		     & (human_group_digits | human_base_1024 | human_B)));

      /* Prefer the base that makes the human-readable value more exact,
	 if there is a difference.  */

      uintmax_t q1000 = output_block_size;
      uintmax_t q1024 = output_block_size;
      bool divisible_by_1000;
      bool divisible_by_1024;

      do
	{
	  divisible_by_1000 = q1000 % 1000 == 0;  q1000 /= 1000;
	  divisible_by_1024 = q1024 % 1024 == 0;  q1024 /= 1024;
	}
      while (divisible_by_1000 & divisible_by_1024);

      if (divisible_by_1000 < divisible_by_1024)
	opts |= human_base_1024;
      if (divisible_by_1024 < divisible_by_1000)
	opts &= ~human_base_1024;
      if (! (opts & human_base_1024))
	opts |= human_B;

      printf (_(" %4s-blocks      Used Available Use%%"),
	      human_readable (output_block_size, buf, opts, 1, 1));
    }

  printf (_(" Mounted on\n"));
}

/* Is FSTYPE a type of file system that should be listed?  */

static bool
selected_fstype (const char *fstype)
{
  const struct fs_type_list *fsp;

  if (fs_select_list == NULL || fstype == NULL)
    return true;
  for (fsp = fs_select_list; fsp; fsp = fsp->fs_next)
    if (STREQ (fstype, fsp->fs_name))
      return true;
  return false;
}

/* Is FSTYPE a type of file system that should be omitted?  */

static bool
excluded_fstype (const char *fstype)
{
  const struct fs_type_list *fsp;

  if (fs_exclude_list == NULL || fstype == NULL)
    return false;
  for (fsp = fs_exclude_list; fsp; fsp = fsp->fs_next)
    if (STREQ (fstype, fsp->fs_name))
      return true;
  return false;
}

/* Like human_readable (N, BUF, human_output_opts, INPUT_UNITS, OUTPUT_UNITS),
   except:

    - Return "-" if N is -1,
    - If NEGATIVE is 1 then N represents a negative number,
      expressed in two's complement.  */

static char const *
df_readable (bool negative, uintmax_t n, char *buf,
	     uintmax_t input_units, uintmax_t output_units)
{
  if (n == UINTMAX_MAX)
    return "-";
  else
    {
      char *p = human_readable (negative ? -n : n, buf + negative,
				human_output_opts, input_units, output_units);
      if (negative)
	*--p = '-';
      return p;
    }
}

/* Display a space listing for the disk device with absolute file name DISK.
   If MOUNT_POINT is non-NULL, it is the name of the root of the
   file system on DISK.
   If STAT_FILE is non-null, it is the name of a file within the file
   system that the user originally asked for; this provides better
   diagnostics, and sometimes it provides better results on networked
   file systems that give different free-space results depending on
   where in the file system you probe.
   If FSTYPE is non-NULL, it is the type of the file system on DISK.
   If MOUNT_POINT is non-NULL, then DISK may be NULL -- certain systems may
   not be able to produce statistics in this case.
   ME_DUMMY and ME_REMOTE are the mount entry flags.  */

static void
show_dev (char const *disk, char const *mount_point,
	  char const *stat_file, char const *fstype,
	  bool me_dummy, bool me_remote)
{
  struct fs_usage fsu;
  char buf[3][LONGEST_HUMAN_READABLE + 2];
  int width;
  int use_width;
  uintmax_t input_units;
  uintmax_t output_units;
  uintmax_t total;
  uintmax_t available;
  bool negate_available;
  uintmax_t available_to_root;
  uintmax_t used;
  bool negate_used;
  double pct = -1;

  if (me_remote & show_local_fs)
    return;

  if (me_dummy & !show_all_fs & !show_listed_fs)
    return;

  if (!selected_fstype (fstype) || excluded_fstype (fstype))
    return;

  /* If MOUNT_POINT is NULL, then the file system is not mounted, and this
     program reports on the file system that the special file is on.
     It would be better to report on the unmounted file system,
     but statfs doesn't do that on most systems.  */
  if (!stat_file)
    stat_file = mount_point ? mount_point : disk;

  if (get_fs_usage (stat_file, disk, &fsu))
    {
      error (0, errno, "%s", quote (stat_file));
      exit_status = EXIT_FAILURE;
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
      size_t disk_name_len = strlen (disk);
      size_t fstype_len = strlen (fstype);
      if (disk_name_len + fstype_len < 18)
	printf ("%s%*s  ", disk, 18 - (int) disk_name_len, fstype);
      else if (!posix_format)
	printf ("%s\n%18s  ", disk, fstype);
      else
	printf ("%s %s", disk, fstype);
    }
  else
    {
      if (strlen (disk) > 20 && !posix_format)
	printf ("%s\n%20s", disk, "");
      else
	printf ("%-20s", disk);
    }

  if (inode_format)
    {
      width = 7;
      use_width = 5;
      input_units = output_units = 1;
      total = fsu.fsu_files;
      available = fsu.fsu_ffree;
      negate_available = false;
      available_to_root = available;
    }
  else
    {
      width = (human_output_opts & human_autoscale
	       ? 5 + ! (human_output_opts & human_base_1024)
	       : 9);
      use_width = ((posix_format
		    && ! (human_output_opts & human_autoscale))
		   ? 8 : 4);
      input_units = fsu.fsu_blocksize;
      output_units = output_block_size;
      total = fsu.fsu_blocks;
      available = fsu.fsu_bavail;
      negate_available = fsu.fsu_bavail_top_bit_set;
      available_to_root = fsu.fsu_bfree;
    }

  used = -1;
  negate_used = false;
  if (total != UINTMAX_MAX && available_to_root != UINTMAX_MAX)
    {
      used = total - available_to_root;
      if (total < available_to_root)
	{
	  negate_used = true;
	  used = - used;
	}
    }

  printf (" %*s %*s %*s ",
	  width, df_readable (false, total,
			      buf[0], input_units, output_units),
	  width, df_readable (negate_used, used,
			      buf[1], input_units, output_units),
	  width, df_readable (negate_available, available,
			      buf[2], input_units, output_units));

  if (used == UINTMAX_MAX || available == UINTMAX_MAX)
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
	  long int lipct = pct = u * 100 / nonroot_total;
	  double ipct = lipct;

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

/* Return the root mountpoint of the file system on which FILE exists, in
   malloced storage.  FILE_STAT should be the result of stating FILE.
   Give a diagnostic and return NULL if unable to determine the mount point.
   Exit if unable to restore current working directory.  */
static char *
find_mount_point (const char *file, const struct stat *file_stat)
{
  struct saved_cwd cwd;
  struct stat last_stat;
  char *mp = NULL;		/* The malloced mount point.  */

  if (save_cwd (&cwd) != 0)
    {
      error (0, errno, _("cannot get current directory"));
      return NULL;
    }

  if (S_ISDIR (file_stat->st_mode))
    /* FILE is a directory, so just chdir there directly.  */
    {
      last_stat = *file_stat;
      if (chdir (file) < 0)
	{
	  error (0, errno, _("cannot change to directory %s"), quote (file));
	  return NULL;
	}
    }
  else
    /* FILE is some other kind of file; use its directory.  */
    {
      char *xdir = dir_name (file);
      char *dir;
      ASSIGN_STRDUPA (dir, xdir);
      free (xdir);

      if (chdir (dir) < 0)
	{
	  error (0, errno, _("cannot change to directory %s"), quote (dir));
	  return NULL;
	}

      if (stat (".", &last_stat) < 0)
	{
	  error (0, errno, _("cannot stat current directory (now %s)"),
		 quote (dir));
	  goto done;
	}
    }

  /* Now walk up FILE's parents until we find another file system or /,
     chdiring as we go.  LAST_STAT holds stat information for the last place
     we visited.  */
  for (;;)
    {
      struct stat st;
      if (stat ("..", &st) < 0)
	{
	  error (0, errno, _("cannot stat %s"), quote (".."));
	  goto done;
	}
      if (st.st_dev != last_stat.st_dev || st.st_ino == last_stat.st_ino)
	/* cwd is the mount point.  */
	break;
      if (chdir ("..") < 0)
	{
	  error (0, errno, _("cannot change to directory %s"), quote (".."));
	  goto done;
	}
      last_stat = st;
    }

  /* Finally reached a mount point, see what it's called.  */
  mp = xgetcwd ();

done:
  /* Restore the original cwd.  */
  {
    int save_errno = errno;
    if (restore_cwd (&cwd) != 0)
      error (EXIT_FAILURE, errno,
	     _("failed to return to initial working directory"));
    free_cwd (&cwd);
    errno = save_errno;
  }

  return mp;
}

/* If DISK corresponds to a mount point, show its usage
   and return true.  Otherwise, return false.  */
static bool
show_disk (char const *disk)
{
  struct mount_entry const *me;
  struct mount_entry const *best_match = NULL;

  for (me = mount_list; me; me = me->me_next)
    if (STREQ (disk, me->me_devname))
      best_match = me;

  if (best_match)
    {
      show_dev (best_match->me_devname, best_match->me_mountdir, NULL,
		best_match->me_type, best_match->me_dummy,
		best_match->me_remote);
      return true;
    }

  return false;
}

/* Figure out which device file or directory POINT is mounted on
   and show its disk usage.
   STATP must be the result of `stat (POINT, STATP)'.  */
static void
show_point (const char *point, const struct stat *statp)
{
  struct stat disk_stats;
  struct mount_entry *me;
  struct mount_entry const *best_match = NULL;

  /* If POINT is an absolute file name, see if we can find the
     mount point without performing any extra stat calls at all.  */
  if (*point == '/')
    {
      /* Find the best match: prefer non-dummies, and then prefer the
	 last match if there are ties.  */

      for (me = mount_list; me; me = me->me_next)
	if (STREQ (me->me_mountdir, point) && !STREQ (me->me_type, "lofs")
	    && (!best_match || best_match->me_dummy || !me->me_dummy))
	  best_match = me;
    }

  /* Calculate the real absolute file name for POINT, and use that to find
     the mount point.  This avoids statting unavailable mount points,
     which can hang df.  */
  if (! best_match)
    {
      char *resolved = canonicalize_file_name (point);

      if (resolved && resolved[0] == '/')
	{
	  size_t resolved_len = strlen (resolved);
	  size_t best_match_len = 0;

	  for (me = mount_list; me; me = me->me_next)
	    if (!STREQ (me->me_type, "lofs")
		&& (!best_match || best_match->me_dummy || !me->me_dummy))
	      {
		size_t len = strlen (me->me_mountdir);
		if (best_match_len <= len && len <= resolved_len
		    && (len == 1 /* root file system */
			|| ((len == resolved_len || resolved[len] == '/')
			    && strncmp (me->me_mountdir, resolved, len) == 0)))
		  {
		    best_match = me;
		    best_match_len = len;
		  }
	      }
	}

      free (resolved);

      if (best_match
	  && (stat (best_match->me_mountdir, &disk_stats) != 0
	      || disk_stats.st_dev != statp->st_dev))
	best_match = NULL;
    }

  if (! best_match)
    for (me = mount_list; me; me = me->me_next)
      {
	if (me->me_dev == (dev_t) -1)
	  {
	    if (stat (me->me_mountdir, &disk_stats) == 0)
	      me->me_dev = disk_stats.st_dev;
	    else
	      {
		error (0, errno, "%s", quote (me->me_mountdir));
		exit_status = EXIT_FAILURE;
		/* So we won't try and fail repeatedly. */
		me->me_dev = (dev_t) -2;
	      }
	  }

	if (statp->st_dev == me->me_dev
	    && !STREQ (me->me_type, "lofs")
	    && (!best_match || best_match->me_dummy || !me->me_dummy))
	  {
	    /* Skip bogus mtab entries.  */
	    if (stat (me->me_mountdir, &disk_stats) != 0
		|| disk_stats.st_dev != me->me_dev)
	      me->me_dev = (dev_t) -2;
	    else
	      best_match = me;
	  }
      }

  if (best_match)
    show_dev (best_match->me_devname, best_match->me_mountdir, point,
	      best_match->me_type, best_match->me_dummy, best_match->me_remote);
  else
    {
      /* We couldn't find the mount entry corresponding to POINT.  Go ahead and
	 print as much info as we can; methods that require the device to be
	 present will fail at a later point.  */

      /* Find the actual mount point.  */
      char *mp = find_mount_point (point, statp);
      if (mp)
	{
	  show_dev (NULL, mp, NULL, NULL, false, false);
	  free (mp);
	}
    }
}

/* Determine what kind of node NAME is and show the disk usage
   for it.  STATP is the results of `stat' on NAME.  */

static void
show_entry (char const *name, struct stat const *statp)
{
  if ((S_ISBLK (statp->st_mode) || S_ISCHR (statp->st_mode))
      && show_disk (name))
    return;

  show_point (name, statp);
}

/* Show all mounted file systems, except perhaps those that are of
   an unselected type or are empty. */

static void
show_all_entries (void)
{
  struct mount_entry *me;

  for (me = mount_list; me; me = me->me_next)
    show_dev (me->me_devname, me->me_mountdir, NULL, me->me_type,
	      me->me_dummy, me->me_remote);
}

/* Add FSTYPE to the list of file system types to display. */

static void
add_fs_type (const char *fstype)
{
  struct fs_type_list *fsp;

  fsp = xmalloc (sizeof *fsp);
  fsp->fs_name = (char *) fstype;
  fsp->fs_next = fs_select_list;
  fs_select_list = fsp;
}

/* Add FSTYPE to the list of file system types to be omitted. */

static void
add_excluded_fs_type (const char *fstype)
{
  struct fs_type_list *fsp;

  fsp = xmalloc (sizeof *fsp);
  fsp->fs_name = (char *) fstype;
  fsp->fs_next = fs_exclude_list;
  fs_exclude_list = fsp;
}

void
usage (int status)
{
  if (status != EXIT_SUCCESS)
    fprintf (stderr, _("Try `%s --help' for more information.\n"),
	     program_name);
  else
    {
      printf (_("Usage: %s [OPTION]... [FILE]...\n"), program_name);
      fputs (_("\
Show information about the file system on which each FILE resides,\n\
or all file systems by default.\n\
\n\
"), stdout);
      fputs (_("\
Mandatory arguments to long options are mandatory for short options too.\n\
"), stdout);
      fputs (_("\
  -a, --all             include file systems having 0 blocks\n\
  -B, --block-size=SIZE use SIZE-byte blocks\n\
  -h, --human-readable  print sizes in human readable format (e.g., 1K 234M 2G)\n\
  -H, --si              likewise, but use powers of 1000 not 1024\n\
"), stdout);
      fputs (_("\
  -i, --inodes          list inode information instead of block usage\n\
  -k                    like --block-size=1K\n\
  -l, --local           limit listing to local file systems\n\
      --no-sync         do not invoke sync before getting usage info (default)\n\
"), stdout);
      fputs (_("\
  -P, --portability     use the POSIX output format\n\
      --sync            invoke sync before getting usage info\n\
  -t, --type=TYPE       limit listing to file systems of type TYPE\n\
  -T, --print-type      print file system type\n\
  -x, --exclude-type=TYPE   limit listing to file systems not of type TYPE\n\
  -v                    (ignored)\n\
"), stdout);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      fputs (_("\n\
SIZE may be (or may be an integer optionally followed by) one of following:\n\
kB 1000, K 1024, MB 1000*1000, M 1024*1024, and so on for G, T, P, E, Z, Y.\n\
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

  initialize_main (&argc, &argv);
  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  fs_select_list = NULL;
  fs_exclude_list = NULL;
  inode_format = false;
  show_all_fs = false;
  show_listed_fs = false;

  human_output_opts = human_options (getenv ("DF_BLOCK_SIZE"), false,
				     &output_block_size);

  print_type = false;
  posix_format = false;
  exit_status = EXIT_SUCCESS;

  while ((c = getopt_long (argc, argv, "aB:iF:hHklmPTt:vx:", long_options, NULL))
	 != -1)
    {
      switch (c)
	{
	case 'a':
	  show_all_fs = true;
	  break;
	case 'B':
	  human_output_opts = human_options (optarg, true, &output_block_size);
	  break;
	case 'i':
	  inode_format = true;
	  break;
	case 'h':
	  human_output_opts = human_autoscale | human_SI | human_base_1024;
	  output_block_size = 1;
	  break;
	case 'H':
	  human_output_opts = human_autoscale | human_SI;
	  output_block_size = 1;
	  break;
	case KILOBYTES_LONG_OPTION:
	  error (0, 0,
		 _("the --kilobytes option is deprecated; use -k instead"));
	  /* fall through */
	case 'k':
	  human_output_opts = 0;
	  output_block_size = 1024;
	  break;
	case 'l':
	  show_local_fs = true;
	  break;
	case 'm': /* obsolescent */
	  human_output_opts = 0;
	  output_block_size = 1024 * 1024;
	  break;
	case 'T':
	  print_type = true;
	  break;
	case 'P':
	  posix_format = true;
	  break;
	case SYNC_OPTION:
	  require_sync = true;
	  break;
	case NO_SYNC_OPTION:
	  require_sync = false;
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
    bool match = false;
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
		match = true;
		break;
	      }
	  }
      }
    if (match)
      exit (EXIT_FAILURE);
  }

  if (optind < argc)
    {
      int i;

      /* stat all the given entries to make sure they get automounted,
	 if necessary, before reading the file system table.  */
      stats = xnmalloc (argc - optind, sizeof *stats);
      for (i = optind; i < argc; ++i)
	{
	  if (stat (argv[i], &stats[i - optind]))
	    {
	      error (0, errno, "%s", quote (argv[i]));
	      exit_status = EXIT_FAILURE;
	      argv[i] = NULL;
	    }
	  else
	    {
	      ++n_valid_args;
	    }
	}
    }

  mount_list =
    read_file_system_list ((fs_select_list != NULL
			    || fs_exclude_list != NULL
			    || print_type
			    || show_local_fs));

  if (mount_list == NULL)
    {
      /* Couldn't read the table of mounted file systems.
	 Fail if df was invoked with no file name arguments;
	 Otherwise, merely give a warning and proceed.  */
      const char *warning = (optind < argc ? _("Warning: ") : "");
      int status = (optind < argc ? 0 : EXIT_FAILURE);
      error (status, errno,
	     _("%scannot read table of mounted file systems"), warning);
    }

  if (require_sync)
    sync ();

  if (optind < argc)
    {
      int i;

      /* Display explicitly requested empty file systems. */
      show_listed_fs = true;

      if (n_valid_args > 0)
	print_header ();

      for (i = optind; i < argc; ++i)
	if (argv[i])
	  show_entry (argv[i], &stats[i - optind]);
    }
  else
    {
      print_header ();
      show_all_entries ();
    }

  exit (exit_status);
}
