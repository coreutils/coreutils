/* stat.c -- display file or file system status
   Copyright (C) 2001, 2002, 2003, 2004, 2005 Free Software Foundation.

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
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

   Written by Michael Meskes.  */

#include <config.h>

#include <stdio.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#if HAVE_SYS_STATVFS_H && HAVE_STRUCT_STATVFS_F_BASETYPE
# include <sys/statvfs.h>
#elif HAVE_SYS_VFS_H
# include <sys/vfs.h>
#elif HAVE_SYS_MOUNT_H && HAVE_SYS_PARAM_H
/* NOTE: freebsd5.0 needs sys/param.h and sys/mount.h for statfs.
   It does have statvfs.h, but shouldn't use it, since it doesn't
   HAVE_STRUCT_STATVFS_F_BASETYPE.  So find a clean way to fix it.  */
/* NetBSD 1.5.2 needs these, for the declaration of struct statfs. */
# include <sys/param.h>
# include <sys/mount.h>
# if HAVE_NETINET_IN_H && HAVE_NFS_NFS_CLNT_H && HAVE_NFS_VFS_H
/* Ultrix 4.4 needs these for the declaration of struct statfs.  */
#  include <netinet/in.h>
#  include <nfs/nfs_clnt.h>
#  include <nfs/vfs.h>
# endif
#endif

#include "system.h"

#include "error.h"
#include "filemode.h"
#include "file-type.h"
#include "fs.h"
#include "getopt.h"
#include "inttostr.h"
#include "quote.h"
#include "quotearg.h"
#include "stat-time.h"
#include "strftime.h"
#include "xreadlink.h"

#define NAMEMAX_FORMAT PRIuMAX

#if HAVE_STRUCT_STATVFS_F_BASETYPE
# define STRUCT_STATVFS struct statvfs
# define HAVE_STRUCT_STATXFS_F_TYPE HAVE_STRUCT_STATVFS_F_TYPE
# if HAVE_STRUCT_STATVFS_F_NAMEMAX
#  define SB_F_NAMEMAX(S) ((uintmax_t) ((S)->f_namemax))
# endif
# if STAT_STATVFS
#  define STATFS statvfs
#  define STATFS_FRSIZE(S) ((S)->f_frsize)
# endif
#else
# define STRUCT_STATVFS struct statfs
# define HAVE_STRUCT_STATXFS_F_TYPE HAVE_STRUCT_STATFS_F_TYPE
# if HAVE_STRUCT_STATFS_F_NAMELEN
#  define SB_F_NAMEMAX(S) ((uintmax_t) ((S)->f_namelen))
# endif
#endif

#ifndef STATFS
# define STATFS statfs
# define STATFS_FRSIZE(S) 0
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
  {"dereference", no_argument, NULL, 'L'},
  {"file-system", no_argument, NULL, 'f'},
  {"filesystem", no_argument, NULL, 'f'}, /* obsolete and undocumented alias */
  {"format", required_argument, NULL, 'c'},
  {"terse", no_argument, NULL, 't'},
  {GETOPT_HELP_OPTION_DECL},
  {GETOPT_VERSION_OPTION_DECL},
  {NULL, 0, NULL, 0}
};

char *program_name;

/* Return the type of the specified file system.
   Some systems have statfvs.f_basetype[FSTYPSZ]. (AIX, HP-UX, and Solaris)
   Others have statfs.f_fstypename[MFSNAMELEN]. (NetBSD 1.5.2)
   Still others have neither and have to get by with f_type (Linux).  */
static char const *
human_fstype (STRUCT_STATVFS const *statfsbuf)
{
#ifdef STATXFS_FILE_SYSTEM_TYPE_MEMBER_NAME
  return statfsbuf->STATXFS_FILE_SYSTEM_TYPE_MEMBER_NAME;
#else
  switch (statfsbuf->f_type)
    {
# if defined __linux__

      /* IMPORTANT NOTE: Each of the following `case S_MAGIC_...:'
	 statements must be followed by a hexadecimal constant in
	 a comment.  The S_MAGIC_... name and constant are automatically
	 combined to produce the #define directives in fs.h.  */

    case S_MAGIC_AFFS: /* 0xADFF */
      return "affs";
    case S_MAGIC_DEVPTS: /* 0x1CD1 */
      return "devpts";
    case S_MAGIC_EXT: /* 0x137D */
      return "ext";
    case S_MAGIC_EXT2_OLD: /* 0xEF51 */
      return "ext2";
    case S_MAGIC_EXT2: /* 0xEF53 */
      return "ext2/ext3";
    case S_MAGIC_JFS: /* 0x3153464a */
      return "jfs";
    case S_MAGIC_XFS: /* 0x58465342 */
      return "xfs";
    case S_MAGIC_HPFS: /* 0xF995E849 */
      return "hpfs";
    case S_MAGIC_ISOFS: /* 0x9660 */
      return "isofs";
    case S_MAGIC_ISOFS_WIN: /* 0x4000 */
      return "isofs";
    case S_MAGIC_ISOFS_R_WIN: /* 0x4004 */
      return "isofs";
    case S_MAGIC_MINIX: /* 0x137F */
      return "minix";
    case S_MAGIC_MINIX_30: /* 0x138F */
      return "minix (30 char.)";
    case S_MAGIC_MINIX_V2: /* 0x2468 */
      return "minix v2";
    case S_MAGIC_MINIX_V2_30: /* 0x2478 */
      return "minix v2 (30 char.)";
    case S_MAGIC_MSDOS: /* 0x4d44 */
      return "msdos";
    case S_MAGIC_FAT: /* 0x4006 */
      return "fat";
    case S_MAGIC_NCP: /* 0x564c */
      return "novell";
    case S_MAGIC_NFS: /* 0x6969 */
      return "nfs";
    case S_MAGIC_PROC: /* 0x9fa0 */
      return "proc";
    case S_MAGIC_SMB: /* 0x517B */
      return "smb";
    case S_MAGIC_XENIX: /* 0x012FF7B4 */
      return "xenix";
    case S_MAGIC_SYSV4: /* 0x012FF7B5 */
      return "sysv4";
    case S_MAGIC_SYSV2: /* 0x012FF7B6 */
      return "sysv2";
    case S_MAGIC_COH: /* 0x012FF7B7 */
      return "coh";
    case S_MAGIC_UFS: /* 0x00011954 */
      return "ufs";
    case S_MAGIC_XIAFS: /* 0x012FD16D */
      return "xia";
    case S_MAGIC_NTFS: /* 0x5346544e */
      return "ntfs";
    case S_MAGIC_TMPFS: /* 0x1021994 */
      return "tmpfs";
    case S_MAGIC_REISERFS: /* 0x52654973 */
      return "reiserfs";
    case S_MAGIC_CRAMFS: /* 0x28cd3d45 */
      return "cramfs";
    case S_MAGIC_ROMFS: /* 0x7275 */
      return "romfs";
    case S_MAGIC_RAMFS: /* 0x858458f6 */
      return "ramfs";
    case S_MAGIC_SQUASHFS: /* 0x73717368 */
      return "squashfs";
    case S_MAGIC_SYSFS: /* 0x62656572 */
      return "sysfs";
# elif __GNU__
    case FSTYPE_UFS:
      return "ufs";
    case FSTYPE_NFS:
      return "nfs";
    case FSTYPE_GFS:
      return "gfs";
    case FSTYPE_LFS:
      return "lfs";
    case FSTYPE_SYSV:
      return "sysv";
    case FSTYPE_FTP:
      return "ftp";
    case FSTYPE_TAR:
      return "tar";
    case FSTYPE_AR:
      return "ar";
    case FSTYPE_CPIO:
      return "cpio";
    case FSTYPE_MSLOSS:
      return "msloss";
    case FSTYPE_CPM:
      return "cpm";
    case FSTYPE_HFS:
      return "hfs";
    case FSTYPE_DTFS:
      return "dtfs";
    case FSTYPE_GRFS:
      return "grfs";
    case FSTYPE_TERM:
      return "term";
    case FSTYPE_DEV:
      return "dev";
    case FSTYPE_PROC:
      return "proc";
    case FSTYPE_IFSOCK:
      return "ifsock";
    case FSTYPE_AFS:
      return "afs";
    case FSTYPE_DFS:
      return "dfs";
    case FSTYPE_PROC9:
      return "proc9";
    case FSTYPE_SOCKET:
      return "socket";
    case FSTYPE_MISC:
      return "misc";
    case FSTYPE_EXT2FS:
      return "ext2/ext3";
    case FSTYPE_HTTP:
      return "http";
    case FSTYPE_MEMFS:
      return "memfs";
    case FSTYPE_ISO9660:
      return "iso9660";
# endif
    default:
      {
	unsigned long int type = statfsbuf->f_type;
	static char buf[sizeof "UNKNOWN (0x%lx)" - 3
			+ (sizeof type * CHAR_BIT + 3) / 4];
	sprintf (buf, "UNKNOWN (0x%lx)", type);
	return buf;
      }
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
human_time (struct timespec t)
{
  static char str[MAX (INT_BUFSIZE_BOUND (intmax_t),
		       (INT_STRLEN_BOUND (int) /* YYYY */
			+ 1 /* because YYYY might equal INT_MAX + 1900 */
			+ sizeof "-MM-DD HH:MM:SS.NNNNNNNNN +ZZZZ"))];
  struct tm const *tm = localtime (&t.tv_sec);
  if (tm == NULL)
    return (TYPE_SIGNED (time_t)
	    ? imaxtostr (t.tv_sec, str)
	    : umaxtostr (t.tv_sec, str));
  nstrftime (str, sizeof str, "%Y-%m-%d %H:%M:%S.%N %z", tm, 0, t.tv_nsec);
  return str;
}

/* Like strcat, but don't return anything and do check that
   DEST_BUFSIZE is at least a long as strlen (DEST) + strlen (SRC) + 1.
   The signature is deliberately different from that of strncat.  */
static void
xstrcat (char *dest, size_t dest_bufsize, char const *src)
{
  size_t dest_len = strlen (dest);
  size_t src_len = strlen (src);
  if (dest_bufsize < dest_len + src_len + 1)
    abort ();
  memcpy (dest + dest_len, src, src_len + 1);
}

/* print statfs info */
static void
print_statfs (char *pformat, size_t buf_len, char m, char const *filename,
	      void const *data)
{
  STRUCT_STATVFS const *statfsbuf = data;

  switch (m)
    {
    case 'n':
      xstrcat (pformat, buf_len, "s");
      printf (pformat, filename);
      break;

    case 'i':
#if HAVE_STRUCT_STATXFS_F_FSID___VAL
      xstrcat (pformat, buf_len, "x %-8x");
      printf (pformat, statfsbuf->f_fsid.__val[0], /* u_long */
	      statfsbuf->f_fsid.__val[1]);
#else
      xstrcat (pformat, buf_len, "Lx");
      printf (pformat, statfsbuf->f_fsid);
#endif
      break;

    case 'l':
      xstrcat (pformat, buf_len, NAMEMAX_FORMAT);
      printf (pformat, SB_F_NAMEMAX (statfsbuf));
      break;
    case 't':
#if HAVE_STRUCT_STATXFS_F_TYPE
      xstrcat (pformat, buf_len, "lx");
      printf (pformat,
	      (unsigned long int) (statfsbuf->f_type));  /* no equiv. */
#else
      fputc ('*', stdout);
#endif
      break;
    case 'T':
      xstrcat (pformat, buf_len, "s");
      printf (pformat, human_fstype (statfsbuf));
      break;
    case 'b':
      xstrcat (pformat, buf_len, PRIdMAX);
      printf (pformat, (intmax_t) (statfsbuf->f_blocks));
      break;
    case 'f':
      xstrcat (pformat, buf_len, PRIdMAX);
      printf (pformat, (intmax_t) (statfsbuf->f_bfree));
      break;
    case 'a':
      xstrcat (pformat, buf_len, PRIdMAX);
      printf (pformat, (intmax_t) (statfsbuf->f_bavail));
      break;
    case 's':
      xstrcat (pformat, buf_len, "lu");
      printf (pformat, (unsigned long int) (statfsbuf->f_bsize));
      break;
    case 'S':
      {
	unsigned long int frsize = STATFS_FRSIZE (statfsbuf);
	if (! frsize)
	  frsize = statfsbuf->f_bsize;
	xstrcat (pformat, buf_len, "lu");
	printf (pformat, frsize);
      }
      break;
    case 'c':
      xstrcat (pformat, buf_len, PRIdMAX);
      printf (pformat, (intmax_t) (statfsbuf->f_files));
      break;
    case 'd':
      xstrcat (pformat, buf_len, PRIdMAX);
      printf (pformat, (intmax_t) (statfsbuf->f_ffree));
      break;

    default:
      xstrcat (pformat, buf_len, "c");
      printf (pformat, m);
      break;
    }
}

/* print stat info */
static void
print_stat (char *pformat, size_t buf_len, char m,
	    char const *filename, void const *data)
{
  struct stat *statbuf = (struct stat *) data;
  struct passwd *pw_ent;
  struct group *gw_ent;

  switch (m)
    {
    case 'n':
      xstrcat (pformat, buf_len, "s");
      printf (pformat, filename);
      break;
    case 'N':
      xstrcat (pformat, buf_len, "s");
      if (S_ISLNK (statbuf->st_mode))
	{
	  char *linkname = xreadlink (filename, statbuf->st_size);
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
      xstrcat (pformat, buf_len, PRIuMAX);
      printf (pformat, (uintmax_t) statbuf->st_dev);
      break;
    case 'D':
      xstrcat (pformat, buf_len, PRIxMAX);
      printf (pformat, (uintmax_t) statbuf->st_dev);
      break;
    case 'i':
      xstrcat (pformat, buf_len, PRIuMAX);
      printf (pformat, (uintmax_t) statbuf->st_ino);
      break;
    case 'a':
      xstrcat (pformat, buf_len, "lo");
      printf (pformat,
	      (unsigned long int) (statbuf->st_mode & CHMOD_MODE_BITS));
      break;
    case 'A':
      xstrcat (pformat, buf_len, "s");
      printf (pformat, human_access (statbuf));
      break;
    case 'f':
      xstrcat (pformat, buf_len, "lx");
      printf (pformat, (unsigned long int) statbuf->st_mode);
      break;
    case 'F':
      xstrcat (pformat, buf_len, "s");
      printf (pformat, file_type (statbuf));
      break;
    case 'h':
      xstrcat (pformat, buf_len, "lu");
      printf (pformat, (unsigned long int) statbuf->st_nlink);
      break;
    case 'u':
      xstrcat (pformat, buf_len, "lu");
      printf (pformat, (unsigned long int) statbuf->st_uid);
      break;
    case 'U':
      xstrcat (pformat, buf_len, "s");
      setpwent ();
      pw_ent = getpwuid (statbuf->st_uid);
      printf (pformat, (pw_ent != 0L) ? pw_ent->pw_name : "UNKNOWN");
      break;
    case 'g':
      xstrcat (pformat, buf_len, "lu");
      printf (pformat, (unsigned long int) statbuf->st_gid);
      break;
    case 'G':
      xstrcat (pformat, buf_len, "s");
      setgrent ();
      gw_ent = getgrgid (statbuf->st_gid);
      printf (pformat, (gw_ent != 0L) ? gw_ent->gr_name : "UNKNOWN");
      break;
    case 't':
      xstrcat (pformat, buf_len, "lx");
      printf (pformat, (unsigned long int) major (statbuf->st_rdev));
      break;
    case 'T':
      xstrcat (pformat, buf_len, "lx");
      printf (pformat, (unsigned long int) minor (statbuf->st_rdev));
      break;
    case 's':
      xstrcat (pformat, buf_len, PRIuMAX);
      printf (pformat, (uintmax_t) (statbuf->st_size));
      break;
    case 'B':
      xstrcat (pformat, buf_len, "lu");
      printf (pformat, (unsigned long int) ST_NBLOCKSIZE);
      break;
    case 'b':
      xstrcat (pformat, buf_len, PRIuMAX);
      printf (pformat, (uintmax_t) ST_NBLOCKS (*statbuf));
      break;
    case 'o':
      xstrcat (pformat, buf_len, "lu");
      printf (pformat, (unsigned long int) statbuf->st_blksize);
      break;
    case 'x':
      xstrcat (pformat, buf_len, "s");
      printf (pformat, human_time (get_stat_atime (statbuf)));
      break;
    case 'X':
      xstrcat (pformat, buf_len, TYPE_SIGNED (time_t) ? "ld" : "lu");
      printf (pformat, (unsigned long int) statbuf->st_atime);
      break;
    case 'y':
      xstrcat (pformat, buf_len, "s");
      printf (pformat, human_time (get_stat_mtime (statbuf)));
      break;
    case 'Y':
      xstrcat (pformat, buf_len, TYPE_SIGNED (time_t) ? "ld" : "lu");
      printf (pformat, (unsigned long int) statbuf->st_mtime);
      break;
    case 'z':
      xstrcat (pformat, buf_len, "s");
      printf (pformat, human_time (get_stat_ctime (statbuf)));
      break;
    case 'Z':
      xstrcat (pformat, buf_len, TYPE_SIGNED (time_t) ? "ld" : "lu");
      printf (pformat, (unsigned long int) statbuf->st_ctime);
      break;
    default:
      xstrcat (pformat, buf_len, "c");
      printf (pformat, m);
      break;
    }
}

static void
print_it (char const *masterformat, char const *filename,
	  void (*print_func) (char *, size_t, char, char const *, void const *),
	  void const *data)
{
  char *b;

  /* create a working copy of the format string */
  char *format = xstrdup (masterformat);

  /* Add 2 to accommodate our conversion of the stat `%s' format string
     to the printf `%llu' one.  */
  size_t n_alloc = strlen (format) + 2 + 1;
  char *dest = xmalloc (n_alloc);

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

	  b = p + 1;
	  switch (*p)
	    {
	    case '\0':
	      b = NULL;
	      /* fall through */
	    case '%':
	      putchar ('%');
	      break;
	    default:
	      print_func (dest, n_alloc, *p, filename, data);
	      break;
	    }
	}
      else
	{
	  fputs (b, stdout);
	  b = NULL;
	}
    }
  free (format);
  free (dest);
}

/* Stat the file system and print what we find.  */
static bool
do_statfs (char const *filename, bool terse, char const *format)
{
  STRUCT_STATVFS statfsbuf;

  if (STATFS (filename, &statfsbuf) != 0)
    {
      error (0, errno, _("cannot read file system information for %s"),
	     quote (filename));
      return false;
    }

  if (format == NULL)
    {
      format = (terse
		? "%n %i %l %t %s %S %b %f %a %c %d\n"
		: "  File: \"%n\"\n"
		"    ID: %-8i Namelen: %-7l Type: %T\n"
		"Block size: %-10s Fundamental block size: %S\n"
		"Blocks: Total: %-10b Free: %-10f Available: %a\n"
		"Inodes: Total: %-10c Free: %d\n");
    }

  print_it (format, filename, print_statfs, &statfsbuf);
  return true;
}

/* stat the file and print what we find */
static bool
do_stat (char const *filename, bool follow_links, bool terse,
	 char const *format)
{
  struct stat statbuf;

  if ((follow_links ? stat : lstat) (filename, &statbuf) != 0)
    {
      error (0, errno, _("cannot stat %s"), quote (filename));
      return false;
    }

  if (format == NULL)
    {
      if (terse)
	{
	  format = "%n %s %b %f %u %g %D %i %h %t %T %X %Y %Z %o\n";
	}
      else
	{
	  /* Temporary hack to match original output until conditional
	     implemented.  */
	  if (S_ISBLK (statbuf.st_mode) || S_ISCHR (statbuf.st_mode))
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
		"Device: %Dh/%dd\tInode: %-10i  Links: %h\n"
		"Access: (%04a/%10.10A)  Uid: (%5u/%8U)   Gid: (%5g/%8G)\n"
		"Access: %x\n" "Modify: %y\n" "Change: %z\n";
	    }
	}
    }
  print_it (format, filename, print_stat, &statbuf);
  return true;
}

void
usage (int status)
{
  if (status != EXIT_SUCCESS)
    fprintf (stderr, _("Try `%s --help' for more information.\n"),
	     program_name);
  else
    {
      printf (_("Usage: %s [OPTION] FILE...\n"), program_name);
      fputs (_("\
Display file or file system status.\n\
\n\
  -f, --file-system     display file system status instead of file status\n\
  -c  --format=FORMAT   use the specified FORMAT instead of the default\n\
  -L, --dereference     follow links\n\
  -t, --terse           print the information in terse form\n\
"), stdout);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);

      fputs (_("\n\
The valid format sequences for files (without --file-system):\n\
\n\
  %a   Access rights in octal\n\
  %A   Access rights in human readable form\n\
  %b   Number of blocks allocated (see %B)\n\
  %B   The size in bytes of each block reported by %b\n\
"), stdout);
      fputs (_("\
  %d   Device number in decimal\n\
  %D   Device number in hex\n\
  %f   Raw mode in hex\n\
  %F   File type\n\
  %g   Group ID of owner\n\
  %G   Group name of owner\n\
"), stdout);
      fputs (_("\
  %h   Number of hard links\n\
  %i   Inode number\n\
  %n   File name\n\
  %N   Quoted file name with dereference if symbolic link\n\
  %o   I/O block size\n\
  %s   Total size, in bytes\n\
  %t   Major device type in hex\n\
  %T   Minor device type in hex\n\
"), stdout);
      fputs (_("\
  %u   User ID of owner\n\
  %U   User name of owner\n\
  %x   Time of last access\n\
  %X   Time of last access as seconds since Epoch\n\
  %y   Time of last modification\n\
  %Y   Time of last modification as seconds since Epoch\n\
  %z   Time of last change\n\
  %Z   Time of last change as seconds since Epoch\n\
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
  %i   File System ID in hex\n\
  %l   Maximum length of filenames\n\
  %n   File name\n\
  %s   Block size (for faster transfers)\n\
  %S   Fundamental block size (for block counts)\n\
  %t   Type in hex\n\
  %T   Type in human readable form\n\
"), stdout);
      printf (USAGE_BUILTIN_WARNING, PROGRAM_NAME);
      printf (_("\nReport bugs to <%s>.\n"), PACKAGE_BUGREPORT);
    }
  exit (status);
}

int
main (int argc, char *argv[])
{
  int c;
  int i;
  bool follow_links = false;
  bool fs = false;
  bool terse = false;
  char *format = NULL;
  bool ok = true;

  initialize_main (&argc, &argv);
  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  while ((c = getopt_long (argc, argv, "c:fLt", long_options, NULL)) != -1)
    {
      switch (c)
	{
	case 'c':
	  format = optarg;
	  break;

	case 'L':
	  follow_links = true;
	  break;

	case 'f':
	  fs = true;
	  break;

	case 't':
	  terse = true;
	  break;

	case_GETOPT_HELP_CHAR;

	case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);

	default:
	  usage (EXIT_FAILURE);
	}
    }

  if (argc == optind)
    {
      error (0, 0, _("missing operand"));
      usage (EXIT_FAILURE);
    }

  for (i = optind; i < argc; i++)
    ok &= (fs
	   ? do_statfs (argv[i], terse, format)
	   : do_stat (argv[i], follow_links, terse, format));

  exit (ok ? EXIT_SUCCESS : EXIT_FAILURE);
}
