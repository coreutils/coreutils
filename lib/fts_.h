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

/*
 * Copyright (c) 1989, 1993
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
 *
 *	@(#)fts.h	8.3 (Berkeley) 8/14/94
 */

#ifndef	_FTS_H
# define _FTS_H 1

# ifdef _LIBC
#  include <features.h>
#  define _LGPL_PACKAGE 1
# else
#  undef __THROW
#  define __THROW
#  undef __BEGIN_DECLS
#  define __BEGIN_DECLS
#  undef __END_DECLS
#  define __END_DECLS
# endif

# include <stddef.h>
# include <sys/types.h>
# include <sys/stat.h>

typedef struct {
	struct _ftsent *fts_cur;	/* current node */
	struct _ftsent *fts_child;	/* linked list of children */
	struct _ftsent **fts_array;	/* sort array */
	dev_t fts_dev;			/* starting device # */
	char *fts_path;			/* file name for this descent */
	int fts_rfd;			/* fd for root */
	size_t fts_pathlen;		/* sizeof(path) */
	size_t fts_nitems;			/* elements in the sort array */
	int (*fts_compar) (struct _ftsent const **, struct _ftsent const **);
					/* compare fn */

# define FTS_COMFOLLOW	0x0001		/* follow command line symlinks */
# define FTS_LOGICAL	0x0002		/* logical walk */
# define FTS_NOCHDIR	0x0004		/* don't change directories */
# define FTS_NOSTAT	0x0008		/* don't get stat info */
# define FTS_PHYSICAL	0x0010		/* physical walk */
# define FTS_SEEDOT	0x0020		/* return dot and dot-dot */
# define FTS_XDEV	0x0040		/* don't cross devices */
# define FTS_WHITEOUT	0x0080		/* return whiteout information */

  /* There are two ways to detect cycles.
     The lazy way (which works only with FTS_PHYSICAL),
     with which one may process a directory that is a
     part of the cycle several times before detecting the cycle.
     The `tight' way, whereby fts uses more memory (proportional
     to number of `active' directories, aka distance from root
     of current tree to current directory -- see active_dir_ht)
     to detect any cycle right away.  For example, du must use
     this option to avoid counting disk space in a cycle multiple
     times, but chown -R need not.
     The default is to use the constant-memory lazy way, when possible
     (see below).

     However, with FTS_LOGICAL (when following symlinks, e.g., chown -L)
     using lazy cycle detection is inadequate.  For example, traversing
     a directory containing a symbolic link to a peer directory, it is
     possible to encounter the same directory twice even though there
     is no cycle:
     dir
     ...
     slink -> dir
     So, when FTS_LOGICAL is selected, we have to use a different
     mode of cycle detection: FTS_TIGHT_CYCLE_CHECK.  */
# define FTS_TIGHT_CYCLE_CHECK	0x0100

# define FTS_OPTIONMASK	0x01ff		/* valid user option mask */

# define FTS_NAMEONLY	0x1000		/* (private) child names only */
# define FTS_STOP	0x2000		/* (private) unrecoverable error */
	int fts_options;		/* fts_open options, global flags */

# if !_LGPL_PACKAGE
	union {
		/* This data structure is used if FTS_TIGHT_CYCLE_CHECK is
		   specified.  It records the directories between a starting
		   point and the current directory.  I.e., a directory is
		   recorded here IFF we have visited it once, but we have not
		   yet completed processing of all its entries.  Every time we
		   visit a new directory, we add that directory to this set.
		   When we finish with a directory (usually by visiting it a
		   second time), we remove it from this set.  Each entry in
		   this data structure is a device/inode pair.  This data
		   structure is used to detect directory cycles efficiently and
		   promptly even when the depth of a hierarchy is in the tens
		   of thousands.  */
		struct hash_table *ht;

		/* This data structure uses lazy checking, as done by rm via
	           cycle-check.c.  It's the default, but it's not appropriate
	           for programs like du.  */
		struct cycle_check_state *state;
	} fts_cycle;
# endif
} FTS;

typedef struct _ftsent {
	struct _ftsent *fts_cycle;	/* cycle node */
	struct _ftsent *fts_parent;	/* parent directory */
	struct _ftsent *fts_link;	/* next file in directory */
	long fts_number;	        /* local numeric value */
	void *fts_pointer;	        /* local address value */
	char *fts_accpath;		/* access file name */
	char *fts_path;			/* root name; == fts_fts->fts_path */
	int fts_errno;			/* errno for this node */
	int fts_symfd;			/* fd for symlink */
	size_t fts_pathlen;		/* strlen(fts_path) */

	FTS *fts_fts;			/* the file hierarchy itself */

# define FTS_ROOTPARENTLEVEL	(-1)
# define FTS_ROOTLEVEL		 0
	ptrdiff_t fts_level;		/* depth (-1 to N) */

	size_t fts_namelen;		/* strlen(fts_name) */

# define FTS_D		 1		/* preorder directory */
# define FTS_DC		 2		/* directory that causes cycles */
# define FTS_DEFAULT	 3		/* none of the above */
# define FTS_DNR	 4		/* unreadable directory */
# define FTS_DOT	 5		/* dot or dot-dot */
# define FTS_DP		 6		/* postorder directory */
# define FTS_ERR	 7		/* error; errno is set */
# define FTS_F		 8		/* regular file */
# define FTS_INIT	 9		/* initialized only */
# define FTS_NS		10		/* stat(2) failed */
# define FTS_NSOK	11		/* no stat(2) requested */
# define FTS_SL		12		/* symbolic link */
# define FTS_SLNONE	13		/* symbolic link without target */
# define FTS_W		14		/* whiteout object */
	unsigned short int fts_info;	/* user flags for FTSENT structure */

# define FTS_DONTCHDIR	 0x01		/* don't chdir .. to the parent */
# define FTS_SYMFOLLOW	 0x02		/* followed a symlink to get here */
	unsigned short int fts_flags;	/* private flags for FTSENT structure */

# define FTS_AGAIN	 1		/* read node again */
# define FTS_FOLLOW	 2		/* follow symbolic link */
# define FTS_NOINSTR	 3		/* no instructions */
# define FTS_SKIP	 4		/* discard node */
	unsigned short int fts_instr;	/* fts_set() instructions */

	struct stat fts_statp[1];	/* stat(2) information */
	char fts_name[1];		/* file name */
} FTSENT;

__BEGIN_DECLS
FTSENT	*fts_children (FTS *, int) __THROW;
int	 fts_close (FTS *) __THROW;
FTS	*fts_open (char * const *, int,
		   int (*)(const FTSENT **, const FTSENT **)) __THROW;
FTSENT	*fts_read (FTS *) __THROW;
int	 fts_set (FTS *, FTSENT *, int) __THROW;
__END_DECLS

#endif /* fts.h */
