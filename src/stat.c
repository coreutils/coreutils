/* stat.c -- display file or filesystem status
   Copyright (C) 2001, 2002, 2003 Free Software Foundation.

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
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

   Written by Michael Meskes.  */

#include <config.h>

#include <stdio.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <unistd.h>
#include <time.h>
#if HAVE_SYS_STATVFS_H
# include <sys/statvfs.h>
#endif
#if HAVE_SYS_VFS_H
# include <sys/vfs.h>
#endif

/* NetBSD 1.5.2 needs these, for the declaration of struct statfs. */
#if !HAVE_SYS_STATVFS_H && !HAVE_SYS_VFS_H
# if HAVE_SYS_MOUNT_H && HAVE_SYS_PARAM_H
#  include <sys/param.h>
#  include <sys/mount.h>
# endif
#endif

#include "system.h"

#include "closeout.h"
#include "error.h"
#include "filemode.h"
#include "file-type.h"
#include "fs.h"
#include "getopt.h"
#include "quote.h"
#include "quotearg.h"
#include "strftime.h"
#include "xreadlink.h"

#define NAMEMAX_FORMAT PRIuMAX

#if HAVE_STRUCT_STATVFS_F_BASETYPE
# define STRUCT_STATVFS struct statvfs
# define HAVE_STRUCT_STATXFS_F_TYPE HAVE_STRUCT_STATVFS
# if HAVE_STRUCT_STATVFS_F_NAMEMAX
#  define SB_F_NAMEMAX(S) ((uintmax_t) ((S)->f_namemax))
# endif
#else
# define STRUCT_STATVFS struct statfs
# define HAVE_STRUCT_STATXFS_F_TYPE HAVE_STRUCT_STATFS
# if HAVE_STRUCT_STATFS_F_NAMELEN
#  define SB_F_NAMEMAX(S) ((uintmax_t) ((S)->f_namelen))
# endif
#endif

#ifndef SB_F_NAMEMAX
/* NetBSD 1.5.2 has neither f_namemax nor f_namelen.  */
# define SB_F_NAMEMAX(S) "*"
# undef NAMEMAX_FORMAT
# define NAMEMAX_FORMAT "s"
#endif

#if HAVE_STRUCT_STATVFS_F_BASETYPE
# define STATXFS_FILE_SYSTEM_TYPE_MEMBER_NAME f_basetype
#else
# if HAVE_STRUCT_STATFS_F_FSTYPENAME
#  define STATXFS_FILE_SYSTEM_TYPE_MEMBER_NAME f_fstypename
# endif
#endif

#define PROGRAM_NAME "stat"

#define AUTHORS "Michael Meskes"

static struct option const long_options[] = {
  {"link", no_argument, 0, 'l'}, /* deprecated.  FIXME: remove in 2003 */
  {"dereference", no_argument, 0, 'L'},
  {"format", required_argument, 0, 'c'},
  {"filesystem", no_argument, 0, 'f'},
  {"terse", no_argument, 0, 't'},
  {GETOPT_HELP_OPTION_DECL},
  {GETOPT_VERSION_OPTION_DECL},
  {NULL, 0, NULL, 0}
};

/* Nonzero means we should exit with EXIT_FAILURE upon completion.  */
static int G_fail;

char *program_name;

/* Return the type of the specified file system.
   Some systems have statfvs.f_basetype[FSTYPSZ]. (AIX, HP-UX, and Solaris)
   Others have statfs.f_fstypename[MFSNAMELEN]. (NetBSD 1.5.2)
   Still others have neither and have to get by with f_type (Linux).  */
static char *
human_fstype (STRUCT_STATVFS const *statfsbuf)
{
#ifdef STATXFS_FILE_SYSTEM_TYPE_MEMBER_NAME
  /* Cast away the `const' attribute.  */
  return (char *) statfsbuf->STATXFS_FILE_SYSTEM_TYPE_MEMBER_NAME;
#else
  char const *type;
  switch (statfsbuf->f_type)
    {
# if defined __linux__
    case S_MAGIC_AFFS:
      type = "affs";
      break;
    case S_MAGIC_EXT:
      type = "ext";
      break;
    case S_MAGIC_EXT2_OLD:
      type = "ext2";
      break;
    case S_MAGIC_EXT2:
      type = "ext2/ext3";
      break;
    case S_MAGIC_HPFS:
      type = "hpfs";
      break;
    case S_MAGIC_ISOFS:
      type = "isofs";
      break;
    case S_MAGIC_ISOFS_WIN:
      type = "isofs";
      break;
    case S_MAGIC_ISOFS_R_WIN:
      type = "isofs";
      break;
    case S_MAGIC_MINIX:
      type = "minix";
      break;
    case S_MAGIC_MINIX_30:
      type = "minix (30 char.)";
      break;
    case S_MAGIC_MINIX_V2:
      type = "minix v2";
      break;
    case S_MAGIC_MINIX_V2_30:
      type = "minix v2 (30 char.)";
      break;
    case S_MAGIC_MSDOS:
      type = "msdos";
      break;
    case S_MAGIC_FAT:
      type = "fat";
      break;
    case S_MAGIC_NCP:
      type = "novell";
      break;
    case S_MAGIC_NFS:
      type = "nfs";
      break;
    case S_MAGIC_PROC:
      type = "proc";
      break;
    case S_MAGIC_SMB:
      type = "smb";
      break;
    case S_MAGIC_XENIX:
      type = "xenix";
      break;
    case S_MAGIC_SYSV4:
      type = "sysv4";
      break;
    case S_MAGIC_SYSV2:
      type = "sysv2";
      break;
    case S_MAGIC_COH:
      type = "coh";
      break;
    case S_MAGIC_UFS:
      type = "ufs";
      break;
    case S_MAGIC_XIAFS:
      type = "xia";
      break;
    case S_MAGIC_NTFS:
      type = "ntfs";
      break;
    case S_MAGIC_TMPFS:
      type = "tmpfs";
      break;
    case S_MAGIC_REISERFS:
      type = "reiserfs";
      break;
    case S_MAGIC_CRAMFS:
      type = "cramfs";
      break;
    case S_MAGIC_ROMFS:
      type = "romfs";
      break;
# elif __GNU__
    case FSTYPE_UFS:
      type = "ufs";
      break;
    case FSTYPE_NFS:
      type = "nfs";
      break;
    case FSTYPE_GFS:
      type = "gfs";
      break;
    case FSTYPE_LFS:
      type = "lfs";
      break;
    case FSTYPE_SYSV:
      type = "sysv";
      break;
    case FSTYPE_FTP:
      type = "ftp";
      break;
    case FSTYPE_TAR:
      type = "tar";
      break;
    case FSTYPE_AR:
      type = "ar";
      break;
    case FSTYPE_CPIO:
      type = "cpio";
      break;
    case FSTYPE_MSLOSS:
      type = "msloss";
      break;
    case FSTYPE_CPM:
      type = "cpm";
      break;
    case FSTYPE_HFS:
      type = "hfs";
      break;
    case FSTYPE_DTFS:
      type = "dtfs";
      break;
    case FSTYPE_GRFS:
      type = "grfs";
      break;
    case FSTYPE_TERM:
      type = "term";
      break;
    case FSTYPE_DEV:
      type = "dev";
      break;
    case FSTYPE_PROC:
      type = "proc";
      break;
    case FSTYPE_IFSOCK:
      type = "ifsock";
      break;
    case FSTYPE_AFS:
      type = "afs";
      break;
    case FSTYPE_DFS:
      type = "dfs";
      break;
    case FSTYPE_PROC9:
      type = "proc9";
      break;
    case FSTYPE_SOCKET:
      type = "socket";
      break;
    case FSTYPE_MISC:
      type = "misc";
      break;
    case FSTYPE_EXT2FS:
      type = "ext2/ext3";
      break;
    case FSTYPE_HTTP:
      type = "http";
      break;
    case FSTYPE_MEMFS:
      type = "memfs";
      break;
    case FSTYPE_ISO9660:
      type = "iso9660";
      break;
# endif
    default:
      type = NULL;
      break;
    }

  if (type)
    return (char *) type;

  {
    static char buf[sizeof "UNKNOWN (0x%x)" - 2
		    + 2 * sizeof (statfsbuf->f_type)];
    sprintf (buf, "UNKNOWN (0x%x)", statfsbuf->f_type);
    return buf;
  }
#endif
}

static char *
human_access (struct stat const *statbuf)
{
  static char modebuf[11];
  mode_string (statbuf->st_mode, modebuf);
  modebuf[10] = 0;
  return modebuf;
}

static char *
human_time (time_t const *t)
{
  static char str[80];
  struct tm *tm = localtime (t);
  if (tm == NULL)
    {
      G_fail = 1;
      return (char *) _("*** invalid date/time ***");
    }
  nstrftime (str, sizeof str, "%Y-%m-%d %H:%M:%S.%N %z", tm, 0, 0);
  return str;
}

/* print statfs info */
static void
print_statfs (char *pformat, char m, char const *filename,
	      void const *data)
{
  STRUCT_STATVFS const *statfsbuf = data;

  switch (m)
    {
    case 'n':
      strcat (pformat, "s");
      printf (pformat, filename);
      break;

    case 'i':
#if HAVE_STRUCT_STATXFS_F_FSID___VAL
      strcat (pformat, "x %-8x");
      printf (pformat, statfsbuf->f_fsid.__val[0], /* u_long */
	      statfsbuf->f_fsid.__val[1]);
#else
      strcat (pformat, "Lx");
      printf (pformat, statfsbuf->f_fsid);
#endif
      break;

    case 'l':
      strcat (pformat, NAMEMAX_FORMAT);
      printf (pformat, SB_F_NAMEMAX (statfsbuf));
      break;
    case 't':
#if HAVE_STRUCT_STATXFS_F_TYPE
      strcat (pformat, "lx");
      printf (pformat, (long int) (statfsbuf->f_type));  /* no equiv. */
#else
      fputc ('*', stdout);
#endif
      break;
    case 'T':
      strcat (pformat, "s");
      printf (pformat, human_fstype (statfsbuf));
      break;
    case 'b':
      strcat (pformat, PRIdMAX);
      printf (pformat, (intmax_t) (statfsbuf->f_blocks));
      break;
    case 'f':
      strcat (pformat, PRIdMAX);
      printf (pformat, (intmax_t) (statfsbuf->f_bfree));
      break;
    case 'a':
      strcat (pformat, PRIdMAX);
      printf (pformat, (intmax_t) (statfsbuf->f_bavail));
      break;
    case 's':
      strcat (pformat, "ld");
      printf (pformat, (long int) (statfsbuf->f_bsize));
      break;
    case 'c':
      strcat (pformat, PRIdMAX);
      printf (pformat, (intmax_t) (statfsbuf->f_files));
      break;
    case 'd':
      strcat (pformat, PRIdMAX);
      printf (pformat, (intmax_t) (statfsbuf->f_ffree));
      break;

    default:
      strcat (pformat, "c");
      printf (pformat, m);
      break;
    }
}

/* print stat info */
static void
print_stat (char *pformat, char m, char const *filename, void const *data)
{
  struct stat *statbuf = (struct stat *) data;
  struct passwd *pw_ent;
  struct group *gw_ent;

  switch (m)
    {
    case 'n':
      strcat (pformat, "s");
      printf (pformat, filename);
      break;
    case 'N':
      strcat (pformat, "s");
      if (S_ISLNK (statbuf->st_mode))
	{
	  char *linkname = xreadlink (filename);
	  if (linkname == NULL)
	    {
	      error (0, errno, _("cannot read symbolic link %s"),
		     quote (filename));
	      return;
	    }
	  /*printf("\"%s\" -> \"%s\"", filename, linkname); */
	  printf (pformat, quote (filename));
	  printf (" -> ");
	  printf (pformat, quote (linkname));
	}
      else
	{
	  printf (pformat, quote (filename));
	}
      break;
    case 'd':
      strcat (pformat, "d");
      printf (pformat, (int) statbuf->st_dev);
      break;
    case 'D':
      strcat (pformat, "x");
      printf (pformat, (int) statbuf->st_dev);
      break;
    case 'i':
      strcat (pformat, "d");
      printf (pformat, (int) statbuf->st_ino);
      break;
    case 'a':
      strcat (pformat, "o");
      printf (pformat, statbuf->st_mode & 07777);
      break;
    case 'A':
      strcat (pformat, "s");
      printf (pformat, human_access (statbuf));
      break;
    case 'f':
      strcat (pformat, "x");
      printf (pformat, statbuf->st_mode);
      break;
    case 'F':
      strcat (pformat, "s");
      printf (pformat, file_type (statbuf));
      break;
    case 'h':
      strcat (pformat, "d");
      printf (pformat, (int) statbuf->st_nlink);
      break;
    case 'u':
      strcat (pformat, "d");
      printf (pformat, statbuf->st_uid);
      break;
    case 'U':
      strcat (pformat, "s");
      setpwent ();
      pw_ent = getpwuid (statbuf->st_uid);
      printf (pformat, (pw_ent != 0L) ? pw_ent->pw_name : "UNKNOWN");
      break;
    case 'g':
      strcat (pformat, "d");
      printf (pformat, statbuf->st_gid);
      break;
    case 'G':
      strcat (pformat, "s");
      setgrent ();
      gw_ent = getgrgid (statbuf->st_gid);
      printf (pformat, (gw_ent != 0L) ? gw_ent->gr_name : "UNKNOWN");
      break;
    case 't':
      strcat (pformat, "x");
      printf (pformat, major (statbuf->st_rdev));
      break;
    case 'T':
      strcat (pformat, "x");
      printf (pformat, minor (statbuf->st_rdev));
      break;
    case 's':
      strcat (pformat, PRIuMAX);
      printf (pformat, (uintmax_t) (statbuf->st_size));
      break;
    case 'B':
      strcat (pformat, "u");
      printf (pformat, (unsigned int) ST_NBLOCKSIZE);
      break;
    case 'b':
      strcat (pformat, "u");
      printf (pformat, (unsigned int) ST_NBLOCKS (*statbuf));
      break;
    case 'o':
      strcat (pformat, "d");
      printf (pformat, (int) statbuf->st_blksize);
      break;
    case 'x':
      strcat (pformat, "s");
      printf (pformat, human_time (&(statbuf->st_atime)));
      break;
    case 'X':
      strcat (pformat, "d");
      printf (pformat, (int) statbuf->st_atime);
      break;
    case 'y':
      strcat (pformat, "s");
      printf (pformat, human_time (&(statbuf->st_mtime)));
      break;
    case 'Y':
      strcat (pformat, "d");
      printf (pformat, (int) statbuf->st_mtime);
      break;
    case 'z':
      strcat (pformat, "s");
      printf (pformat, human_time (&(statbuf->st_ctime)));
      break;
    case 'Z':
      strcat (pformat, "d");
      printf (pformat, (int) statbuf->st_ctime);
      break;
    default:
      strcat (pformat, "c");
      printf (pformat, m);
      break;
    }
}

static void
print_it (char const *masterformat, char const *filename,
	  void (*print_func) (char *, char, char const *, void const *),
	  void const *data)
{
  char *b;

  /* create a working copy of the format string */
  char *format = xstrdup (masterformat);

  char *dest = xmalloc (strlen (format) + 1);


  b = format;
  while (b)
    {
      char *p = strchr (b, '%');
      if (p != NULL)
	{
	  size_t len;
	  *p++ = '\0';
	  fputs (b, stdout);

	  len = strspn (p, "#-+.I 0123456789");
	  dest[0] = '%';
	  memcpy (dest + 1, p, len);
	  dest[1 + len] = 0;
	  p += len;

	  switch (*p)
	    {
	    case '\0':
	    case '%':
	      putchar ('%');
	      break;
	    default:
	      print_func (dest, *p, filename, data);
	      break;
	    }
	  b = p + 1;
	}
      else
	{
	  fputs (b, stdout);
	  b = NULL;
	}
    }
  free (format);
  free (dest);
  fputc ('\n', stdout);
}

/* stat the filesystem and print what we find */
static void
do_statfs (char const *filename, int terse, char const *format)
{
  STRUCT_STATVFS statfsbuf;
  int i = statfs (filename, &statfsbuf);

  if (i == -1)
    {
      error (0, errno, _("cannot read file system information for %s"),
	     quote (filename));
      return;
    }

  if (format == NULL)
    {
      format = (terse
		? "%n %i %l %t %b %f %a %s %c %d"
		: "  File: \"%n\"\n"
		"    ID: %-8i Namelen: %-7l Type: %T\n"
		"Blocks: Total: %-10b Free: %-10f Available: %-10a Size: %s\n"
		"Inodes: Total: %-10c Free: %-10d");
    }

  print_it (format, filename, print_statfs, &statfsbuf);
}

/* stat the file and print what we find */
static void
do_stat (char const *filename, int follow_links, int terse,
	 char const *format)
{
  struct stat statbuf;
  int i = ((follow_links == 1)
	   ? stat (filename, &statbuf)
	   : lstat (filename, &statbuf));

  if (i == -1)
    {
      error (0, errno, _("cannot stat %s"), quote (filename));
      return;
    }

  if (format == NULL)
    {
      if (terse != 0)
	{
	  format = "%n %s %b %f %u %g %D %i %h %t %T %X %Y %Z %o";
	}
      else
	{
	  /* tmp hack to match orignal output until conditional implemented */
	  i = statbuf.st_mode & S_IFMT;
	  if (i == S_IFCHR || i == S_IFBLK)
	    {
	      format =
		"  File: %N\n"
		"  Size: %-10s\tBlocks: %-10b IO Block: %-6o %F\n"
		"Device: %Dh/%dd\tInode: %-10i  Links: %-5h"
		" Device type: %t,%T\n"
		"Access: (%04a/%10.10A)  Uid: (%5u/%8U)   Gid: (%5g/%8G)\n"
		"Access: %x\n" "Modify: %y\n" "Change: %z\n";
	    }
	  else
	    {
	      format =
		"  File: %N\n"
		"  Size: %-10s\tBlocks: %-10b IO Block: %-6o %F\n"
		"Device: %Dh/%dd\tInode: %-10i  Links: %-5h\n"
		"Access: (%04a/%10.10A)  Uid: (%5u/%8U)   Gid: (%5g/%8G)\n"
		"Access: %x\n" "Modify: %y\n" "Change: %z\n";
	    }
	}
    }
  print_it (format, filename, print_stat, &statbuf);
}

void
usage (int status)
{
  if (status != 0)
    fprintf (stderr, _("Try `%s --help' for more information.\n"),
	     program_name);
  else
    {
      printf (_("Usage: %s [OPTION] FILE...\n"), program_name);
      fputs (_("\
Display file or filesystem status.\n\
\n\
  -f, --filesystem      display filesystem status instead of file status\n\
  -c  --format=FORMAT   use the specified FORMAT instead of the default\n\
  -L, --dereference     follow links\n\
  -t, --terse           print the information in terse form\n\
"), stdout);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);

      fputs (_("\n\
The valid format sequences for files (without --filesystem):\n\
\n\
  %A   Access rights in human readable form\n\
  %a   Access rights in octal\n\
  %B   The size in bytes of each block reported by `%b'\n\
  %b   Number of blocks allocated (see %B)\n\
"), stdout);
      fputs (_("\
  %D   Device number in hex\n\
  %d   Device number in decimal\n\
  %F   File type\n\
  %f   Raw mode in hex\n\
  %G   Group name of owner\n\
  %g   Group ID of owner\n\
"), stdout);
      fputs (_("\
  %h   Number of hard links\n\
  %i   Inode number\n\
  %N   Quoted File name with dereference if symbolic link\n\
  %n   File name\n\
  %o   IO block size\n\
  %s   Total size, in bytes\n\
  %T   Minor device type in hex\n\
  %t   Major device type in hex\n\
"), stdout);
      fputs (_("\
  %U   User name of owner\n\
  %u   User ID of owner\n\
  %X   Time of last access as seconds since Epoch\n\
  %x   Time of last access\n\
  %Y   Time of last modification as seconds since Epoch\n\
  %y   Time of last modification\n\
  %Z   Time of last change as seconds since Epoch\n\
  %z   Time of last change\n\
\n\
"), stdout);

      fputs (_("\
Valid format sequences for file systems:\n\
\n\
  %a   Free blocks available to non-superuser\n\
  %b   Total data blocks in file system\n\
  %c   Total file nodes in file system\n\
  %d   Free file nodes in file system\n\
  %f   Free blocks in file system\n\
"), stdout);
      fputs (_("\
  %i   File System id in hex\n\
  %l   Maximum length of filenames\n\
  %n   File name\n\
  %s   Optimal transfer block size\n\
  %T   Type in human readable form\n\
  %t   Type in hex\n\
"), stdout);
      printf (_("\nReport bugs to <%s>.\n"), PACKAGE_BUGREPORT);
    }
  exit (status);
}

int
main (int argc, char *argv[])
{
  int c;
  int i;
  int follow_links = 0;
  int fs = 0;
  int terse = 0;
  char *format = NULL;

  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  while ((c = getopt_long (argc, argv, "c:fLlt", long_options, NULL)) != -1)
    {
      switch (c)
	{
	case 'c':
	  format = optarg;
	  break;
	case 'l': /* deprecated */
	case 'L':
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
	do_stat (argv[i], follow_links, terse, format);
      else
	do_statfs (argv[i], terse, format);
    }

  exit (G_fail ? EXIT_FAILURE : EXIT_SUCCESS);
}
