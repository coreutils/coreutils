#include <config.h>

#include <stdio.h>
#include <sys/types.h>
#ifdef HAVE_SYS_SYSMACROS_H
# include <sys/sysmacros.h>
#endif
#include <pwd.h>
#include <grp.h>
#include <unistd.h>
#include <time.h>
#if HAVE_SYS_VFS_H
# include <sys/vfs.h>
#endif
#include <string.h>
#include <malloc.h>
#include <ctype.h>

#include "system.h"

#include "closeout.h"
#include "error.h"
#include "filemode.h"
#include "fs.h"
#include "getopt.h"
#include "quotearg.h"
#include "xreadlink.h"

#ifdef FLASK_LINUX
# include <selinux/fs_secure.h>
# include <linux/flask/security.h>
# include <selinux/flask_util.h>           /* for is_flask_enabled() */
# define SECURITY_ID_T security_id_t
#else
# define SECURITY_ID_T int
# define is_flask_enabled() 0
# define stat_secure(a,b,c) stat(a,b)
# define lstat_secure(a,b,c) lstat(a,b)
#endif

#define PROGRAM_NAME "stat"

#define AUTHORS "Michael Meskes"

static struct option const long_options[] = {
  {"link", no_argument, 0, 'l'},
  {"format", required_argument, 0, 'c'},
  {"filesystem", no_argument, 0, 'f'},
  {"secure", no_argument, 0, 's'},
  {"terse", no_argument, 0, 't'},
  {GETOPT_HELP_OPTION_DECL},
  {GETOPT_VERSION_OPTION_DECL},
  {NULL, 0, NULL, 0}
};

static char *program_name;

static void
print_human_type (mode_t mode)
{
  char const *type;
  switch (mode & S_IFMT)
    {
    case S_IFDIR:
      type = "Directory";
      break;
    case S_IFCHR:
      type = "Character Device";
      break;
    case S_IFBLK:
      type = "Block Device";
      break;
    case S_IFREG:
      type = "Regular File";
      break;
    case S_IFLNK:
      type = "Symbolic Link";
      break;
    case S_IFSOCK:
      type = "Socket";
      break;
    case S_IFIFO:
      type = "Fifo File";
      break;
    default:
      type = "Unknown";
    }
  fputs (type, stdout);
}

static void
print_human_fstype (struct statfs const *statfsbuf)
{
  char const *type;

  switch (statfsbuf->f_type)
    {
#if defined (__linux__)
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
#elif __GNU__
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
#endif
    default:
      type = NULL;
      break;
    }

  if (type)
    fputs (type, stdout);
  else
    printf ("UNKNOWN (0x%x)\n", statfsbuf->f_type);
}

static void
print_human_access (struct stat const *statbuf)
{
  char modebuf[11];
  mode_string (statbuf->st_mode, modebuf);
  modebuf[10] = 0;
  fputs (modebuf, stdout);
}

static void
print_human_time (time_t const *t)
{
  char str[40];

  if (strftime (str, 40, "%c", localtime (t)) > 0) fputs (str, stdout);
  else printf ("Cannot calculate human readable time, sorry");
}

/* print statfs info */
static void
print_statfs (char *pformat, char m, char const *filename, void const *data, SECURITY_ID_T sid)
{
    struct statfs const *statfsbuf = data;
#ifdef FLASK_LINUX
    char sbuf[256];
    int rv;
    unsigned int sbuflen = sizeof(sbuf);
#endif

    switch(m) {
	case 'n':
	    strcat(pformat, "s");
	    printf(pformat, filename);
	    break;

	case 'i':
#if !defined(__linux__) && defined (__GNU__)
	    strcat(pformat, "Lx");
	    printf(pformat, statfsbuf->f_fsid);
#else
	    strcat(pformat, "x %-8x");
	    printf(pformat, statfsbuf->f_fsid.__val[0], statfsbuf->f_fsid.__val[1]);
#endif
	    break;

	case 'l':
	    strcat(pformat, "llu");
	    printf(pformat, (uintmax_t) (statfsbuf->f_namelen));
	    break;
	case 't':
	    strcat(pformat, "lx");
	    printf(pformat, (long int) (statfsbuf->f_type));
	    break;
	case 'T':
/*	    print_human_fstype(statfsbuf, pformat);*/
	    print_human_fstype(statfsbuf);
	    break;
	case 'b':
	    strcat(pformat, "lld");
	    printf(pformat, (intmax_t) (statfsbuf->f_blocks));
	    break;
	case 'f':
	    strcat(pformat, "lld");
	    printf(pformat, (intmax_t) (statfsbuf->f_bfree));
	    break;
	case 'a':
	    strcat(pformat, "lld");
	    printf(pformat, (intmax_t) (statfsbuf->f_bavail));
	    break;
	case 's':
	    strcat(pformat, "ld");
	    printf(pformat, (long int) (statfsbuf->f_bsize));
	    break;
	case 'c':
	    strcat(pformat, "lld");
	    printf(pformat, (intmax_t) (statfsbuf->f_files));
	    break;
	case 'd':
	    strcat(pformat, "lld");
	    printf(pformat, (intmax_t) (statfsbuf->f_ffree));
	    break;
#ifdef FLASK_LINUX
	case 'S':
	    strcat(pformat, "d");
	    printf(pformat, sid);
	    break;
	case 'C':
	    rv = security_sid_to_context(sid, (security_context_t *) &sbuf, &sbuflen);
	    if ( rv < 0 )
	    	sprintf(sbuf, "<error finding security context %d>", sid);
	    printf(sbuf);
	    break;
#endif
	default:
	    strcat(pformat, "c");
	    printf(pformat, m);
	    break;
    }
}

/* print stat info */
static void
print_stat (char *pformat, char m, char const *filename, void const *data, SECURITY_ID_T sid)
{
    char linkname[256];
    int i;
    struct stat *statbuf = (struct stat*)data;
    struct passwd *pw_ent;
    struct group *gw_ent;
#ifdef FLASK_LINUX
    char sbuf[256];
    int rv;
    unsigned int sbuflen = sizeof(sbuf);
#endif

    switch(m) {
	case 'n':
	    strcat(pformat, "s");
	    printf(pformat, filename);
	    break;
	case 'N':
	    strcat(pformat, "s");
	    if ((statbuf->st_mode & S_IFMT) == S_IFLNK) {
		if ((i = readlink(filename, linkname, 256)) == -1) {
		    perror(filename);
		    return;
		}
		linkname[(i >= 256) ? 255 : i] = '\0';
		/*printf("\"%s\" -> \"%s\"", filename, linkname);*/
		printf("\"");
		printf(pformat, filename);
		printf("\" -> \"");
		printf(pformat, linkname);
		printf("\"");
	    } else {
		printf("\"");
		printf(pformat, filename);
		printf("\"");
	    }
	    break;
	case 'd':
	    strcat(pformat, "d");
	    printf(pformat, (int)statbuf->st_dev);
	    break;
	case 'D':
	    strcat(pformat, "x");
	    printf(pformat, (int)statbuf->st_dev);
	    break;
	case 'i':
	    strcat(pformat, "d");
	    printf(pformat, (int)statbuf->st_ino);
	    break;
	case 'a':
	    strcat(pformat, "o");
	    printf(pformat, statbuf->st_mode & 07777);
	    break;
	case 'A':
	    print_human_access(statbuf);
	    break;
	case 'f':
	    strcat(pformat, "x");
	    printf(pformat, statbuf->st_mode);
	    break;
	case 'F':
	    print_human_type(statbuf->st_mode);
	    break;
	case 'h':
	    strcat(pformat, "d");
	    printf(pformat, (int)statbuf->st_nlink);
	    break;
#ifdef FLASK_LINUX
	case 'S':
	    strcat(pformat, "d");
	    printf(pformat, sid);
	    break;
	case 'C':
	    rv = security_sid_to_context(sid, (security_context_t *) &sbuf, &sbuflen);
	    if ( rv < 0 )
	    	sprintf(sbuf, "<error finding security context %d>", sid);
	    printf(sbuf);
	    break;
#endif
	case 'u':
	    strcat(pformat, "d");
	    printf(pformat, statbuf->st_uid);
	    break;
	case 'U':
	    strcat(pformat, "s");
	    setpwent();
	    pw_ent = getpwuid(statbuf->st_uid);
	    printf(pformat,
		(pw_ent != 0L) ? pw_ent->pw_name : "UNKNOWN");
	    break;
	case 'g':
	    strcat(pformat, "d");
	    printf(pformat, statbuf->st_gid);
	    break;
	case 'G':
	    strcat(pformat, "s");
	    setgrent();
	    gw_ent = getgrgid(statbuf->st_gid);
	    printf(pformat,
		(gw_ent != 0L) ? gw_ent->gr_name : "UNKNOWN");
	    break;
	case 't':
	    strcat(pformat, "x");
	    printf(pformat, major(statbuf->st_rdev));
	    break;
	case 'T':
	    strcat(pformat, "x");
	    printf(pformat, minor(statbuf->st_rdev));
	    break;
	case 's':
#ifdef __USE_FILE_OFFSET64
	    strcat(pformat, "llu");
	    printf(pformat, (unsigned long long)statbuf->st_size);
#else
	    strcat(pformat, "u");
	    printf(pformat, (unsigned int)statbuf->st_size);
#endif
	    break;
	case 'b':
	    strcat(pformat, "u");
	    printf(pformat, (unsigned int)statbuf->st_blocks);
	    break;
	case 'o':
	    strcat(pformat, "d");
	    printf(pformat, (int)statbuf->st_blksize);
	    break;
	case 'x':
	    print_human_time(&(statbuf->st_atime));
	    break;
	case 'X':
	    strcat(pformat, "d");
	    printf(pformat, (int)statbuf->st_atime);
	    break;
	case 'y':
	    print_human_time(&(statbuf->st_mtime));
	    break;
	case 'Y':
	    strcat(pformat, "d");
	    printf(pformat, (int)statbuf->st_mtime);
	    break;
	case 'z':
	    print_human_time(&(statbuf->st_ctime));
	    break;
	case 'Z':
	    strcat(pformat, "d");
	    printf(pformat, (int)statbuf->st_ctime);
	    break;
	default:
	    strcat(pformat, "c");
	    printf(pformat, m);
	    break;
    }
}

static void
print_it (char const *masterformat, char const *filename,
	  void (*print_func) (char *, char, char const *, void const *, SECURITY_ID_T),
	  void const *data, SECURITY_ID_T sid)
{
    char *m, *b, *format;
    char pformat[65];

    /* create a working copy of the format string */
    format = strdup(masterformat);
    if (!format) {
	perror(filename);
	return;
    }

    b = format;
    while (b)
    {
	if ((m = strchr(b, (int)'%')) != NULL)
	{
	    strcpy (pformat, "%");
	    *m++ = '\0';
	    fputs (b, stdout);

	    /* copy all format specifiers to our format string */
	    while (isdigit(*m) || strchr("#0-+. I", *m))
	    {
	    	char copy[2]="a";

		*copy = *m;
		/* make sure the format specifier is not too long */
		if (strlen (pformat) > 63)
			fprintf(stderr, "Warning: Format specifier too long, truncating: %s\n", pformat);
		else
			strcat (pformat, copy);
	    	m++;
	    }

	    switch(*m) {
		case '\0':
		case '%':
		    fputs("%", stdout);
		    break;
		default:
		    print_func(pformat, *m, filename, data, sid);
		    break;
	    }
	    b = m + 1;
	}
	else
	{
	    fputs (b, stdout);
	    b = NULL;
	}
    }
    free(format);
    printf("\n");
}

/* stat the filesystem and print what we find */
static void
do_statfs (char const *filename, int terse, int secure, char const *format)
{
  struct statfs statfsbuf;
  SECURITY_ID_T sid = -1;
  int i;

#ifdef FLASK_LINUX
  if (secure)
    i = statfs_secure(filename, &statfsbuf, &sid);
  else
#endif
    i = statfs(filename, &statfsbuf);
  if (i == -1)
    {
      perror (filename);
      return;
    }

  if (format == NULL)
    {
	if (terse != 0)
	  {
		if (secure)
	      		format = "%n %i %l %t %b %f %a %s %c %d %S %C";
		else
		      	format = "%n %i %l %t %b %f %a %s %c %d";
	  }
	else
	  {
		if (secure)
		    format = "  File: \"%n\"\n"
	                 "    ID: %-8i Namelen: %-7l Type: %T\n"
	                 "Blocks: Total: %-10b Free: %-10f Available: %-10a Size: %s\n"
	                 "Inodes: Total: %-10c Free: %-10d\n"
	                 "   SID: %-14S  S_Context: %C\n";
		else
		    format = "  File: \"%n\"\n"
	                 "    ID: %-8i Namelen: %-7l Type: %T\n"
	                 "Blocks: Total: %-10b Free: %-10f Available: %-10a Size: %s\n"
	                 "Inodes: Total: %-10c Free: %-10d";
	  }
    }

    print_it (format, filename, print_statfs, &statfsbuf, sid);
}

/* stat the file and print what we find */
static void
do_stat (char const *filename, int follow_links, int terse, int secure, char const *format)
{
  struct stat statbuf;
  int i;
  SECURITY_ID_T sid = -1;

  if (secure)
    i = (follow_links == 1) ? stat_secure(filename, &statbuf, &sid) : lstat_secure(filename, &statbuf, &sid);
  else
    i = (follow_links == 1) ? stat(filename, &statbuf) : lstat(filename, &statbuf);

  if (i == -1)
    {
      perror (filename);
      return;
    }

   if (format == NULL)
    {
       if (terse != 0)
       {
	   if (secure)
		  format = "%n %s %b %f %u %g %D %i %h %t %T %X %Y %Z %o %S %C";
	   else
	          format = "%n %s %b %f %u %g %D %i %h %t %T %X %Y %Z %o";
       }
       else
       {
           /* tmp hack to match orignal output until conditional implemented */
          i = statbuf.st_mode & S_IFMT;
          if (i == S_IFCHR || i == S_IFBLK) {
          	if (secure)
               		format =
	                   "  File: %N\n"
        	           "  Size: %-10s\tBlocks: %-10b IO Block: %-6o %F\n"
                	   "Device: %Dh/%dd\tInode: %-10i  Links: %-5h"
	                   " Device type: %t,%T\n"
        	           "Access: (%04a/%10.10A)  Uid: (%5u/%8U)   Gid: (%5g/%8G)\n"
	                   "   SID: %-14S  S_Context: %C\n"
	                   "Access: %x\n"
        	           "Modify: %y\n"
	                   "Change: %z\n";
        	else
        		format =
	                   "  File: %N\n"
        	           "  Size: %-10s\tBlocks: %-10b IO Block: %-6o %F\n"
                	   "Device: %Dh/%dd\tInode: %-10i  Links: %-5h"
	                   " Device type: %t,%T\n"
	                   "Access: (%04a/%10.10A)  Uid: (%5u/%8U)   Gid: (%5g/%8G)\n"
	                   "Access: %x\n"
        	           "Modify: %y\n"
                	   "Change: %z\n";
           }
	   else
	   {
	   	if (secure)
	               format =
        	           "  File: %N\n"
                	   "  Size: %-10s\tBlocks: %-10b IO Block: %-6o %F\n"
			   "Device: %Dh/%dd\tInode: %-10i  Links: %-5h\n"
        	           "Access: (%04a/%10.10A)  Uid: (%5u/%8U)   Gid: (%5g/%8G)\n"
                	   "   SID: %-14S  S_Context: %C\n"
	                   "Access: %x\n"
        	           "Modify: %y\n"
                	   "Change: %z\n";
                else
                      format =
	                   "  File: %N\n"
        	           "  Size: %-10s\tBlocks: %-10b IO Block: %-6o %F\n"
			   "Device: %Dh/%dd\tInode: %-10i  Links: %-5h\n"
	                   "Access: (%04a/%10.10A)  Uid: (%5u/%8U)   Gid: (%5g/%8G)\n"
        	           "Access: %x\n"
	                   "Modify: %y\n"
        	           "Change: %z\n";
          }
       }
    }
    print_it(format, filename, print_stat, &statbuf, sid);
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
  -f, --filesystem	display filesystem status instead of file status\n\
  -c  --format=FORMAT   FIXME\n\
  -l, --link		follow links\n\
  -s, --secure	        FIXME\n\
  -t, --terse		print the information in terse form\n\
"), stdout);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);

      fputs (_("\n\
The valid format sequences for files (without --filesystem):\n\
\n\
  %A - Access rights in human readable form\n\
  %a - Access rights in octal\n\
  %b - Number of blocks allocated\n\
  %C - Security context in SE-Linux\n\
"), stdout);
      fputs (_("\
  %D - Device number in hex\n\
  %d - Device number in decimal\n\
  %F - File type\n\
  %f - raw mode in hex\n\
  %G - Group name of owner\n\
  %g - Group ID of owner\n\
"), stdout);
      fputs (_("\
  %h - Number of hard links\n\
  %i - Inode number\n\
  %N - Quoted File name with dereference if symbolic link\n\
  %n - File name\n\
  %o - IO block size\n\
  %S - Security ID in SE-Linux\n\
  %s - Total size, in bytes\n\
  %T - Minor device type in hex\n\
  %t - Major device type in hex\n\
"), stdout);
      fputs (_("\
  %U - User name of owner\n\
  %u - User ID of owner\n\
  %X - Time of last access as seconds since Epoch\n\
  %x - Time of last access\n\
  %Y - Time of last modification as seconds since Epoch\n\
  %y - Time of last modification\n\
  %Z - Time of last change as seconds since Epoch\n\
  %z - Time of last change\n\
\n\
"), stdout);

      fputs (_("\
Valid format sequences for file systems:\n\
\n\
  %a - Free blocks available to non-superuser\n\
  %b - Total data blocks in file system\n\
  %C - Security context in SE-Linux\n\
  %c - Total file nodes in file system\n\
  %d - Free file nodes in file system\n\
  %f - Free blocks in file system\n\
"), stdout);
      fputs (_("\
  %i - File System id in hex\n\
  %l - Maximum length of filenames\n\
  %n - File name\n\
  %S - Security ID in SE-Linux\n\
  %s - Optimal transfer block size\n\
  %T - Type in human readable form\n\
  %t - Type in hex\n\
"), stdout);
      puts (_("\nReport bugs to <bug-fileutils@gnu.org>."));
    }
  exit (status);
}

static void
verbose_usage(char *progname)
{
    fprintf(stderr, "Usage: %s [-l] [-f] [-s] [-v] [-h] [-t] [-c format] file1 [file2 ...]\n", progname);
    fprintf(stderr, "\tformat interpreted sequences for file stat are:\n");
    fprintf(stderr, "\t\t%%n - File name\n");
    fprintf(stderr, "\t\t%%N - Quoted File name with dereference if symbolic link\n");
    fprintf(stderr, "\t\t%%d - Device number in decimal\n");
    fprintf(stderr, "\t\t%%D - Device number in hex\n");
    fprintf(stderr, "\t\t%%i - Inode number\n");
    fprintf(stderr, "\t\t%%a - Access rights in octal\n");
    fprintf(stderr, "\t\t%%A - Access rights in human readable form\n");
    fprintf(stderr, "\t\t%%f - raw mode in hex\n");
    fprintf(stderr, "\t\t%%F - File type\n");
    fprintf(stderr, "\t\t%%h - Number of hard links\n");
    fprintf(stderr, "\t\t%%u - User Id of owner\n");
    fprintf(stderr, "\t\t%%U - User name of owner\n");
    fprintf(stderr, "\t\t%%g - Group Id of owner\n");
    fprintf(stderr, "\t\t%%G - Group name of owner\n");
    fprintf(stderr, "\t\t%%t - Major device type in hex\n");
    fprintf(stderr, "\t\t%%T - Minor device type in hex\n");
    fprintf(stderr, "\t\t%%s - Total size, in bytes\n");
    fprintf(stderr, "\t\t%%b - Number of blocks allocated\n");
    fprintf(stderr, "\t\t%%o - IO block size\n");
    fprintf(stderr, "\t\t%%x - Time of last access\n");
    fprintf(stderr, "\t\t%%X - Time of last access as seconds since Epoch\n");
    fprintf(stderr, "\t\t%%y - Time of last modification\n");
    fprintf(stderr, "\t\t%%Y - Time of last modification as seconds since Epoch\n");
    fprintf(stderr, "\t\t%%z - Time of last change\n");
    fprintf(stderr, "\t\t%%Z - Time of last change as seconds since Epoch\n");
    fprintf(stderr, "\t\t%%S - Security ID in SE-Linux\n");
    fprintf(stderr, "\t\t%%C - Security context in SE-Linux\n");
    fprintf(stderr, "\tformat interpreted sequences for filesystem stat are:\n");
    fprintf(stderr, "\t\t%%n - File name\n");
    fprintf(stderr, "\t\t%%i - File System id in hex\n");
    fprintf(stderr, "\t\t%%l - Maximum length of filenames\n");
    fprintf(stderr, "\t\t%%t - Type in hex\n");
    fprintf(stderr, "\t\t%%T - Type in human readable form\n");
    fprintf(stderr, "\t\t%%b - Total data blocks in file system\n");
    fprintf(stderr, "\t\t%%f - Free blocks in file system\n");
    fprintf(stderr, "\t\t%%a - Free blocks available to non-superuser\n");
    fprintf(stderr, "\t\t%%s - Optimal transfer block size\n");
    fprintf(stderr, "\t\t%%c - Total file nodes in file system\n");
    fprintf(stderr, "\t\t%%S - Security ID in SE-Linux\n");
    fprintf(stderr, "\t\t%%C - Security context in SE-Linux\n");
    fprintf(stderr, "\t\t%%d - Free file nodes in file system\n");
    exit(1);
}

int
main (int argc, char *argv[])
{
  int c;
  int i;
  int follow_links = 0;
  int fs = 0;
  int terse = 0;
  int secure = 0;
  char *format = NULL;

  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  while ((c = getopt_long (argc, argv, "c:flst", long_options, NULL)) != -1)
    {
      switch (c)
	{
	case 'c':
	  format = optarg;
	  break;
	case 'l':
	  follow_links = 1;
	  break;
	case 'f':
	  fs = 1;
	  break;
	case 's':
	  secure = 1;
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

  if (!is_flask_enabled ())
    secure = 0;

  for (i = optind; i < argc; i++)
    {
      if (fs == 0)
	do_stat (argv[i], follow_links, terse, secure, format);
      else
	do_statfs (argv[i], terse, secure, format);
    }

  exit (EXIT_SUCCESS);
}
