/* mountlist.c -- return a list of mounted filesystems
   Copyright (C) 1991, 1992, 1997-2003 Free Software Foundation, Inc.

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

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <sys/types.h>


#ifdef STDC_HEADERS
# include <stdlib.h>
#else
void free ();
#endif
#if defined STDC_HEADERS || defined HAVE_STRING_H
# include <string.h>
#else
# include <strings.h>
#endif
#include "xalloc.h"

#ifndef strstr
char *strstr ();
#endif

#include <errno.h>
#ifndef errno
extern int errno;
#endif

#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#if HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif

#if defined MOUNTED_GETFSSTAT	/* OSF_1 and Darwin1.3.x */
# if HAVE_SYS_UCRED_H
#  include <sys/ucred.h> /* needed by powerpc-apple-darwin1.3.7 */
# endif
# if HAVE_SYS_MOUNT_H
#  include <sys/mount.h>
# endif
# if HAVE_SYS_FS_TYPES_H
#  include <sys/fs_types.h> /* needed by powerpc-apple-darwin1.3.7 */
# endif
# if HAVE_STRUCT_FSSTAT_F_FSTYPENAME
#  define FS_TYPE(Ent) ((Ent).f_fstypename)
# else
#  define FS_TYPE(Ent) mnt_names[(Ent).f_type]
# endif
#endif /* MOUNTED_GETFSSTAT */

#ifdef MOUNTED_GETMNTENT1	/* 4.3BSD, SunOS, HP-UX, Dynix, Irix.  */
# include <mntent.h>
# if !defined MOUNTED
#  if defined _PATH_MOUNTED	/* GNU libc  */
#   define MOUNTED _PATH_MOUNTED
#  endif
#  if defined MNT_MNTTAB	/* HP-UX.  */
#   define MOUNTED MNT_MNTTAB
#  endif
#  if defined MNTTABNAME	/* Dynix.  */
#   define MOUNTED MNTTABNAME
#  endif
# endif
#endif

#ifdef MOUNTED_GETMNTINFO	/* 4.4BSD.  */
# include <sys/mount.h>
#endif

#ifdef MOUNTED_GETMNT		/* Ultrix.  */
# include <sys/mount.h>
# include <sys/fs_types.h>
#endif

#ifdef MOUNTED_FS_STAT_DEV	/* BeOS.  */
# include <fs_info.h>
# include <dirent.h>
#endif

#ifdef MOUNTED_FREAD		/* SVR2.  */
# include <mnttab.h>
#endif

#ifdef MOUNTED_FREAD_FSTYP	/* SVR3.  */
# include <mnttab.h>
# include <sys/fstyp.h>
# include <sys/statfs.h>
#endif

#ifdef MOUNTED_LISTMNTENT
# include <mntent.h>
#endif

#ifdef MOUNTED_GETMNTENT2	/* SVR4.  */
# include <sys/mnttab.h>
#endif

#ifdef MOUNTED_VMOUNT		/* AIX.  */
# include <fshelp.h>
# include <sys/vfs.h>
#endif

#ifdef DOLPHIN
/* So special that it's not worth putting this in autoconf.  */
# undef MOUNTED_FREAD_FSTYP
# define MOUNTED_GETMNTTBL
#endif

#if HAVE_SYS_MNTENT_H
/* This is to get MNTOPT_IGNORE on e.g. SVR4.  */
# include <sys/mntent.h>
#endif

#undef MNT_IGNORE
#if defined MNTOPT_IGNORE && defined HAVE_HASMNTOPT
# define MNT_IGNORE(M) hasmntopt ((M), MNTOPT_IGNORE)
#else
# define MNT_IGNORE(M) 0
#endif

#include "mountlist.h"
#include "unlocked-io.h"

#ifdef MOUNTED_GETMNTENT1	/* 4.3BSD, SunOS, HP-UX, Dynix, Irix.  */
/* Return the value of the hexadecimal number represented by CP.
   No prefix (like '0x') or suffix (like 'h') is expected to be
   part of CP. */
/* FIXME: this can overflow */

static int
xatoi (char *cp)
{
  int val;

  val = 0;
  while (*cp)
    {
      if (*cp >= 'a' && *cp <= 'f')
	val = val * 16 + *cp - 'a' + 10;
      else if (*cp >= 'A' && *cp <= 'F')
	val = val * 16 + *cp - 'A' + 10;
      else if (*cp >= '0' && *cp <= '9')
	val = val * 16 + *cp - '0';
      else
	break;
      cp++;
    }
  return val;
}
#endif /* MOUNTED_GETMNTENT1.  */

#if MOUNTED_GETMNTINFO

# if ! HAVE_F_FSTYPENAME_IN_STATFS
static char *
fstype_to_string (short t)
{
  switch (t)
    {
#  ifdef MOUNT_PC
    case MOUNT_PC:
      return "pc";
#  endif
#  ifdef MOUNT_MFS
    case MOUNT_MFS:
      return "mfs";
#  endif
#  ifdef MOUNT_LO
    case MOUNT_LO:
      return "lo";
#  endif
#  ifdef MOUNT_TFS
    case MOUNT_TFS:
      return "tfs";
#  endif
#  ifdef MOUNT_TMP
    case MOUNT_TMP:
      return "tmp";
#  endif
#  ifdef MOUNT_UFS
   case MOUNT_UFS:
     return "ufs" ;
#  endif
#  ifdef MOUNT_NFS
   case MOUNT_NFS:
     return "nfs" ;
#  endif
#  ifdef MOUNT_MSDOS
   case MOUNT_MSDOS:
     return "msdos" ;
#  endif
#  ifdef MOUNT_LFS
   case MOUNT_LFS:
     return "lfs" ;
#  endif
#  ifdef MOUNT_LOFS
   case MOUNT_LOFS:
     return "lofs" ;
#  endif
#  ifdef MOUNT_FDESC
   case MOUNT_FDESC:
     return "fdesc" ;
#  endif
#  ifdef MOUNT_PORTAL
   case MOUNT_PORTAL:
     return "portal" ;
#  endif
#  ifdef MOUNT_NULL
   case MOUNT_NULL:
     return "null" ;
#  endif
#  ifdef MOUNT_UMAP
   case MOUNT_UMAP:
     return "umap" ;
#  endif
#  ifdef MOUNT_KERNFS
   case MOUNT_KERNFS:
     return "kernfs" ;
#  endif
#  ifdef MOUNT_PROCFS
   case MOUNT_PROCFS:
     return "procfs" ;
#  endif
#  ifdef MOUNT_AFS
   case MOUNT_AFS:
     return "afs" ;
#  endif
#  ifdef MOUNT_CD9660
   case MOUNT_CD9660:
     return "cd9660" ;
#  endif
#  ifdef MOUNT_UNION
   case MOUNT_UNION:
     return "union" ;
#  endif
#  ifdef MOUNT_DEVFS
   case MOUNT_DEVFS:
     return "devfs" ;
#  endif
#  ifdef MOUNT_EXT2FS
   case MOUNT_EXT2FS:
     return "ext2fs" ;
#  endif
    default:
      return "?";
    }
}
# endif /* ! HAVE_F_FSTYPENAME_IN_STATFS */

/* __NetBSD__ || BSD_NET2 || __OpenBSD__ */
static char *
fsp_to_string (const struct statfs *fsp)
{
# if defined HAVE_F_FSTYPENAME_IN_STATFS
  return (char *) (fsp->f_fstypename);
# else
  return fstype_to_string (fsp->f_type);
# endif
}

#endif /* MOUNTED_GETMNTINFO */

#ifdef MOUNTED_VMOUNT		/* AIX.  */
static char *
fstype_to_string (int t)
{
  struct vfs_ent *e;

  e = getvfsbytype (t);
  if (!e || !e->vfsent_name)
    return "none";
  else
    return e->vfsent_name;
}
#endif /* MOUNTED_VMOUNT */

/* Return a list of the currently mounted filesystems, or NULL on error.
   Add each entry to the tail of the list so that they stay in order.
   If NEED_FS_TYPE is nonzero, ensure that the filesystem type fields in
   the returned list are valid.  Otherwise, they might not be.  */

struct mount_entry *
read_filesystem_list (int need_fs_type)
{
  struct mount_entry *mount_list;
  struct mount_entry *me;
  struct mount_entry **mtail = &mount_list;

#ifdef MOUNTED_LISTMNTENT
  {
    struct tabmntent *mntlist, *p;
    struct mntent *mnt;
    struct mount_entry *me;

    /* the third and fourth arguments could be used to filter mounts,
       but Crays doesn't seem to have any mounts that we want to
       remove. Specifically, automount create normal NFS mounts.
       */

    if(listmntent(&mntlist, KMTAB, NULL, NULL) < 0)
      return NULL;
    for (p = mntlist; p; p = p->next) {
      mnt = p->ment;
      me = (struct mount_entry*) xmalloc(sizeof (struct mount_entry));
      me->me_devname = xstrdup(mnt->mnt_fsname);
      me->me_mountdir = xstrdup(mnt->mnt_dir);
      me->me_type = xstrdup(mnt->mnt_type);
      me->me_dummy = ME_DUMMY (me->me_devname, me->me_type);
      me->me_remote = ME_REMOTE (me->me_devname, me->me_type);
      me->me_dev = -1;
      *mtail = me;
      mtail = &me->me_next;
    }
    freemntlist(mntlist);
  }
#endif

#ifdef MOUNTED_GETMNTENT1	/* 4.3BSD, SunOS, HP-UX, Dynix, Irix.  */
  {
    struct mntent *mnt;
    char *table = MOUNTED;
    FILE *fp;
    char *devopt;

    fp = setmntent (table, "r");
    if (fp == NULL)
      return NULL;

    while ((mnt = getmntent (fp)))
      {
	me = (struct mount_entry *) xmalloc (sizeof (struct mount_entry));
	me->me_devname = xstrdup (mnt->mnt_fsname);
	me->me_mountdir = xstrdup (mnt->mnt_dir);
	me->me_type = xstrdup (mnt->mnt_type);
	me->me_dummy = ME_DUMMY (me->me_devname, me->me_type);
	me->me_remote = ME_REMOTE (me->me_devname, me->me_type);
	devopt = strstr (mnt->mnt_opts, "dev=");
	if (devopt)
	  {
	    if (devopt[4] == '0' && (devopt[5] == 'x' || devopt[5] == 'X'))
	      me->me_dev = xatoi (devopt + 6);
	    else
	      me->me_dev = xatoi (devopt + 4);
	  }
	else
	  me->me_dev = (dev_t) -1;	/* Magic; means not known yet. */

	/* Add to the linked list. */
	*mtail = me;
	mtail = &me->me_next;
      }

    if (endmntent (fp) == 0)
      goto free_then_fail;
  }
#endif /* MOUNTED_GETMNTENT1. */

#ifdef MOUNTED_GETMNTINFO	/* 4.4BSD.  */
  {
    struct statfs *fsp;
    int entries;

    entries = getmntinfo (&fsp, MNT_NOWAIT);
    if (entries < 0)
      return NULL;
    for (; entries-- > 0; fsp++)
      {
	char *fs_type = fsp_to_string (fsp);

	me = (struct mount_entry *) xmalloc (sizeof (struct mount_entry));
	me->me_devname = xstrdup (fsp->f_mntfromname);
	me->me_mountdir = xstrdup (fsp->f_mntonname);
	me->me_type = fs_type;
	me->me_dummy = ME_DUMMY (me->me_devname, me->me_type);
	me->me_remote = ME_REMOTE (me->me_devname, me->me_type);
	me->me_dev = (dev_t) -1;	/* Magic; means not known yet. */

	/* Add to the linked list. */
	*mtail = me;
	mtail = &me->me_next;
      }
  }
#endif /* MOUNTED_GETMNTINFO */

#ifdef MOUNTED_GETMNT		/* Ultrix.  */
  {
    int offset = 0;
    int val;
    struct fs_data fsd;

    while (errno = 0,
	   0 < (val = getmnt (&offset, &fsd, sizeof (fsd), NOSTAT_MANY,
			      (char *) 0)))
      {
	me = (struct mount_entry *) xmalloc (sizeof (struct mount_entry));
	me->me_devname = xstrdup (fsd.fd_req.devname);
	me->me_mountdir = xstrdup (fsd.fd_req.path);
	me->me_type = gt_names[fsd.fd_req.fstype];
	me->me_dummy = ME_DUMMY (me->me_devname, me->me_type);
	me->me_remote = ME_REMOTE (me->me_devname, me->me_type);
	me->me_dev = fsd.fd_req.dev;

	/* Add to the linked list. */
	*mtail = me;
	mtail = &me->me_next;
      }
    if (val < 0)
      goto free_then_fail;
  }
#endif /* MOUNTED_GETMNT. */

#if defined MOUNTED_FS_STAT_DEV /* BeOS */
  {
    /* The next_dev() and fs_stat_dev() system calls give the list of
       all filesystems, including the information returned by statvfs()
       (fs type, total blocks, free blocks etc.), but without the mount
       point. But on BeOS all filesystems except / are mounted in the
       rootfs, directly under /.
       The directory name of the mount point is often, but not always,
       identical to the volume name of the device.
       We therefore get the list of subdirectories of /, and the list
       of all filesystems, and match the two lists.  */

    DIR *dirp;
    struct rootdir_entry
      {
        char *name;
        dev_t dev;
        ino_t ino;
        struct rootdir_entry *next;
      };
    struct rootdir_entry *rootdir_list;
    struct rootdir_entry **rootdir_tail;
    int32 pos;
    dev_t dev;
    fs_info fi;

    /* All volumes are mounted in the rootfs, directly under /. */
    rootdir_list = NULL;
    rootdir_tail = &rootdir_list;
    dirp = opendir ("/");
    if (dirp)
      {
        struct dirent *d;

        while ((d = readdir (dirp)) != NULL)
          {
            char *name;
            struct stat statbuf;

            if (strcmp (d->d_name, "..") == 0)
              continue;

            if (strcmp (d->d_name, ".") == 0)
              name = xstrdup ("/");
            else
              {
                name = xmalloc (1 + strlen (d->d_name) + 1);
                name[0] = '/';
                strcpy (name + 1, d->d_name);
              }

            if (lstat (name, &statbuf) >= 0 && S_ISDIR (statbuf.st_mode))
              {
                struct rootdir_entry *re;

                re = (struct rootdir_entry *) xmalloc (sizeof (struct rootdir_entry));
                re->name = name;
                re->dev = statbuf.st_dev;
                re->ino = statbuf.st_ino;

                /* Add to the linked list.  */
                *rootdir_tail = re;
                rootdir_tail = &re->next;
              }
            else
              free (name);
          }
        closedir (dirp);
      }
    *rootdir_tail = NULL;

    for (pos = 0; (dev = next_dev (&pos)) >= 0; )
      if (fs_stat_dev (dev, &fi) >= 0)
        {
          /* Note: fi.dev == dev. */
          struct rootdir_entry *re;

          for (re = rootdir_list; re; re = re->next)
            if (re->dev == fi.dev && re->ino == fi.root)
              break;

          me = (struct mount_entry *) xmalloc (sizeof (struct mount_entry));
          me->me_devname = xstrdup (fi.device_name[0] != '\0' ? fi.device_name : fi.fsh_name);
          me->me_mountdir = xstrdup (re != NULL ? re->name : fi.fsh_name);
          me->me_type = xstrdup (fi.fsh_name);
          me->me_dev = fi.dev;
          me->me_dummy = 0;
          me->me_remote = (fi.flags & B_FS_IS_SHARED) != 0;

          /* Add to the linked list. */
          *mtail = me;
          mtail = &me->me_next;
        }
    *mtail = NULL;

    while (rootdir_list != NULL)
      {
        struct rootdir_entry *re = rootdir_list;
        rootdir_list = re->next;
        free (re->name);
        free (re);
      }
  }
#endif /* MOUNTED_FS_STAT_DEV */

#if defined MOUNTED_GETFSSTAT	/* __alpha running OSF_1 */
  {
    int numsys, counter, bufsize;
    struct statfs *stats;

    numsys = getfsstat ((struct statfs *)0, 0L, MNT_WAIT);
    if (numsys < 0)
      return (NULL);

    bufsize = (1 + numsys) * sizeof (struct statfs);
    stats = (struct statfs *)xmalloc (bufsize);
    numsys = getfsstat (stats, bufsize, MNT_WAIT);

    if (numsys < 0)
      {
	free (stats);
	return (NULL);
      }

    for (counter = 0; counter < numsys; counter++)
      {
	me = (struct mount_entry *) xmalloc (sizeof (struct mount_entry));
	me->me_devname = xstrdup (stats[counter].f_mntfromname);
	me->me_mountdir = xstrdup (stats[counter].f_mntonname);
	me->me_type = xstrdup (FS_TYPE (stats[counter]));
	me->me_dummy = ME_DUMMY (me->me_devname, me->me_type);
	me->me_remote = ME_REMOTE (me->me_devname, me->me_type);
	me->me_dev = (dev_t) -1;	/* Magic; means not known yet. */

	/* Add to the linked list. */
	*mtail = me;
	mtail = &me->me_next;
      }

    free (stats);
  }
#endif /* MOUNTED_GETFSSTAT */

#if defined MOUNTED_FREAD || defined MOUNTED_FREAD_FSTYP /* SVR[23].  */
  {
    struct mnttab mnt;
    char *table = "/etc/mnttab";
    FILE *fp;

    fp = fopen (table, "r");
    if (fp == NULL)
      return NULL;

    while (fread (&mnt, sizeof mnt, 1, fp) > 0)
      {
	me = (struct mount_entry *) xmalloc (sizeof (struct mount_entry));
# ifdef GETFSTYP			/* SVR3.  */
	me->me_devname = xstrdup (mnt.mt_dev);
# else
	me->me_devname = xmalloc (strlen (mnt.mt_dev) + 6);
	strcpy (me->me_devname, "/dev/");
	strcpy (me->me_devname + 5, mnt.mt_dev);
# endif
	me->me_mountdir = xstrdup (mnt.mt_filsys);
	me->me_dev = (dev_t) -1;	/* Magic; means not known yet. */
	me->me_type = "";
# ifdef GETFSTYP			/* SVR3.  */
	if (need_fs_type)
	  {
	    struct statfs fsd;
	    char typebuf[FSTYPSZ];

	    if (statfs (me->me_mountdir, &fsd, sizeof fsd, 0) != -1
		&& sysfs (GETFSTYP, fsd.f_fstyp, typebuf) != -1)
	      me->me_type = xstrdup (typebuf);
	  }
# endif
	me->me_dummy = ME_DUMMY (me->me_devname, me->me_type);
	me->me_remote = ME_REMOTE (me->me_devname, me->me_type);

	/* Add to the linked list. */
	*mtail = me;
	mtail = &me->me_next;
      }

    if (ferror (fp))
      {
	int saved_errno = errno;
	fclose (fp);
	errno = saved_errno;
	goto free_then_fail;
      }

    if (fclose (fp) == EOF)
      goto free_then_fail;
  }
#endif /* MOUNTED_FREAD || MOUNTED_FREAD_FSTYP.  */

#ifdef MOUNTED_GETMNTTBL	/* DolphinOS goes it's own way */
  {
    struct mntent **mnttbl=getmnttbl(),**ent;
    for (ent=mnttbl;*ent;ent++)
      {
	me = (struct mount_entry *) xmalloc (sizeof (struct mount_entry));
	me->me_devname = xstrdup ( (*ent)->mt_resource);
	me->me_mountdir = xstrdup( (*ent)->mt_directory);
	me->me_type =  xstrdup ((*ent)->mt_fstype);
	me->me_dummy = ME_DUMMY (me->me_devname, me->me_type);
	me->me_remote = ME_REMOTE (me->me_devname, me->me_type);
	me->me_dev = (dev_t) -1;	/* Magic; means not known yet. */

	/* Add to the linked list. */
	*mtail = me;
	mtail = &me->me_next;
      }
    endmnttbl();
  }
#endif

#ifdef MOUNTED_GETMNTENT2	/* SVR4.  */
  {
    struct mnttab mnt;
    char *table = MNTTAB;
    FILE *fp;
    int ret;
    int lockfd = -1;

# if defined F_RDLCK && defined F_SETLKW
    /* MNTTAB_LOCK is a macro name of our own invention; it's not present in
       e.g. Solaris 2.6.  If the SVR4 folks ever define a macro
       for this file name, we should use their macro name instead.
       (Why not just lock MNTTAB directly?  We don't know.)  */
#  ifndef MNTTAB_LOCK
#   define MNTTAB_LOCK "/etc/.mnttab.lock"
#  endif
    lockfd = open (MNTTAB_LOCK, O_RDONLY);
    if (0 <= lockfd)
      {
	struct flock flock;
	flock.l_type = F_RDLCK;
	flock.l_whence = SEEK_SET;
	flock.l_start = 0;
	flock.l_len = 0;
	while (fcntl (lockfd, F_SETLKW, &flock) == -1)
	  if (errno != EINTR)
	    {
	      int saved_errno = errno;
	      close (lockfd);
	      errno = saved_errno;
	      return NULL;
	    }
      }
    else if (errno != ENOENT)
      return NULL;
# endif

    errno = 0;
    fp = fopen (table, "r");
    if (fp == NULL)
      ret = errno;
    else
      {
	while ((ret = getmntent (fp, &mnt)) == 0)
	  {
	    me = (struct mount_entry *) xmalloc (sizeof (struct mount_entry));
	    me->me_devname = xstrdup (mnt.mnt_special);
	    me->me_mountdir = xstrdup (mnt.mnt_mountp);
	    me->me_type = xstrdup (mnt.mnt_fstype);
	    me->me_dummy = MNT_IGNORE (&mnt) != 0;
	    me->me_remote = ME_REMOTE (me->me_devname, me->me_type);
	    me->me_dev = (dev_t) -1;	/* Magic; means not known yet. */

	    /* Add to the linked list. */
	    *mtail = me;
	    mtail = &me->me_next;
	  }

	ret = fclose (fp) == EOF ? errno : 0 < ret ? 0 : -1;
      }

    if (0 <= lockfd && close (lockfd) != 0)
      ret = errno;

    if (0 <= ret)
      {
	errno = ret;
	goto free_then_fail;
      }
  }
#endif /* MOUNTED_GETMNTENT2.  */

#ifdef MOUNTED_VMOUNT		/* AIX.  */
  {
    int bufsize;
    char *entries, *thisent;
    struct vmount *vmp;
    int n_entries;
    int i;

    /* Ask how many bytes to allocate for the mounted filesystem info.  */
    if (mntctl (MCTL_QUERY, sizeof bufsize, (struct vmount *) &bufsize) != 0)
      return NULL;
    entries = xmalloc (bufsize);

    /* Get the list of mounted filesystems.  */
    n_entries = mntctl (MCTL_QUERY, bufsize, (struct vmount *) entries);
    if (n_entries < 0)
      {
	int saved_errno = errno;
	free (entries);
	errno = saved_errno;
	return NULL;
      }

    for (i = 0, thisent = entries;
	 i < n_entries;
	 i++, thisent += vmp->vmt_length)
      {
	char *options, *ignore;

	vmp = (struct vmount *) thisent;
	me = (struct mount_entry *) xmalloc (sizeof (struct mount_entry));
	if (vmp->vmt_flags & MNT_REMOTE)
	  {
	    char *host, *path;

	    me->me_remote = 1;
	    /* Prepend the remote pathname.  */
	    host = thisent + vmp->vmt_data[VMT_HOSTNAME].vmt_off;
	    path = thisent + vmp->vmt_data[VMT_OBJECT].vmt_off;
	    me->me_devname = xmalloc (strlen (host) + strlen (path) + 2);
	    strcpy (me->me_devname, host);
	    strcat (me->me_devname, ":");
	    strcat (me->me_devname, path);
	  }
	else
	  {
	    me->me_remote = 0;
	    me->me_devname = xstrdup (thisent +
				      vmp->vmt_data[VMT_OBJECT].vmt_off);
	  }
	me->me_mountdir = xstrdup (thisent + vmp->vmt_data[VMT_STUB].vmt_off);
	me->me_type = xstrdup (fstype_to_string (vmp->vmt_gfstype));
	options = thisent + vmp->vmt_data[VMT_ARGS].vmt_off;
	ignore = strstr (options, "ignore");
	me->me_dummy = (ignore
			&& (ignore == options || ignore[-1] == ',')
			&& (ignore[sizeof "ignore" - 1] == ','
			    || ignore[sizeof "ignore" - 1] == '\0'));
	me->me_dev = (dev_t) -1; /* vmt_fsid might be the info we want.  */

	/* Add to the linked list. */
	*mtail = me;
	mtail = &me->me_next;
      }
    free (entries);
  }
#endif /* MOUNTED_VMOUNT. */

  *mtail = NULL;
  return mount_list;


 free_then_fail:
  {
    int saved_errno = errno;
    *mtail = NULL;

    while (mount_list)
      {
	me = mount_list->me_next;
	free (mount_list->me_devname);
	free (mount_list->me_mountdir);
	/* FIXME: me_type is not always malloced.  */
	free (mount_list);
	mount_list = me;
      }

    errno = saved_errno;
    return NULL;
  }
}
