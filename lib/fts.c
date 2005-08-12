/* Traverse a file hierarchy.

   Copyright (C) 2004, 2005 Free Software Foundation, Inc.

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

/*-
 * Copyright (c) 1990, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)fts.c	8.6 (Berkeley) 8/14/94";
#endif /* LIBC_SCCS and not lint */

#include "fts_.h"

#if HAVE_SYS_PARAM_H || defined _LIBC
# include <sys/param.h>
#endif
#ifdef _LIBC
# include <include/sys/stat.h>
#else
# include <sys/stat.h>
#endif
#include <fcntl.h>
#include <errno.h>
#include "dirfd.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#if ! _LIBC
# include "lstat.h"
#endif

#if defined _LIBC
# include <dirent.h>
# define NAMLEN(dirent) _D_EXACT_NAMLEN (dirent)
#else
# if HAVE_DIRENT_H
#  include <dirent.h>
#  define NAMLEN(dirent) strlen ((dirent)->d_name)
# else
#  define dirent direct
#  define NAMLEN(dirent) (dirent)->d_namlen
#  if HAVE_SYS_NDIR_H
#   include <sys/ndir.h>
#  endif
#  if HAVE_SYS_DIR_H
#   include <sys/dir.h>
#  endif
#  if HAVE_NDIR_H
#   include <ndir.h>
#  endif
# endif
#endif

#ifdef _LIBC
# undef close
# define close __close
# undef closedir
# define closedir __closedir
# undef fchdir
# define fchdir __fchdir
# undef open
# define open __open
# undef opendir
# define opendir __opendir
# undef readdir
# define readdir __readdir
#else
# undef internal_function
# define internal_function /* empty */
#endif

#ifndef __set_errno
# define __set_errno(Val) errno = (Val)
#endif

#ifndef __attribute__
# if __GNUC__ < 2 || (__GNUC__ == 2 && __GNUC_MINOR__ < 8) || __STRICT_ANSI__
#  define __attribute__(x) /* empty */
# endif
#endif

#ifndef ATTRIBUTE_UNUSED
# define ATTRIBUTE_UNUSED __attribute__ ((__unused__))
#endif


static FTSENT	*fts_alloc (FTS *, const char *, size_t) internal_function;
static FTSENT	*fts_build (FTS *, int) internal_function;
static void	 fts_lfree (FTSENT *) internal_function;
static void	 fts_load (FTS *, FTSENT *) internal_function;
static size_t	 fts_maxarglen (char * const *) internal_function;
static void	 fts_padjust (FTS *, FTSENT *) internal_function;
static bool	 fts_palloc (FTS *, size_t) internal_function;
static FTSENT	*fts_sort (FTS *, FTSENT *, size_t) internal_function;
static unsigned short int fts_stat (FTS *, FTSENT *, bool) internal_function;
static int      fts_safe_changedir (FTS *, FTSENT *, int, const char *)
     internal_function;

#if _LGPL_PACKAGE
static bool enter_dir (FTS *fts, FTSENT *ent) { return true; }
static void leave_dir (FTS *fts, FTSENT *ent) {}
static bool setup_dir (FTS *fts) { return true; }
static void free_dir (FTS *fts) {}
#else
# include "fcntl--.h"
# include "fts-cycle.c"
#endif

#ifndef MAX
# define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

#ifndef SIZE_MAX
# define SIZE_MAX ((size_t) -1)
#endif

#ifndef O_DIRECTORY
# define O_DIRECTORY 0
#endif

#define ISDOT(a)	(a[0] == '.' && (!a[1] || (a[1] == '.' && !a[2])))

#define CLR(opt)	(sp->fts_options &= ~(opt))
#define ISSET(opt)	(sp->fts_options & (opt))
#define SET(opt)	(sp->fts_options |= (opt))

#define FCHDIR(sp, fd)	(!ISSET(FTS_NOCHDIR) && fchdir(fd))

/* fts_build flags */
#define BCHILD		1		/* fts_children */
#define BNAMES		2		/* fts_children, names only */
#define BREAD		3		/* fts_read */

#if FTS_DEBUG
# include <inttypes.h>
# include <stdint.h>
# include <stdio.h>
bool fts_debug = false;
# define Dprintf(x) do { if (fts_debug) printf x; } while (0)
#else
# define Dprintf(x)
#endif

#define LEAVE_DIR(Fts, Ent, Tag)				\
  do								\
    {								\
      Dprintf (("  %s-leaving: %s\n", Tag, (Ent)->fts_path));	\
      leave_dir (Fts, Ent);					\
    }								\
  while (false)

/* Open the directory DIR if possible, and return a file
   descriptor.  Return -1 and set errno on failure.  It doesn't matter
   whether the file descriptor has read or write access.  */

static int
internal_function
diropen (char const *dir)
{
  int fd = open (dir, O_RDONLY | O_DIRECTORY);
  if (fd < 0)
    fd = open (dir, O_WRONLY | O_DIRECTORY);
  return fd;
}

FTS *
fts_open (char * const *argv,
	  register int options,
	  int (*compar) (FTSENT const **, FTSENT const **))
{
	register FTS *sp;
	register FTSENT *p, *root;
	register size_t nitems;
	FTSENT *parent, *tmp = NULL;	/* pacify gcc */
	size_t len;

	/* Options check. */
	if (options & ~FTS_OPTIONMASK) {
		__set_errno (EINVAL);
		return (NULL);
	}

	/* Allocate/initialize the stream */
	if ((sp = malloc(sizeof(FTS))) == NULL)
		return (NULL);
	memset(sp, 0, sizeof(FTS));
	sp->fts_compar = compar;
	sp->fts_options = options;

	/* Logical walks turn on NOCHDIR; symbolic links are too hard. */
	if (ISSET(FTS_LOGICAL))
		SET(FTS_NOCHDIR);

	/*
	 * Start out with 1K of file name space, and enough, in any case,
	 * to hold the user's file names.
	 */
#ifndef MAXPATHLEN
# define MAXPATHLEN 1024
#endif
	if (! fts_palloc(sp, MAX(fts_maxarglen(argv), MAXPATHLEN)))
		goto mem1;

	/* Allocate/initialize root's parent. */
	if ((parent = fts_alloc(sp, "", 0)) == NULL)
		goto mem2;
	parent->fts_level = FTS_ROOTPARENTLEVEL;

	/* Allocate/initialize root(s). */
	for (root = NULL, nitems = 0; *argv != NULL; ++argv, ++nitems) {
		/* Don't allow zero-length file names. */
		if ((len = strlen(*argv)) == 0) {
			__set_errno (ENOENT);
			goto mem3;
		}

		if ((p = fts_alloc(sp, *argv, len)) == NULL)
			goto mem3;
		p->fts_level = FTS_ROOTLEVEL;
		p->fts_parent = parent;
		p->fts_accpath = p->fts_name;
		p->fts_info = fts_stat(sp, p, ISSET(FTS_COMFOLLOW) != 0);

		/* Command-line "." and ".." are real directories. */
		if (p->fts_info == FTS_DOT)
			p->fts_info = FTS_D;

		/*
		 * If comparison routine supplied, traverse in sorted
		 * order; otherwise traverse in the order specified.
		 */
		if (compar) {
			p->fts_link = root;
			root = p;
		} else {
			p->fts_link = NULL;
			if (root == NULL)
				tmp = root = p;
			else {
				tmp->fts_link = p;
				tmp = p;
			}
		}
	}
	if (compar && nitems > 1)
		root = fts_sort(sp, root, nitems);

	/*
	 * Allocate a dummy pointer and make fts_read think that we've just
	 * finished the node before the root(s); set p->fts_info to FTS_INIT
	 * so that everything about the "current" node is ignored.
	 */
	if ((sp->fts_cur = fts_alloc(sp, "", 0)) == NULL)
		goto mem3;
	sp->fts_cur->fts_link = root;
	sp->fts_cur->fts_info = FTS_INIT;
	if (! setup_dir (sp))
	  goto mem3;

	/*
	 * If using chdir(2), grab a file descriptor pointing to dot to ensure
	 * that we can get back here; this could be avoided for some file names,
	 * but almost certainly not worth the effort.  Slashes, symbolic links,
	 * and ".." are all fairly nasty problems.  Note, if we can't get the
	 * descriptor we run anyway, just more slowly.
	 */
	if (!ISSET(FTS_NOCHDIR)
	    && (sp->fts_rfd = diropen (".")) < 0)
		SET(FTS_NOCHDIR);

	return (sp);

mem3:	fts_lfree(root);
	free(parent);
mem2:	free(sp->fts_path);
mem1:	free(sp);
	return (NULL);
}

static void
internal_function
fts_load (FTS *sp, register FTSENT *p)
{
	register size_t len;
	register char *cp;

	/*
	 * Load the stream structure for the next traversal.  Since we don't
	 * actually enter the directory until after the preorder visit, set
	 * the fts_accpath field specially so the chdir gets done to the right
	 * place and the user can access the first node.  From fts_open it's
	 * known that the file name will fit.
	 */
	len = p->fts_pathlen = p->fts_namelen;
	memmove(sp->fts_path, p->fts_name, len + 1);
	if ((cp = strrchr(p->fts_name, '/')) && (cp != p->fts_name || cp[1])) {
		len = strlen(++cp);
		memmove(p->fts_name, cp, len + 1);
		p->fts_namelen = len;
	}
	p->fts_accpath = p->fts_path = sp->fts_path;
	sp->fts_dev = p->fts_statp->st_dev;
}

int
fts_close (FTS *sp)
{
	register FTSENT *freep, *p;
	int saved_errno = 0;

	/*
	 * This still works if we haven't read anything -- the dummy structure
	 * points to the root list, so we step through to the end of the root
	 * list which has a valid parent pointer.
	 */
	if (sp->fts_cur) {
		for (p = sp->fts_cur; p->fts_level >= FTS_ROOTLEVEL;) {
			freep = p;
			p = p->fts_link != NULL ? p->fts_link : p->fts_parent;
			free(freep);
		}
		free(p);
	}

	/* Free up child linked list, sort array, file name buffer. */
	if (sp->fts_child)
		fts_lfree(sp->fts_child);
	if (sp->fts_array)
		free(sp->fts_array);
	free(sp->fts_path);

	/* Return to original directory, save errno if necessary. */
	if (!ISSET(FTS_NOCHDIR)) {
		if (fchdir(sp->fts_rfd))
			saved_errno = errno;
		(void)close(sp->fts_rfd);
	}

	free_dir (sp);

	/* Free up the stream pointer. */
	free(sp);

	/* Set errno and return. */
	if (saved_errno) {
		__set_errno (saved_errno);
		return (-1);
	}

	return (0);
}

/*
 * Special case of "/" at the end of the file name so that slashes aren't
 * appended which would cause file names to be written as "....//foo".
 */
#define NAPPEND(p)							\
	(p->fts_path[p->fts_pathlen - 1] == '/'				\
	    ? p->fts_pathlen - 1 : p->fts_pathlen)

FTSENT *
fts_read (register FTS *sp)
{
	register FTSENT *p, *tmp;
	register unsigned short int instr;
	register char *t;
	int saved_errno;

	/* If finished or unrecoverable error, return NULL. */
	if (sp->fts_cur == NULL || ISSET(FTS_STOP))
		return (NULL);

	/* Set current node pointer. */
	p = sp->fts_cur;

	/* Save and zero out user instructions. */
	instr = p->fts_instr;
	p->fts_instr = FTS_NOINSTR;

	/* Any type of file may be re-visited; re-stat and re-turn. */
	if (instr == FTS_AGAIN) {
		p->fts_info = fts_stat(sp, p, false);
		return (p);
	}
	Dprintf (("fts_read: p=%s\n",
		  p->fts_info == FTS_INIT ? "" : p->fts_path));

	/*
	 * Following a symlink -- SLNONE test allows application to see
	 * SLNONE and recover.  If indirecting through a symlink, have
	 * keep a pointer to current location.  If unable to get that
	 * pointer, follow fails.
	 */
	if (instr == FTS_FOLLOW &&
	    (p->fts_info == FTS_SL || p->fts_info == FTS_SLNONE)) {
		p->fts_info = fts_stat(sp, p, true);
		if (p->fts_info == FTS_D && !ISSET(FTS_NOCHDIR)) {
			if ((p->fts_symfd = diropen (".")) < 0) {
				p->fts_errno = errno;
				p->fts_info = FTS_ERR;
			} else
				p->fts_flags |= FTS_SYMFOLLOW;
		}
		goto check_for_dir;
	}

	/* Directory in pre-order. */
	if (p->fts_info == FTS_D) {
		/* If skipped or crossed mount point, do post-order visit. */
		if (instr == FTS_SKIP ||
		    (ISSET(FTS_XDEV) && p->fts_statp->st_dev != sp->fts_dev)) {
			if (p->fts_flags & FTS_SYMFOLLOW)
				(void)close(p->fts_symfd);
			if (sp->fts_child) {
				fts_lfree(sp->fts_child);
				sp->fts_child = NULL;
			}
			p->fts_info = FTS_DP;
			LEAVE_DIR (sp, p, "1");
			return (p);
		}

		/* Rebuild if only read the names and now traversing. */
		if (sp->fts_child != NULL && ISSET(FTS_NAMEONLY)) {
			CLR(FTS_NAMEONLY);
			fts_lfree(sp->fts_child);
			sp->fts_child = NULL;
		}

		/*
		 * Cd to the subdirectory.
		 *
		 * If have already read and now fail to chdir, whack the list
		 * to make the names come out right, and set the parent errno
		 * so the application will eventually get an error condition.
		 * Set the FTS_DONTCHDIR flag so that when we logically change
		 * directories back to the parent we don't do a chdir.
		 *
		 * If haven't read do so.  If the read fails, fts_build sets
		 * FTS_STOP or the fts_info field of the node.
		 */
		if (sp->fts_child != NULL) {
			if (fts_safe_changedir(sp, p, -1, p->fts_accpath)) {
				p->fts_errno = errno;
				p->fts_flags |= FTS_DONTCHDIR;
				for (p = sp->fts_child; p != NULL;
				     p = p->fts_link)
					p->fts_accpath =
					    p->fts_parent->fts_accpath;
			}
		} else if ((sp->fts_child = fts_build(sp, BREAD)) == NULL) {
			if (ISSET(FTS_STOP))
				return (NULL);
			/* If fts_build's call to fts_safe_changedir failed
			   because it was not able to fchdir into a
			   subdirectory, tell the caller.  */
			if (p->fts_errno)
				p->fts_info = FTS_ERR;
			/* FIXME: see if this should be in an else block */
			LEAVE_DIR (sp, p, "2");
			return (p);
		}
		p = sp->fts_child;
		sp->fts_child = NULL;
		goto name;
	}

	/* Move to the next node on this level. */
next:	tmp = p;
	if ((p = p->fts_link) != NULL) {
		free(tmp);

		/*
		 * If reached the top, return to the original directory (or
		 * the root of the tree), and load the file names for the next
		 * root.
		 */
		if (p->fts_level == FTS_ROOTLEVEL) {
			if (FCHDIR(sp, sp->fts_rfd)) {
				SET(FTS_STOP);
				return (NULL);
			}
			fts_load(sp, p);
			goto check_for_dir;
		}

		/*
		 * User may have called fts_set on the node.  If skipped,
		 * ignore.  If followed, get a file descriptor so we can
		 * get back if necessary.
		 */
		if (p->fts_instr == FTS_SKIP)
			goto next;
		if (p->fts_instr == FTS_FOLLOW) {
			p->fts_info = fts_stat(sp, p, true);
			if (p->fts_info == FTS_D && !ISSET(FTS_NOCHDIR)) {
				if ((p->fts_symfd = diropen (".")) < 0) {
					p->fts_errno = errno;
					p->fts_info = FTS_ERR;
				} else
					p->fts_flags |= FTS_SYMFOLLOW;
			}
			p->fts_instr = FTS_NOINSTR;
		}

name:		t = sp->fts_path + NAPPEND(p->fts_parent);
		*t++ = '/';
		memmove(t, p->fts_name, p->fts_namelen + 1);
check_for_dir:
		sp->fts_cur = p;
		if (p->fts_info == FTS_D)
		  {
		    Dprintf (("  %s-entering: %s\n", sp, p->fts_path));
		    if (! enter_dir (sp, p))
		      {
			__set_errno (ENOMEM);
			return NULL;
		      }
		  }
		return p;
	}

	/* Move up to the parent node. */
	p = tmp->fts_parent;
	free(tmp);

	if (p->fts_level == FTS_ROOTPARENTLEVEL) {
		/*
		 * Done; free everything up and set errno to 0 so the user
		 * can distinguish between error and EOF.
		 */
		free(p);
		__set_errno (0);
		return (sp->fts_cur = NULL);
	}

	/* NUL terminate the file name.  */
	sp->fts_path[p->fts_pathlen] = '\0';

	/*
	 * Return to the parent directory.  If at a root node or came through
	 * a symlink, go back through the file descriptor.  Otherwise, cd up
	 * one directory.
	 */
	if (p->fts_level == FTS_ROOTLEVEL) {
		if (FCHDIR(sp, sp->fts_rfd)) {
			p->fts_errno = errno;
			SET(FTS_STOP);
		}
	} else if (p->fts_flags & FTS_SYMFOLLOW) {
		if (FCHDIR(sp, p->fts_symfd)) {
			saved_errno = errno;
			(void)close(p->fts_symfd);
			__set_errno (saved_errno);
			p->fts_errno = errno;
			SET(FTS_STOP);
		}
		(void)close(p->fts_symfd);
	} else if (!(p->fts_flags & FTS_DONTCHDIR) &&
		   fts_safe_changedir(sp, p->fts_parent, -1, "..")) {
		p->fts_errno = errno;
		SET(FTS_STOP);
	}
	p->fts_info = p->fts_errno ? FTS_ERR : FTS_DP;
	if (p->fts_errno == 0)
		LEAVE_DIR (sp, p, "3");
	sp->fts_cur = p;
	return ISSET(FTS_STOP) ? NULL : p;
}

/*
 * Fts_set takes the stream as an argument although it's not used in this
 * implementation; it would be necessary if anyone wanted to add global
 * semantics to fts using fts_set.  An error return is allowed for similar
 * reasons.
 */
/* ARGSUSED */
int
fts_set(FTS *sp ATTRIBUTE_UNUSED, FTSENT *p, int instr)
{
	if (instr != 0 && instr != FTS_AGAIN && instr != FTS_FOLLOW &&
	    instr != FTS_NOINSTR && instr != FTS_SKIP) {
		__set_errno (EINVAL);
		return (1);
	}
	p->fts_instr = instr;
	return (0);
}

FTSENT *
fts_children (register FTS *sp, int instr)
{
	register FTSENT *p;
	int fd;

	if (instr != 0 && instr != FTS_NAMEONLY) {
		__set_errno (EINVAL);
		return (NULL);
	}

	/* Set current node pointer. */
	p = sp->fts_cur;

	/*
	 * Errno set to 0 so user can distinguish empty directory from
	 * an error.
	 */
	__set_errno (0);

	/* Fatal errors stop here. */
	if (ISSET(FTS_STOP))
		return (NULL);

	/* Return logical hierarchy of user's arguments. */
	if (p->fts_info == FTS_INIT)
		return (p->fts_link);

	/*
	 * If not a directory being visited in pre-order, stop here.  Could
	 * allow FTS_DNR, assuming the user has fixed the problem, but the
	 * same effect is available with FTS_AGAIN.
	 */
	if (p->fts_info != FTS_D /* && p->fts_info != FTS_DNR */)
		return (NULL);

	/* Free up any previous child list. */
	if (sp->fts_child != NULL)
		fts_lfree(sp->fts_child);

	if (instr == FTS_NAMEONLY) {
		SET(FTS_NAMEONLY);
		instr = BNAMES;
	} else
		instr = BCHILD;

	/*
	 * If using chdir on a relative file name and called BEFORE fts_read
	 * does its chdir to the root of a traversal, we can lose -- we need to
	 * chdir into the subdirectory, and we don't know where the current
	 * directory is, so we can't get back so that the upcoming chdir by
	 * fts_read will work.
	 */
	if (p->fts_level != FTS_ROOTLEVEL || p->fts_accpath[0] == '/' ||
	    ISSET(FTS_NOCHDIR))
		return (sp->fts_child = fts_build(sp, instr));

	if ((fd = diropen (".")) < 0)
		return (sp->fts_child = NULL);
	sp->fts_child = fts_build(sp, instr);
	if (fchdir(fd)) {
		(void)close(fd);
		return (NULL);
	}
	(void)close(fd);
	return (sp->fts_child);
}

/*
 * This is the tricky part -- do not casually change *anything* in here.  The
 * idea is to build the linked list of entries that are used by fts_children
 * and fts_read.  There are lots of special cases.
 *
 * The real slowdown in walking the tree is the stat calls.  If FTS_NOSTAT is
 * set and it's a physical walk (so that symbolic links can't be directories),
 * we can do things quickly.  First, if it's a 4.4BSD file system, the type
 * of the file is in the directory entry.  Otherwise, we assume that the number
 * of subdirectories in a node is equal to the number of links to the parent.
 * The former skips all stat calls.  The latter skips stat calls in any leaf
 * directories and for any files after the subdirectories in the directory have
 * been found, cutting the stat calls by about 2/3.
 */
static FTSENT *
internal_function
fts_build (register FTS *sp, int type)
{
	register struct dirent *dp;
	register FTSENT *p, *head;
	register size_t nitems;
	FTSENT *cur, *tail;
	DIR *dirp;
	void *oldaddr;
	int cderrno;
	int saved_errno;
	bool descend;
	bool doadjust;
	ptrdiff_t level;
	nlink_t nlinks;
	bool nostat;
	size_t len, maxlen, new_len;
	char *cp;

	/* Set current node pointer. */
	cur = sp->fts_cur;

	/*
	 * Open the directory for reading.  If this fails, we're done.
	 * If being called from fts_read, set the fts_info field.
	 */
#if defined FTS_WHITEOUT && 0
	if (ISSET(FTS_WHITEOUT))
		oflag = DTF_NODUP|DTF_REWIND;
	else
		oflag = DTF_HIDEW|DTF_NODUP|DTF_REWIND;
#else
# define __opendir2(file, flag) opendir(file)
#endif
       if ((dirp = __opendir2(cur->fts_accpath, oflag)) == NULL) {
		if (type == BREAD) {
			cur->fts_info = FTS_DNR;
			cur->fts_errno = errno;
		}
		return (NULL);
	}

	/*
	 * Nlinks is the number of possible entries of type directory in the
	 * directory if we're cheating on stat calls, 0 if we're not doing
	 * any stat calls at all, (nlink_t) -1 if we're statting everything.
	 */
	if (type == BNAMES) {
		nlinks = 0;
		/* Be quiet about nostat, GCC. */
		nostat = false;
	} else if (ISSET(FTS_NOSTAT) && ISSET(FTS_PHYSICAL)) {
		nlinks = (cur->fts_statp->st_nlink
			  - (ISSET(FTS_SEEDOT) ? 0 : 2));
		nostat = true;
	} else {
		nlinks = -1;
		nostat = false;
	}

	/*
	 * If we're going to need to stat anything or we want to descend
	 * and stay in the directory, chdir.  If this fails we keep going,
	 * but set a flag so we don't chdir after the post-order visit.
	 * We won't be able to stat anything, but we can still return the
	 * names themselves.  Note, that since fts_read won't be able to
	 * chdir into the directory, it will have to return different file
	 * names than before, i.e. "a/b" instead of "b".  Since the node
	 * has already been visited in pre-order, have to wait until the
	 * post-order visit to return the error.  There is a special case
	 * here, if there was nothing to stat then it's not an error to
	 * not be able to stat.  This is all fairly nasty.  If a program
	 * needed sorted entries or stat information, they had better be
	 * checking FTS_NS on the returned nodes.
	 */
	cderrno = 0;
	if (nlinks || type == BREAD) {
		if (fts_safe_changedir(sp, cur, dirfd(dirp), NULL)) {
			if (nlinks && type == BREAD)
				cur->fts_errno = errno;
			cur->fts_flags |= FTS_DONTCHDIR;
			descend = false;
			cderrno = errno;
			closedir(dirp);
			dirp = NULL;
		} else
			descend = true;
	} else
		descend = false;

	/*
	 * Figure out the max file name length that can be stored in the
	 * current buffer -- the inner loop allocates more space as necessary.
	 * We really wouldn't have to do the maxlen calculations here, we
	 * could do them in fts_read before returning the name, but it's a
	 * lot easier here since the length is part of the dirent structure.
	 *
	 * If not changing directories set a pointer so that can just append
	 * each new component into the file name.
	 */
	len = NAPPEND(cur);
	if (ISSET(FTS_NOCHDIR)) {
		cp = sp->fts_path + len;
		*cp++ = '/';
	} else {
		/* GCC, you're too verbose. */
		cp = NULL;
	}
	len++;
	maxlen = sp->fts_pathlen - len;

	level = cur->fts_level + 1;

	/* Read the directory, attaching each entry to the `link' pointer. */
	doadjust = false;
	for (head = tail = NULL, nitems = 0; dirp && (dp = readdir(dirp));) {
		if (!ISSET(FTS_SEEDOT) && ISDOT(dp->d_name))
			continue;

		if ((p = fts_alloc(sp, dp->d_name, NAMLEN (dp))) == NULL)
			goto mem1;
		if (NAMLEN (dp) >= maxlen) {/* include space for NUL */
			oldaddr = sp->fts_path;
			if (! fts_palloc(sp, NAMLEN (dp) + len + 1)) {
				/*
				 * No more memory.  Save
				 * errno, free up the current structure and the
				 * structures already allocated.
				 */
mem1:				saved_errno = errno;
				if (p)
					free(p);
				fts_lfree(head);
				closedir(dirp);
				cur->fts_info = FTS_ERR;
				SET(FTS_STOP);
				__set_errno (saved_errno);
				return (NULL);
			}
			/* Did realloc() change the pointer? */
			if (oldaddr != sp->fts_path) {
				doadjust = true;
				if (ISSET(FTS_NOCHDIR))
					cp = sp->fts_path + len;
			}
			maxlen = sp->fts_pathlen - len;
		}

		new_len = len + NAMLEN (dp);
		if (new_len < len) {
			/*
			 * In the unlikely even that we would end up
			 * with a file name longer than SIZE_MAX, free up
			 * the current structure and the structures already
			 * allocated, then error out with ENAMETOOLONG.
			 */
			free(p);
			fts_lfree(head);
			closedir(dirp);
			cur->fts_info = FTS_ERR;
			SET(FTS_STOP);
			__set_errno (ENAMETOOLONG);
			return (NULL);
		}
		p->fts_level = level;
		p->fts_parent = sp->fts_cur;
		p->fts_pathlen = new_len;

#if defined FTS_WHITEOUT && 0
		if (dp->d_type == DT_WHT)
			p->fts_flags |= FTS_ISW;
#endif

		if (cderrno) {
			if (nlinks) {
				p->fts_info = FTS_NS;
				p->fts_errno = cderrno;
			} else
				p->fts_info = FTS_NSOK;
			p->fts_accpath = cur->fts_accpath;
		} else if (nlinks == 0
#if HAVE_STRUCT_DIRENT_D_TYPE
			   || (nostat &&
			       dp->d_type != DT_DIR && dp->d_type != DT_UNKNOWN)
#endif
		    ) {
			p->fts_accpath =
			    ISSET(FTS_NOCHDIR) ? p->fts_path : p->fts_name;
			p->fts_info = FTS_NSOK;
		} else {
			/* Build a file name for fts_stat to stat. */
			if (ISSET(FTS_NOCHDIR)) {
				p->fts_accpath = p->fts_path;
				memmove(cp, p->fts_name, p->fts_namelen + 1);
			} else
				p->fts_accpath = p->fts_name;
			/* Stat it. */
			p->fts_info = fts_stat(sp, p, false);

			/* Decrement link count if applicable. */
			if (nlinks > 0 && (p->fts_info == FTS_D ||
			    p->fts_info == FTS_DC || p->fts_info == FTS_DOT))
				nlinks -= nostat;
		}

		/* We walk in directory order so "ls -f" doesn't get upset. */
		p->fts_link = NULL;
		if (head == NULL)
			head = tail = p;
		else {
			tail->fts_link = p;
			tail = p;
		}
		++nitems;
	}
	if (dirp)
		closedir(dirp);

	/*
	 * If realloc() changed the address of the file name, adjust the
	 * addresses for the rest of the tree and the dir list.
	 */
	if (doadjust)
		fts_padjust(sp, head);

	/*
	 * If not changing directories, reset the file name back to original
	 * state.
	 */
	if (ISSET(FTS_NOCHDIR)) {
		if (len == sp->fts_pathlen || nitems == 0)
			--cp;
		*cp = '\0';
	}

	/*
	 * If descended after called from fts_children or after called from
	 * fts_read and nothing found, get back.  At the root level we use
	 * the saved fd; if one of fts_open()'s arguments is a relative name
	 * to an empty directory, we wind up here with no other way back.  If
	 * can't get back, we're done.
	 */
	if (descend && (type == BCHILD || !nitems) &&
	    (cur->fts_level == FTS_ROOTLEVEL ?
	     FCHDIR(sp, sp->fts_rfd) :
	     fts_safe_changedir(sp, cur->fts_parent, -1, ".."))) {
		cur->fts_info = FTS_ERR;
		SET(FTS_STOP);
		return (NULL);
	}

	/* If didn't find anything, return NULL. */
	if (!nitems) {
		if (type == BREAD)
			cur->fts_info = FTS_DP;
		return (NULL);
	}

	/* Sort the entries. */
	if (sp->fts_compar && nitems > 1)
		head = fts_sort(sp, head, nitems);
	return (head);
}

#if FTS_DEBUG

/* Walk ->fts_parent links starting at E_CURR, until the root of the
   current hierarchy.  There should be a directory with dev/inode
   matching those of AD.  If not, print a lot of diagnostics.  */
static void
find_matching_ancestor (FTSENT const *e_curr, struct Active_dir const *ad)
{
  FTSENT const *ent;
  for (ent = e_curr; ent->fts_level >= FTS_ROOTLEVEL; ent = ent->fts_parent)
    {
      if (ad->ino == ent->fts_statp->st_ino
	  && ad->dev == ent->fts_statp->st_dev)
	return;
    }
  printf ("ERROR: tree dir, %s, not active\n", ad->fts_ent->fts_accpath);
  printf ("active dirs:\n");
  for (ent = e_curr;
       ent->fts_level >= FTS_ROOTLEVEL; ent = ent->fts_parent)
    printf ("  %s(%"PRIuMAX"/%"PRIuMAX") to %s(%"PRIuMAX"/%"PRIuMAX")...\n",
	    ad->fts_ent->fts_accpath,
	    (uintmax_t) ad->dev,
	    (uintmax_t) ad->ino,
	    ent->fts_accpath,
	    (uintmax_t) ent->fts_statp->st_dev,
	    (uintmax_t) ent->fts_statp->st_ino);
}

void
fts_cross_check (FTS const *sp)
{
  FTSENT const *ent = sp->fts_cur;
  FTSENT const *t;
  if ( ! ISSET (FTS_TIGHT_CYCLE_CHECK))
    return;

  Dprintf (("fts-cross-check cur=%s\n", ent->fts_path));
  /* Make sure every parent dir is in the tree.  */
  for (t = ent->fts_parent; t->fts_level >= FTS_ROOTLEVEL; t = t->fts_parent)
    {
      struct Active_dir ad;
      ad.ino = t->fts_statp->st_ino;
      ad.dev = t->fts_statp->st_dev;
      if ( ! hash_lookup (sp->fts_cycle.ht, &ad))
	printf ("ERROR: active dir, %s, not in tree\n", t->fts_path);
    }

  /* Make sure every dir in the tree is an active dir.
     But ENT is not necessarily a directory.  If so, just skip this part. */
  if (ent->fts_parent->fts_level >= FTS_ROOTLEVEL
      && (ent->fts_info == FTS_DP
	  || ent->fts_info == FTS_D))
    {
      struct Active_dir *ad;
      for (ad = hash_get_first (sp->fts_cycle.ht); ad != NULL;
	   ad = hash_get_next (sp->fts_cycle.ht, ad))
	{
	  find_matching_ancestor (ent, ad);
	}
    }
}
#endif

static unsigned short int
internal_function
fts_stat(FTS *sp, register FTSENT *p, bool follow)
{
	struct stat *sbp = p->fts_statp;
	int saved_errno;

#if defined FTS_WHITEOUT && 0
	/* check for whiteout */
	if (p->fts_flags & FTS_ISW) {
		memset(sbp, '\0', sizeof (*sbp));
		sbp->st_mode = S_IFWHT;
		return (FTS_W);
       }
#endif

	/*
	 * If doing a logical walk, or application requested FTS_FOLLOW, do
	 * a stat(2).  If that fails, check for a non-existent symlink.  If
	 * fail, set the errno from the stat call.
	 */
	if (ISSET(FTS_LOGICAL) || follow) {
		if (stat(p->fts_accpath, sbp)) {
			saved_errno = errno;
			if (!lstat(p->fts_accpath, sbp)) {
				__set_errno (0);
				return (FTS_SLNONE);
			}
			p->fts_errno = saved_errno;
			goto err;
		}
	} else if (lstat(p->fts_accpath, sbp)) {
		p->fts_errno = errno;
err:		memset(sbp, 0, sizeof(struct stat));
		return (FTS_NS);
	}

	if (S_ISDIR(sbp->st_mode)) {
		if (ISDOT(p->fts_name))
			return (FTS_DOT);

#if _LGPL_PACKAGE
		{
		  /*
		   * Cycle detection is done by brute force when the directory
		   * is first encountered.  If the tree gets deep enough or the
		   * number of symbolic links to directories is high enough,
		   * something faster might be worthwhile.
		   */
		  FTSENT *t;

		  for (t = p->fts_parent;
		       t->fts_level >= FTS_ROOTLEVEL; t = t->fts_parent)
		    if (sbp->st_ino == t->fts_statp->st_ino
			&& sbp->st_dev == t->fts_statp->st_dev)
		      {
			p->fts_cycle = t;
			return (FTS_DC);
		      }
		}
#endif

		return (FTS_D);
	}
	if (S_ISLNK(sbp->st_mode))
		return (FTS_SL);
	if (S_ISREG(sbp->st_mode))
		return (FTS_F);
	return (FTS_DEFAULT);
}

static int
fts_compar (void const *a, void const *b)
{
  /* Convert A and B to the correct types, to pacify the compiler, and
     for portability to bizarre hosts where "void const *" and "FTSENT
     const **" differ in runtime representation.  The comparison
     function cannot modify *a and *b, but there is no compile-time
     check for this.  */
  FTSENT const **pa = (FTSENT const **) a;
  FTSENT const **pb = (FTSENT const **) b;
  return pa[0]->fts_fts->fts_compar (pa, pb);
}

static FTSENT *
internal_function
fts_sort (FTS *sp, FTSENT *head, register size_t nitems)
{
	register FTSENT **ap, *p;

	/* On most modern hosts, void * and FTSENT ** have the same
	   run-time representation, and one can convert sp->fts_compar to
	   the type qsort expects without problem.  Use the heuristic that
	   this is OK if the two pointer types are the same size, and if
	   converting FTSENT ** to long int is the same as converting
	   FTSENT ** to void * and then to long int.  This heuristic isn't
	   valid in general but we don't know of any counterexamples.  */
	FTSENT *dummy;
	int (*compare) (void const *, void const *) =
	  ((sizeof &dummy == sizeof (void *)
	    && (long int) &dummy == (long int) (void *) &dummy)
	   ? (int (*) (void const *, void const *)) sp->fts_compar
	   : fts_compar);

	/*
	 * Construct an array of pointers to the structures and call qsort(3).
	 * Reassemble the array in the order returned by qsort.  If unable to
	 * sort for memory reasons, return the directory entries in their
	 * current order.  Allocate enough space for the current needs plus
	 * 40 so don't realloc one entry at a time.
	 */
	if (nitems > sp->fts_nitems) {
		struct _ftsent **a;

		sp->fts_nitems = nitems + 40;
		if (SIZE_MAX / sizeof *a < sp->fts_nitems
		    || ! (a = realloc (sp->fts_array,
				       sp->fts_nitems * sizeof *a))) {
			free(sp->fts_array);
			sp->fts_array = NULL;
			sp->fts_nitems = 0;
			return (head);
		}
		sp->fts_array = a;
	}
	for (ap = sp->fts_array, p = head; p; p = p->fts_link)
		*ap++ = p;
	qsort((void *)sp->fts_array, nitems, sizeof(FTSENT *), compare);
	for (head = *(ap = sp->fts_array); --nitems; ++ap)
		ap[0]->fts_link = ap[1];
	ap[0]->fts_link = NULL;
	return (head);
}

static FTSENT *
internal_function
fts_alloc (FTS *sp, const char *name, register size_t namelen)
{
	register FTSENT *p;
	size_t len;

	/*
	 * The file name is a variable length array.  Allocate the FTSENT
	 * structure and the file name in one chunk.
	 */
	len = sizeof(FTSENT) + namelen;
	if ((p = malloc(len)) == NULL)
		return (NULL);

	/* Copy the name and guarantee NUL termination. */
	memmove(p->fts_name, name, namelen);
	p->fts_name[namelen] = '\0';

	p->fts_namelen = namelen;
	p->fts_fts = sp;
	p->fts_path = sp->fts_path;
	p->fts_errno = 0;
	p->fts_flags = 0;
	p->fts_instr = FTS_NOINSTR;
	p->fts_number = 0;
	p->fts_pointer = NULL;
	return (p);
}

static void
internal_function
fts_lfree (register FTSENT *head)
{
	register FTSENT *p;

	/* Free a linked list of structures. */
	while ((p = head)) {
		head = head->fts_link;
		free(p);
	}
}

/*
 * Allow essentially unlimited file name lengths; find, rm, ls should
 * all work on any tree.  Most systems will allow creation of file
 * names much longer than MAXPATHLEN, even though the kernel won't
 * resolve them.  Add the size (not just what's needed) plus 256 bytes
 * so don't realloc the file name 2 bytes at a time.
 */
static bool
internal_function
fts_palloc (FTS *sp, size_t more)
{
	char *p;
	size_t new_len = sp->fts_pathlen + more + 256;

	/*
	 * See if fts_pathlen would overflow.
	 */
	if (new_len < sp->fts_pathlen) {
		if (sp->fts_path) {
			free(sp->fts_path);
			sp->fts_path = NULL;
		}
		sp->fts_path = NULL;
		__set_errno (ENAMETOOLONG);
		return false;
	}
	sp->fts_pathlen = new_len;
	p = realloc(sp->fts_path, sp->fts_pathlen);
	if (p == NULL) {
		free(sp->fts_path);
		sp->fts_path = NULL;
		return false;
	}
	sp->fts_path = p;
	return true;
}

/*
 * When the file name is realloc'd, have to fix all of the pointers in
 *  structures already returned.
 */
static void
internal_function
fts_padjust (FTS *sp, FTSENT *head)
{
	FTSENT *p;
	char *addr = sp->fts_path;

#define ADJUST(p) do {							\
	if ((p)->fts_accpath != (p)->fts_name) {			\
		(p)->fts_accpath =					\
		    (char *)addr + ((p)->fts_accpath - (p)->fts_path);	\
	}								\
	(p)->fts_path = addr;						\
} while (0)
	/* Adjust the current set of children. */
	for (p = sp->fts_child; p; p = p->fts_link)
		ADJUST(p);

	/* Adjust the rest of the tree, including the current level. */
	for (p = head; p->fts_level >= FTS_ROOTLEVEL;) {
		ADJUST(p);
		p = p->fts_link ? p->fts_link : p->fts_parent;
	}
}

static size_t
internal_function
fts_maxarglen (char * const *argv)
{
	size_t len, max;

	for (max = 0; *argv; ++argv)
		if ((len = strlen(*argv)) > max)
			max = len;
	return (max + 1);
}

/*
 * Change to dir specified by fd or file name without getting
 * tricked by someone changing the world out from underneath us.
 * Assumes p->fts_statp->st_dev and p->fts_statp->st_ino are filled in.
 */
static int
internal_function
fts_safe_changedir (FTS *sp, FTSENT *p, int fd, char const *dir)
{
	int ret, oerrno, newfd;
	struct stat sb;

	newfd = fd;
	if (ISSET(FTS_NOCHDIR))
		return (0);
	if (fd < 0 && (newfd = diropen (dir)) < 0)
		return (-1);
	if (fstat(newfd, &sb)) {
		ret = -1;
		goto bail;
	}
	if (p->fts_statp->st_dev != sb.st_dev
	    || p->fts_statp->st_ino != sb.st_ino) {
		__set_errno (ENOENT);		/* disinformation */
		ret = -1;
		goto bail;
	}
	ret = fchdir(newfd);
bail:
	oerrno = errno;
	if (fd < 0)
		(void)close(newfd);
	__set_errno (oerrno);
	return (ret);
}
