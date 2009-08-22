/* stat.c -- display file or file system status
   Copyright (C) 2001-2009 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Written by Michael Meskes.  */

#include <config.h>

/* Keep this conditional in sync with the similar conditional in
   ../m4/stat-prog.m4.  */
#if (STAT_STATVFS \
     && (HAVE_STRUCT_STATVFS_F_BASETYPE || HAVE_STRUCT_STATVFS_F_FSTYPENAME \
         || (! HAVE_STRUCT_STATFS_F_FSTYPENAME && HAVE_STRUCT_STATVFS_F_TYPE)))
# define USE_STATVFS 1
#else
# define USE_STATVFS 0
#endif

#include <stddef.h>
#include <stdio.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#if USE_STATVFS
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
#elif HAVE_OS_H /* BeOS */
# include <fs_info.h>
#endif
#include <selinux/selinux.h>

#include "system.h"

#include "error.h"
#include "filemode.h"
#include "file-type.h"
#include "fs.h"
#include "getopt.h"
#include "quote.h"
#include "quotearg.h"
#include "stat-time.h"
#include "strftime.h"
#include "areadlink.h"

#define alignof(type) offsetof (struct { char c; type x; }, x)

#if USE_STATVFS
# define STRUCT_STATVFS struct statvfs
# define STRUCT_STATXFS_F_FSID_IS_INTEGER STRUCT_STATVFS_F_FSID_IS_INTEGER
# define HAVE_STRUCT_STATXFS_F_TYPE HAVE_STRUCT_STATVFS_F_TYPE
# if HAVE_STRUCT_STATVFS_F_NAMEMAX
#  define SB_F_NAMEMAX(S) ((S)->f_namemax)
# endif
# define STATFS statvfs
# define STATFS_FRSIZE(S) ((S)->f_frsize)
#else
# define HAVE_STRUCT_STATXFS_F_TYPE HAVE_STRUCT_STATFS_F_TYPE
# if HAVE_STRUCT_STATFS_F_NAMELEN
#  define SB_F_NAMEMAX(S) ((S)->f_namelen)
# endif
# define STATFS statfs
# if HAVE_OS_H /* BeOS */
/* BeOS has a statvfs function, but it does not return sensible values
   for f_files, f_ffree and f_favail, and lacks f_type, f_basetype and
   f_fstypename.  Use 'struct fs_info' instead.  */
static int
statfs (char const *filename, struct fs_info *buf)
{
  dev_t device = dev_for_path (filename);
  if (device < 0)
    {
      errno = (device == B_ENTRY_NOT_FOUND ? ENOENT
               : device == B_BAD_VALUE ? EINVAL
               : device == B_NAME_TOO_LONG ? ENAMETOOLONG
               : device == B_NO_MEMORY ? ENOMEM
               : device == B_FILE_ERROR ? EIO
               : 0);
      return -1;
    }
  /* If successful, buf->dev will be == device.  */
  return fs_stat_dev (device, buf);
}
#  define f_fsid dev
#  define f_blocks total_blocks
#  define f_bfree free_blocks
#  define f_bavail free_blocks
#  define f_bsize io_size
#  define f_files total_nodes
#  define f_ffree free_nodes
#  define STRUCT_STATVFS struct fs_info
#  define STRUCT_STATXFS_F_FSID_IS_INTEGER true
#  define STATFS_FRSIZE(S) ((S)->block_size)
# else
#  define STRUCT_STATVFS struct statfs
#  define STRUCT_STATXFS_F_FSID_IS_INTEGER STRUCT_STATFS_F_FSID_IS_INTEGER
#  define STATFS_FRSIZE(S) 0
# endif
#endif

#ifdef SB_F_NAMEMAX
# define OUT_NAMEMAX out_uint
#else
/* NetBSD 1.5.2 has neither f_namemax nor f_namelen.  */
# define SB_F_NAMEMAX(S) "*"
# define OUT_NAMEMAX out_string
#endif

#if HAVE_STRUCT_STATVFS_F_BASETYPE
# define STATXFS_FILE_SYSTEM_TYPE_MEMBER_NAME f_basetype
#else
# if HAVE_STRUCT_STATVFS_F_FSTYPENAME || HAVE_STRUCT_STATFS_F_FSTYPENAME
#  define STATXFS_FILE_SYSTEM_TYPE_MEMBER_NAME f_fstypename
# elif HAVE_OS_H /* BeOS */
#  define STATXFS_FILE_SYSTEM_TYPE_MEMBER_NAME fsh_name
# endif
#endif

/* FIXME: these are used by printf.c, too */
#define isodigit(c) ('0' <= (c) && (c) <= '7')
#define octtobin(c) ((c) - '0')
#define hextobin(c) ((c) >= 'a' && (c) <= 'f' ? (c) - 'a' + 10 : \
                     (c) >= 'A' && (c) <= 'F' ? (c) - 'A' + 10 : (c) - '0')

#define PROGRAM_NAME "stat"

#define AUTHORS proper_name ("Michael Meskes")

enum
{
  PRINTF_OPTION = CHAR_MAX + 1
};

static struct option const long_options[] =
{
  {"context", no_argument, 0, 'Z'},
  {"dereference", no_argument, NULL, 'L'},
  {"file-system", no_argument, NULL, 'f'},
  {"format", required_argument, NULL, 'c'},
  {"printf", required_argument, NULL, PRINTF_OPTION},
  {"terse", no_argument, NULL, 't'},
  {GETOPT_HELP_OPTION_DECL},
  {GETOPT_VERSION_OPTION_DECL},
  {NULL, 0, NULL, 0}
};

/* Whether to follow symbolic links;  True for --dereference (-L).  */
static bool follow_links;

/* Whether to interpret backslash-escape sequences.
   True for --printf=FMT, not for --format=FMT (-c).  */
static bool interpret_backslash_escapes;

/* The trailing delimiter string:
   "" for --printf=FMT, "\n" for --format=FMT (-c).  */
static char const *trailing_delim = "";

/* Return the type of the specified file system.
   Some systems have statfvs.f_basetype[FSTYPSZ] (AIX, HP-UX, and Solaris).
   Others have statvfs.f_fstypename[_VFS_NAMELEN] (NetBSD 3.0).
   Others have statfs.f_fstypename[MFSNAMELEN] (NetBSD 1.5.2).
   Still others have neither and have to get by with f_type (GNU/Linux).
   But f_type may only exist in statfs (Cygwin).  */
static char const *
human_fstype (STRUCT_STATVFS const *statfsbuf)
{
#ifdef STATXFS_FILE_SYSTEM_TYPE_MEMBER_NAME
  return statfsbuf->STATXFS_FILE_SYSTEM_TYPE_MEMBER_NAME;
#else
  switch (statfsbuf->f_type)
    {
# if defined __linux__

      /* Compare with what's in libc:
         f=/a/libc/sysdeps/unix/sysv/linux/linux_fsinfo.h
         sed -n '/ADFS_SUPER_MAGIC/,/SYSFS_MAGIC/p' $f \
           | perl -n -e '/#define (.*?)_(?:SUPER_)MAGIC\s+0x(\S+)/' \
             -e 'and print "case S_MAGIC_$1: /\* 0x" . uc($2) . " *\/\n"' \
           | sort > sym_libc
         perl -ne '/^\s+(case S_MAGIC_.*?): \/\* 0x(\S+) \*\//' \
             -e 'and do { $v=uc$2; print "$1: /\* 0x$v *\/\n"}' stat.c \
           | sort > sym_stat
         diff -u sym_stat sym_libc
      */

      /* Also sync from the list in "man 2 statfs".  */

      /* IMPORTANT NOTE: Each of the following `case S_MAGIC_...:'
         statements must be followed by a hexadecimal constant in
         a comment.  The S_MAGIC_... name and constant are automatically
         combined to produce the #define directives in fs.h.  */

    case S_MAGIC_ADFS: /* 0xADF5 */
      return "adfs";
    case S_MAGIC_AFFS: /* 0xADFF */
      return "affs";
    case S_MAGIC_AUTOFS: /* 0x187 */
      return "autofs";
    case S_MAGIC_BEFS: /* 0x42465331 */
      return "befs";
    case S_MAGIC_BFS: /* 0x1BADFACE */
      return "bfs";
    case S_MAGIC_BINFMT_MISC: /* 0x42494e4d */
      return "binfmt_misc";
    case S_MAGIC_CODA: /* 0x73757245 */
      return "coda";
    case S_MAGIC_COH: /* 0x012FF7B7 */
      return "coh";
    case S_MAGIC_CRAMFS: /* 0x28CD3D45 */
      return "cramfs";
    case S_MAGIC_DEVFS: /* 0x1373 */
      return "devfs";
    case S_MAGIC_DEVPTS: /* 0x1CD1 */
      return "devpts";
    case S_MAGIC_EFS: /* 0x414A53 */
      return "efs";
    case S_MAGIC_EXT: /* 0x137D */
      return "ext";
    case S_MAGIC_EXT2: /* 0xEF53 */
      return "ext2/ext3";
    case S_MAGIC_EXT2_OLD: /* 0xEF51 */
      return "ext2";
    case S_MAGIC_FAT: /* 0x4006 */
      return "fat";
    case S_MAGIC_FUSECTL: /* 0x65735543 */
      return "fusectl";
    case S_MAGIC_HPFS: /* 0xF995E849 */
      return "hpfs";
    case S_MAGIC_HUGETLBFS: /* 0x958458f6 */
      return "hugetlbfs";
    case S_MAGIC_ISOFS: /* 0x9660 */
      return "isofs";
    case S_MAGIC_ISOFS_R_WIN: /* 0x4004 */
      return "isofs";
    case S_MAGIC_ISOFS_WIN: /* 0x4000 */
      return "isofs";
    case S_MAGIC_JFFS2: /* 0x72B6 */
      return "jffs2";
    case S_MAGIC_JFFS: /* 0x07C0 */
      return "jffs";
    case S_MAGIC_JFS: /* 0x3153464A */
      return "jfs";
    case S_MAGIC_LUSTRE: /* 0x0BD00BD0 */
      return "lustre";
    case S_MAGIC_MINIX: /* 0x137F */
      return "minix";
    case S_MAGIC_MINIX_30: /* 0x138F */
      return "minix (30 char.)";
    case S_MAGIC_MINIX_V2: /* 0x2468 */
      return "minix v2";
    case S_MAGIC_MINIX_V2_30: /* 0x2478 */
      return "minix v2 (30 char.)";
    case S_MAGIC_MSDOS: /* 0x4D44 */
      return "msdos";
    case S_MAGIC_NCP: /* 0x564C */
      return "novell";
    case S_MAGIC_NFS: /* 0x6969 */
      return "nfs";
    case S_MAGIC_NFSD: /* 0x6E667364 */
      return "nfsd";
    case S_MAGIC_NTFS: /* 0x5346544E */
      return "ntfs";
    case S_MAGIC_OPENPROM: /* 0x9fa1 */
      return "openprom";
    case S_MAGIC_PROC: /* 0x9FA0 */
      return "proc";
    case S_MAGIC_QNX4: /* 0x002F */
      return "qnx4";
    case S_MAGIC_RAMFS: /* 0x858458F6 */
      return "ramfs";
    case S_MAGIC_REISERFS: /* 0x52654973 */
      return "reiserfs";
    case S_MAGIC_ROMFS: /* 0x7275 */
      return "romfs";
    case S_MAGIC_SMB: /* 0x517B */
      return "smb";
    case S_MAGIC_SQUASHFS: /* 0x73717368 */
      return "squashfs";
    case S_MAGIC_SYSFS: /* 0x62656572 */
      return "sysfs";
    case S_MAGIC_SYSV2: /* 0x012FF7B6 */
      return "sysv2";
    case S_MAGIC_SYSV4: /* 0x012FF7B5 */
      return "sysv4";
    case S_MAGIC_TMPFS: /* 0x1021994 */
      return "tmpfs";
    case S_MAGIC_UDF: /* 0x15013346 */
      return "udf";
    case S_MAGIC_UFS: /* 0x00011954 */
      return "ufs";
    case S_MAGIC_UFS_BYTESWAPPED: /* 0x54190100 */
      return "ufs";
    case S_MAGIC_USBDEVFS: /* 0x9FA2 */
      return "usbdevfs";
    case S_MAGIC_VXFS: /* 0xA501FCF5 */
      return "vxfs";
    case S_MAGIC_XENIX: /* 0x012FF7B4 */
      return "xenix";
    case S_MAGIC_XFS: /* 0x58465342 */
      return "xfs";
    case S_MAGIC_XIAFS: /* 0x012FD16D */
      return "xia";

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
  static char modebuf[12];
  filemodestring (statbuf, modebuf);
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
    return timetostr (t.tv_sec, str);
  nstrftime (str, sizeof str, "%Y-%m-%d %H:%M:%S.%N %z", tm, 0, t.tv_nsec);
  return str;
}

static void
out_string (char *pformat, size_t prefix_len, char const *arg)
{
  strcpy (pformat + prefix_len, "s");
  printf (pformat, arg);
}
static void
out_int (char *pformat, size_t prefix_len, intmax_t arg)
{
  strcpy (pformat + prefix_len, PRIdMAX);
  printf (pformat, arg);
}
static void
out_uint (char *pformat, size_t prefix_len, uintmax_t arg)
{
  strcpy (pformat + prefix_len, PRIuMAX);
  printf (pformat, arg);
}
static void
out_uint_o (char *pformat, size_t prefix_len, uintmax_t arg)
{
  strcpy (pformat + prefix_len, PRIoMAX);
  printf (pformat, arg);
}
static void
out_uint_x (char *pformat, size_t prefix_len, uintmax_t arg)
{
  strcpy (pformat + prefix_len, PRIxMAX);
  printf (pformat, arg);
}

/* Very specialized function (modifies FORMAT), just so as to avoid
   duplicating this code between both print_statfs and print_stat.  */
static void
out_file_context (char const *filename, char *pformat, size_t prefix_len)
{
  char *scontext;
  if ((follow_links
       ? getfilecon (filename, &scontext)
       : lgetfilecon (filename, &scontext)) < 0)
    {
      error (0, errno, _("failed to get security context of %s"),
             quote (filename));
      scontext = NULL;
    }
  strcpy (pformat + prefix_len, "s");
  printf (pformat, (scontext ? scontext : "?"));
  if (scontext)
    freecon (scontext);
}

/* print statfs info */
static void
print_statfs (char *pformat, size_t prefix_len, char m, char const *filename,
              void const *data)
{
  STRUCT_STATVFS const *statfsbuf = data;

  switch (m)
    {
    case 'n':
      out_string (pformat, prefix_len, filename);
      break;

    case 'i':
      {
#if STRUCT_STATXFS_F_FSID_IS_INTEGER
        uintmax_t fsid = statfsbuf->f_fsid;
#else
        typedef unsigned int fsid_word;
        verify (alignof (STRUCT_STATVFS) % alignof (fsid_word) == 0);
        verify (offsetof (STRUCT_STATVFS, f_fsid) % alignof (fsid_word) == 0);
        verify (sizeof statfsbuf->f_fsid % alignof (fsid_word) == 0);
        fsid_word const *p = (fsid_word *) &statfsbuf->f_fsid;

        /* Assume a little-endian word order, as that is compatible
           with glibc's statvfs implementation.  */
        uintmax_t fsid = 0;
        int words = sizeof statfsbuf->f_fsid / sizeof *p;
        int i;
        for (i = 0; i < words && i * sizeof *p < sizeof fsid; i++)
          {
            uintmax_t u = p[words - 1 - i];
            fsid |= u << (i * CHAR_BIT * sizeof *p);
          }
#endif
        out_uint_x (pformat, prefix_len, fsid);
      }
      break;

    case 'l':
      OUT_NAMEMAX (pformat, prefix_len, SB_F_NAMEMAX (statfsbuf));
      break;
    case 't':
#if HAVE_STRUCT_STATXFS_F_TYPE
      out_uint_x (pformat, prefix_len, statfsbuf->f_type);
#else
      fputc ('?', stdout);
#endif
      break;
    case 'T':
      out_string (pformat, prefix_len, human_fstype (statfsbuf));
      break;
    case 'b':
      out_int (pformat, prefix_len, statfsbuf->f_blocks);
      break;
    case 'f':
      out_int (pformat, prefix_len, statfsbuf->f_bfree);
      break;
    case 'a':
      out_int (pformat, prefix_len, statfsbuf->f_bavail);
      break;
    case 's':
      out_uint (pformat, prefix_len, statfsbuf->f_bsize);
      break;
    case 'S':
      {
        uintmax_t frsize = STATFS_FRSIZE (statfsbuf);
        if (! frsize)
          frsize = statfsbuf->f_bsize;
        out_uint (pformat, prefix_len, frsize);
      }
      break;
    case 'c':
      out_uint (pformat, prefix_len, statfsbuf->f_files);
      break;
    case 'd':
      out_int (pformat, prefix_len, statfsbuf->f_ffree);
      break;
    case 'C':
      out_file_context (filename, pformat, prefix_len);
      break;
    default:
      fputc ('?', stdout);
      break;
    }
}

/* print stat info */
static void
print_stat (char *pformat, size_t prefix_len, char m,
            char const *filename, void const *data)
{
  struct stat *statbuf = (struct stat *) data;
  struct passwd *pw_ent;
  struct group *gw_ent;

  switch (m)
    {
    case 'n':
      out_string (pformat, prefix_len, filename);
      break;
    case 'N':
      out_string (pformat, prefix_len, quote (filename));
      if (S_ISLNK (statbuf->st_mode))
        {
          char *linkname = areadlink_with_size (filename, statbuf->st_size);
          if (linkname == NULL)
            {
              error (0, errno, _("cannot read symbolic link %s"),
                     quote (filename));
              return;
            }
          printf (" -> ");
          out_string (pformat, prefix_len, quote (linkname));
        }
      break;
    case 'd':
      out_uint (pformat, prefix_len, statbuf->st_dev);
      break;
    case 'D':
      out_uint_x (pformat, prefix_len, statbuf->st_dev);
      break;
    case 'i':
      out_uint (pformat, prefix_len, statbuf->st_ino);
      break;
    case 'a':
      out_uint_o (pformat, prefix_len, statbuf->st_mode & CHMOD_MODE_BITS);
      break;
    case 'A':
      out_string (pformat, prefix_len, human_access (statbuf));
      break;
    case 'f':
      out_uint_x (pformat, prefix_len, statbuf->st_mode);
      break;
    case 'F':
      out_string (pformat, prefix_len, file_type (statbuf));
      break;
    case 'h':
      out_uint (pformat, prefix_len, statbuf->st_nlink);
      break;
    case 'u':
      out_uint (pformat, prefix_len, statbuf->st_uid);
      break;
    case 'U':
      setpwent ();
      pw_ent = getpwuid (statbuf->st_uid);
      out_string (pformat, prefix_len,
                  pw_ent ? pw_ent->pw_name : "UNKNOWN");
      break;
    case 'g':
      out_uint (pformat, prefix_len, statbuf->st_gid);
      break;
    case 'G':
      setgrent ();
      gw_ent = getgrgid (statbuf->st_gid);
      out_string (pformat, prefix_len,
                  gw_ent ? gw_ent->gr_name : "UNKNOWN");
      break;
    case 't':
      out_uint_x (pformat, prefix_len, major (statbuf->st_rdev));
      break;
    case 'T':
      out_uint_x (pformat, prefix_len, minor (statbuf->st_rdev));
      break;
    case 's':
      out_uint (pformat, prefix_len, statbuf->st_size);
      break;
    case 'B':
      out_uint (pformat, prefix_len, ST_NBLOCKSIZE);
      break;
    case 'b':
      out_uint (pformat, prefix_len, ST_NBLOCKS (*statbuf));
      break;
    case 'o':
      out_uint (pformat, prefix_len, statbuf->st_blksize);
      break;
    case 'x':
      out_string (pformat, prefix_len, human_time (get_stat_atime (statbuf)));
      break;
    case 'X':
      if (TYPE_SIGNED (time_t))
        out_int (pformat, prefix_len, statbuf->st_atime);
      else
        out_uint (pformat, prefix_len, statbuf->st_atime);
      break;
    case 'y':
      out_string (pformat, prefix_len, human_time (get_stat_mtime (statbuf)));
      break;
    case 'Y':
      if (TYPE_SIGNED (time_t))
        out_int (pformat, prefix_len, statbuf->st_mtime);
      else
        out_uint (pformat, prefix_len, statbuf->st_mtime);
      break;
    case 'z':
      out_string (pformat, prefix_len, human_time (get_stat_ctime (statbuf)));
      break;
    case 'Z':
      if (TYPE_SIGNED (time_t))
        out_int (pformat, prefix_len, statbuf->st_ctime);
      else
        out_uint (pformat, prefix_len, statbuf->st_ctime);
      break;
    case 'C':
      out_file_context (filename, pformat, prefix_len);
      break;
    default:
      fputc ('?', stdout);
      break;
    }
}

/* Output a single-character \ escape.  */

static void
print_esc_char (char c)
{
  switch (c)
    {
    case 'a':			/* Alert. */
      c ='\a';
      break;
    case 'b':			/* Backspace. */
      c ='\b';
      break;
    case 'f':			/* Form feed. */
      c ='\f';
      break;
    case 'n':			/* New line. */
      c ='\n';
      break;
    case 'r':			/* Carriage return. */
      c ='\r';
      break;
    case 't':			/* Horizontal tab. */
      c ='\t';
      break;
    case 'v':			/* Vertical tab. */
      c ='\v';
      break;
    case '"':
    case '\\':
      break;
    default:
      error (0, 0, _("warning: unrecognized escape `\\%c'"), c);
      break;
    }
  putchar (c);
}

static void
print_it (char const *format, char const *filename,
          void (*print_func) (char *, size_t, char, char const *, void const *),
          void const *data)
{
  /* Add 2 to accommodate our conversion of the stat `%s' format string
     to the longer printf `%llu' one.  */
  enum
    {
      MAX_ADDITIONAL_BYTES =
        (MAX (sizeof PRIdMAX,
              MAX (sizeof PRIoMAX, MAX (sizeof PRIuMAX, sizeof PRIxMAX)))
         - 1)
    };
  size_t n_alloc = strlen (format) + MAX_ADDITIONAL_BYTES + 1;
  char *dest = xmalloc (n_alloc);
  char const *b;
  for (b = format; *b; b++)
    {
      switch (*b)
        {
        case '%':
          {
            size_t len = strspn (b + 1, "#-+.I 0123456789");
            char const *fmt_char = b + len + 1;
            memcpy (dest, b, len + 1);

            b = fmt_char;
            switch (*fmt_char)
              {
              case '\0':
                --b;
                /* fall through */
              case '%':
                if (0 < len)
                  {
                    dest[len + 1] = *fmt_char;
                    dest[len + 2] = '\0';
                    error (EXIT_FAILURE, 0, _("%s: invalid directive"),
                           quotearg_colon (dest));
                  }
                putchar ('%');
                break;
              default:
                print_func (dest, len + 1, *fmt_char, filename, data);
                break;
              }
            break;
          }

        case '\\':
          if ( ! interpret_backslash_escapes)
            {
              putchar ('\\');
              break;
            }
          ++b;
          if (isodigit (*b))
            {
              int esc_value = octtobin (*b);
              int esc_length = 1;	/* number of octal digits */
              for (++b; esc_length < 3 && isodigit (*b);
                   ++esc_length, ++b)
                {
                  esc_value = esc_value * 8 + octtobin (*b);
                }
              putchar (esc_value);
              --b;
            }
          else if (*b == 'x' && isxdigit (to_uchar (b[1])))
            {
              int esc_value = hextobin (b[1]);	/* Value of \xhh escape. */
              /* A hexadecimal \xhh escape sequence must have
                 1 or 2 hex. digits.  */
              ++b;
              if (isxdigit (to_uchar (b[1])))
                {
                  ++b;
                  esc_value = esc_value * 16 + hextobin (*b);
                }
              putchar (esc_value);
            }
          else if (*b == '\0')
            {
              error (0, 0, _("warning: backslash at end of format"));
              putchar ('\\');
              /* Arrange to exit the loop.  */
              --b;
            }
          else
            {
              print_esc_char (*b);
            }
          break;

        default:
          putchar (*b);
          break;
        }
    }
  free (dest);

  fputs (trailing_delim, stdout);
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
do_stat (char const *filename, bool terse, char const *format)
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
      printf (_("Usage: %s [OPTION]... FILE...\n"), program_name);
      fputs (_("\
Display file or file system status.\n\
\n\
  -L, --dereference     follow links\n\
  -f, --file-system     display file system status instead of file status\n\
"), stdout);
      fputs (_("\
  -c  --format=FORMAT   use the specified FORMAT instead of the default;\n\
                          output a newline after each use of FORMAT\n\
      --printf=FORMAT   like --format, but interpret backslash escapes,\n\
                          and do not output a mandatory trailing newline.\n\
                          If you want a newline, include \\n in FORMAT.\n\
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
  %C   SELinux security context string\n\
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
  %C   SELinux security context string\n\
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
      emit_bug_reporting_address ();
    }
  exit (status);
}

int
main (int argc, char *argv[])
{
  int c;
  int i;
  bool fs = false;
  bool terse = false;
  char *format = NULL;
  bool ok = true;

  initialize_main (&argc, &argv);
  set_program_name (argv[0]);
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  while ((c = getopt_long (argc, argv, "c:fLtZ", long_options, NULL)) != -1)
    {
      switch (c)
        {
        case PRINTF_OPTION:
          format = optarg;
          interpret_backslash_escapes = true;
          trailing_delim = "";
          break;

        case 'c':
          format = optarg;
          interpret_backslash_escapes = false;
          trailing_delim = "\n";
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

        case 'Z':  /* FIXME: remove in 2010 */
          /* Ignore, for compatibility with distributions
             that implemented this before upstream.
             But warn of impending removal.  */
          error (0, 0,
                 _("the --context (-Z) option is obsolete and will be removed\n"
                   "in a future release"));
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
           : do_stat (argv[i], terse, format));

  exit (ok ? EXIT_SUCCESS : EXIT_FAILURE);
}
