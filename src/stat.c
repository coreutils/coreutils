#include <stdio.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <pwd.h>
#include <grp.h>
#include <unistd.h>
#include <time.h>
#include <sys/vfs.h>
#include <string.h>
#include <malloc.h>
#include <ctype.h>

#ifdef FLASK_LINUX
# include <selinux/fs_secure.h>
# include <linux/flask/security.h>
# include <selinux/flask_util.h>           /* for is_flask_enabled() */
# define SECURITY_ID_T security_id_t
#else
# define SECURITY_ID_T int
#endif

#include "fs.h"

void print_human_type(unsigned short mode)
{
  switch (mode & S_IFMT)
    {
    case S_IFDIR:
      printf ("Directory");
      break;
    case S_IFCHR:
      printf ("Character Device");
      break;
    case S_IFBLK:
      printf ("Block Device");
      break;
    case S_IFREG:
      printf ("Regular File");
      break;
    case S_IFLNK:
      printf ("Symbolic Link");
      break;
    case S_IFSOCK:
      printf ("Socket");
      break;
    case S_IFIFO:
      printf ("Fifo File");
      break;
    default:
      printf ("Unknown");
    }
}

void print_human_fstype(struct statfs *statfsbuf)
{
  char *type;

  switch (statfsbuf->f_type)
#if defined (__linux__)
  {
    	case S_MAGIC_AFFS:
	  type = strdup("affs");
	  break;
	case S_MAGIC_EXT:
	  type = strdup("ext");
	  break;
	case S_MAGIC_EXT2_OLD:
	  type = strdup("ext2");
	  break;
	case S_MAGIC_EXT2:
	  type = strdup("ext2/ext3");
	  break;
	case S_MAGIC_HPFS:
	  type = strdup("hpfs");
	  break;
	case S_MAGIC_ISOFS:
	  type = strdup("isofs");
	  break;
	case S_MAGIC_ISOFS_WIN:
	  type = strdup("isofs");
	  break;
	case S_MAGIC_ISOFS_R_WIN:
	  type = strdup("isofs");
	  break;
	case S_MAGIC_MINIX:
	  type = strdup("minix");
	case S_MAGIC_MINIX_30:
	  type = strdup("minix (30 char.)");
	  break;
	case S_MAGIC_MINIX_V2:
	  type = strdup("minix v2");
	  break;
	case S_MAGIC_MINIX_V2_30:
	  type = strdup("minix v2 (30 char.)");
	  break;
	case S_MAGIC_MSDOS:
	  type = strdup("msdos");
	  break;
	case S_MAGIC_FAT:
	  type = strdup("fat");
	  break;
	case S_MAGIC_NCP:
	  type = strdup("novell");
	  break;
	case S_MAGIC_NFS:
	  type = strdup("nfs");
	  break;
	case S_MAGIC_PROC:
	  type = strdup("proc");
	  break;
	case S_MAGIC_SMB:
	  type = strdup("smb");
	  break;
	case S_MAGIC_XENIX:
	  type = strdup("xenix");
	  break;
	case S_MAGIC_SYSV4:
	  type = strdup("sysv4");
	  break;
	case S_MAGIC_SYSV2:
	  type = strdup("sysv2");
	  break;
	case S_MAGIC_COH:
	  type = strdup("coh");
	  break;
	case S_MAGIC_UFS:
	  type = strdup("ufs");
	  break;
	case S_MAGIC_XIAFS:
	  type = strdup("xia");
	  break;
	case S_MAGIC_NTFS:
	  type = strdup("ntfs");
	  break;
	case S_MAGIC_TMPFS:
	  type = strdup("tmpfs");
	  break;
	case S_MAGIC_REISERFS:
	  type = strdup("reiserfs");
	  break;
	case S_MAGIC_CRAMFS:
	  type = strdup("cramfs");
	  break;
	case S_MAGIC_ROMFS:
	  type = strdup("romfs");
	  break;
#elif __GNU__
        case FSTYPE_UFS:
          type = strdup("ufs");
          break;
        case FSTYPE_NFS:
          type = strdup("nfs");
          break;
        case FSTYPE_GFS:
          type = strdup("gfs");
          break;
        case FSTYPE_LFS:
          type = strdup("lfs");
          break;
        case FSTYPE_SYSV:
          type = strdup("sysv");
          break;
        case FSTYPE_FTP:
          type = strdup("ftp");
          break;
        case FSTYPE_TAR:
          type = strdup("tar");
          break;
        case FSTYPE_AR:
          type = strdup("ar");
          break;
        case FSTYPE_CPIO:
          type = strdup("cpio");
          break;
        case FSTYPE_MSLOSS:
          type = strdup("msloss");
          break;
        case FSTYPE_CPM:
          type = strdup("cpm");
          break;
        case FSTYPE_HFS:
          type = strdup("hfs");
          break;
        case FSTYPE_DTFS:
          type = strdup("dtfs");
          break;
        case FSTYPE_GRFS:
          type = strdup("grfs");
          break;
        case FSTYPE_TERM:
          type = strdup("term");
          break;
        case FSTYPE_DEV:
          type = strdup("dev");
          break;
        case FSTYPE_PROC:
          type = strdup("proc");
          break;
        case FSTYPE_IFSOCK:
          type = strdup("ifsock");
          break;
        case FSTYPE_AFS:
          type = strdup("afs");
          break;
        case FSTYPE_DFS:
          type = strdup("dfs");
          break;
        case FSTYPE_PROC9:
          type = strdup("proc9");
          break;
        case FSTYPE_SOCKET:
          type = strdup("socket");
          break;
        case FSTYPE_MISC:
          type = strdup("misc");
          break;
        case FSTYPE_EXT2FS:
          type = strdup("ext2/ext3");
          break;
        case FSTYPE_HTTP:
          type = strdup("http");
          break;
        case FSTYPE_MEMFS:
          type = strdup("memfs");
          break;
        case FSTYPE_ISO9660:
          type = strdup("iso9660");
          break;
#endif
	default:
	  if ((type = (char*)malloc(30 * sizeof(char))) == NULL) {
		perror("malloc error");
		return;
          }
#ifdef __USE_FILE_OFFSET64
	  sprintf (type, "UNKNOWN (0x%x)\n", statfsbuf->f_type);
#else
	  sprintf (type, "UNKNOWN (0x%x)\n", statfsbuf->f_type);
#endif
  }

  printf("%s", type);
  free(type);
}

void print_human_access(struct stat *statbuf)
{
  char access[10];

  access[9] = (statbuf->st_mode & S_IXOTH) ?
    ((statbuf->st_mode & S_ISVTX) ? 't' : 'x') :
    ((statbuf->st_mode & S_ISVTX) ? 'T' : '-');
  access[8] = (statbuf->st_mode & S_IWOTH) ? 'w' : '-';
  access[7] = (statbuf->st_mode & S_IROTH) ? 'r' : '-';
  access[6] = (statbuf->st_mode & S_IXGRP) ?
    ((statbuf->st_mode & S_ISGID) ? 's' : 'x') :
    ((statbuf->st_mode & S_ISGID) ? 'S' : '-');
  access[5] = (statbuf->st_mode & S_IWGRP) ? 'w' : '-';
  access[4] = (statbuf->st_mode & S_IRGRP) ? 'r' : '-';
  access[3] = (statbuf->st_mode & S_IXUSR) ?
    ((statbuf->st_mode & S_ISUID) ? 's' : 'x') :
    ((statbuf->st_mode & S_ISUID) ? 'S' : '-');
  access[2] = (statbuf->st_mode & S_IWUSR) ? 'w' : '-';
  access[1] = (statbuf->st_mode & S_IRUSR) ? 'r' : '-';

  switch (statbuf->st_mode & S_IFMT)
    {
    case S_IFDIR:
      access[0] = 'd';
      break;
    case S_IFCHR:
      access[0] = 'c';
      break;
    case S_IFBLK:
      access[0] = 'b';
      break;
    case S_IFREG:
      access[0] = '-';
      break;
    case S_IFLNK:
      access[0] = 'l';
      break;
    case S_IFSOCK:
      access[0] = 's';
      break;
    case S_IFIFO:
      access[0] = 'p';
      break;
    default:
      access[0] = '?';
    }
    printf (access);
}

void print_human_time(time_t *t)
{
  char str[40];

  if (strftime(str, 40, "%c", localtime(t)) > 0) printf(str);
  else printf("Cannot calculate human readable time, sorry");
}

/* print statfs info */
void print_statfs(char *pformat, char m, char *filename, void *data, SECURITY_ID_T sid)
{
    struct statfs *statfsbuf = (struct statfs*)data;
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
#if !defined(__linux__) && defined (__GNU__)
	case 'i':
	    strcat(pformat, "Lx");
	    printf(pformat, statfsbuf->f_fsid);
	    break;
#else
	case 'i':
	    strcat(pformat, "x %-8x");
	    printf(pformat, statfsbuf->f_fsid.__val[0], statfsbuf->f_fsid.__val[1]);
	    break;
#endif

	case 'l':
#ifdef __USE_FILE_OFFSET64
	    strcat(pformat, "lu");
#else
	    strcat(pformat, "d");
#endif
	    printf(pformat, statfsbuf->f_namelen);
	    break;
	case 't':
#ifdef __USE_FILE_OFFSET64
	    strcat(pformat, "lx");
#else
	    strcat(pformat, "x");
#endif
	    printf(pformat, statfsbuf->f_type);
	    break;
	case 'T':
/*	    print_human_fstype(statfsbuf, pformat);*/
	    print_human_fstype(statfsbuf);
	    break;
	case 'b':
#ifdef __USE_FILE_OFFSET64
	    strcat(pformat, "lld");
#else
# if !defined(__linux__) && defined (__GNU__)
	    strcat(pformat, "d");
# else
	    strcat(pformat, "ld");
# endif
#endif
	    printf(pformat, statfsbuf->f_blocks);
	    break;
	case 'f':
#ifdef __USE_FILE_OFFSET64
	    strcat(pformat, "lld");
#else
# if !defined(__linux__) && defined (__GNU__)
	    strcat(pformat, "d");
# else
	    strcat(pformat, "ld");
# endif
#endif
	    printf(pformat, statfsbuf->f_bfree);
	    break;
	case 'a':
#ifdef __USE_FILE_OFFSET64
	    strcat(pformat, "lld");
#else
# if !defined(__linux__) && defined (__GNU__)
	    strcat(pformat, "d");
# else
	    strcat(pformat, "ld");
# endif
#endif
	    printf(pformat, statfsbuf->f_bavail);
	    break;
	case 's':
#ifdef __USE_FILE_OFFSET64
	    strcat(pformat, "ld");
#else
	    strcat(pformat, "d");
#endif
	    printf(pformat, statfsbuf->f_bsize);
	    break;
	case 'c':
#ifdef __USE_FILE_OFFSET64
	    strcat(pformat, "lld");
#else
# if !defined(__linux__) && defined (__GNU__)
	    strcat(pformat, "d");
# else
	    strcat(pformat, "ld");
# endif
#endif
	    printf(pformat, statfsbuf->f_files);
	    break;
	case 'd':
#ifdef __USE_FILE_OFFSET64
	    strcat(pformat, "lld");
#else
# if !defined(__linux__) && defined (__GNU__)
	    strcat(pformat, "d");
# else
	    strcat(pformat, "ld");
# endif
#endif
	    printf(pformat, statfsbuf->f_ffree);
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
void print_stat(char *pformat, char m, char *filename, void *data, SECURITY_ID_T sid)
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

void print_it(char *masterformat, char *filename,
    void (*print_func)(char*, char, char*, void*, SECURITY_ID_T), void *data, SECURITY_ID_T sid)
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
	    printf(b);

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
		    printf("%%");
		    break;
		default:
		    print_func(pformat, *m, filename, data, sid);
		    break;
	    }
	    b = m + 1;
	}
	else
	{
	    printf(b);
	    b = NULL;
	}
    }
    free(format);
    printf("\n");
}

/* stat the filesystem and print what we find */
void
do_statfs (char *filename, int terse, int secure, char *format)
{
  struct statfs statfsbuf;
  SECURITY_ID_T sid = -1;
  int i;

#ifdef FLASK_LINUX
  if(!is_flask_enabled())
    secure = 0;
  if(secure)
    i = statfs_secure(filename, &statfsbuf, &sid);
  else
#endif
    i = statfs(filename, &statfsbuf);
  if(i == -1)
    {
      perror (filename);
      return;
    }

  if (format == NULL)
    {
	if (terse != 0)
	  {
#ifdef FLASK_LINUX
		if(secure)
	      		format = "%n %i %l %t %b %f %a %s %c %d %S %C";
		else
#endif
		      	format = "%n %i %l %t %b %f %a %s %c %d";
	  }
	else
	  {
#ifdef FLASK_LINUX
		if(secure)
		    format = "  File: \"%n\"\n"
	                 "    ID: %-8i Namelen: %-7l Type: %T\n"
	                 "Blocks: Total: %-10b Free: %-10f Available: %-10a Size: %s\n"
	                 "Inodes: Total: %-10c Free: %-10d\n"
	                 "   SID: %-14S  S_Context: %C\n";
		else
#endif
		    format = "  File: \"%n\"\n"
	                 "    ID: %-8i Namelen: %-7l Type: %T\n"
	                 "Blocks: Total: %-10b Free: %-10f Available: %-10a Size: %s\n"
	                 "Inodes: Total: %-10c Free: %-10d";
	  }
    }

    print_it(format, filename, print_statfs, &statfsbuf, sid);
}

/* stat the file and print what we find */
void
do_stat (char *filename, int link, int terse, int secure, char *format)
{
  struct stat statbuf;
  int i;
  SECURITY_ID_T sid = -1;

#ifdef FLASK_LINUX
  if(!is_flask_enabled())
    secure = 0;
  if(secure)
    i = (link == 1) ? stat_secure(filename, &statbuf, &sid) : lstat_secure(filename, &statbuf, &sid);
  else
#endif
  i = (link == 1) ? stat(filename, &statbuf) : lstat(filename, &statbuf);

  if (i == -1)
    {
      perror (filename);
      return;
    }

   if (format == NULL)
    {
       if (terse != 0)
       {
#ifdef FLASK_LINUX
	   if (secure)
		  format = "%n %s %b %f %u %g %D %i %h %t %T %X %Y %Z %o %S %C";
	   else
#endif
	          format = "%n %s %b %f %u %g %D %i %h %t %T %X %Y %Z %o";
       }
       else
       {
           /* tmp hack to match orignal output until conditional implemented */
          i = statbuf.st_mode & S_IFMT;
          if (i == S_IFCHR || i == S_IFBLK) {
#ifdef FLASK_LINUX
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
#endif
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
#ifdef FLASK_LINUX
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
#endif
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
usage (char *progname)
{
  fprintf (stderr, "Usage: %s [-l] [-f] [-s] [-v] [-h] [-t] [-c format] file1 [file2 ...]\n", progname);
  exit (1);
}

void verbose_usage(char *progname)
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
  int c, i, link = 0, fs = 0, terse = 0, secure = 0;
  char *format = NULL;

  while ((c = getopt (argc, argv, "lsfvthc:")) != EOF)
    {
      switch (c)
	{
	case 'l':
	  link = 1;
	  break;
	case 's':
	  secure = 1;
	  break;
	case 'f':
	  fs = 1;
	  break;
	case 't':
	  terse = 1;
	  break;
	case 'c':
	  format = optarg;
	  break;
	case 'h':
	  printf ("stat version: 3.0\n");
	  verbose_usage(argv[0]);
	  break;
	case 'v':
          printf ("stat version: 3.0\n");
	default:
	  usage (argv[0]);
	}
    }
  if (argc == optind)
    usage (argv[0]);

  for (i = optind; i < argc; i++)
    (fs == 0) ? do_stat (argv[i], link, terse, secure, format) :
     		do_statfs (argv[i], terse, secure, format);

  return (0);
}
