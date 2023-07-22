/* copy.c -- core functions for copying files and directories
   Copyright (C) 1989-2023 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* Extracted from cp.c and librarified by Jim Meyering.  */

#include <config.h>
#include <stdckdint.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <selinux/selinux.h>

#if HAVE_HURD_H
# include <hurd.h>
#endif
#if HAVE_PRIV_H
# include <priv.h>
#endif

#include "system.h"
#include "acl.h"
#include "alignalloc.h"
#include "assure.h"
#include "backupfile.h"
#include "buffer-lcm.h"
#include "canonicalize.h"
#include "copy.h"
#include "cp-hash.h"
#include "fadvise.h"
#include "fcntl--.h"
#include "file-set.h"
#include "filemode.h"
#include "filenamecat.h"
#include "force-link.h"
#include "full-write.h"
#include "hash.h"
#include "hash-triple.h"
#include "ignore-value.h"
#include "ioblksize.h"
#include "quote.h"
#include "renameatu.h"
#include "root-uid.h"
#include "same.h"
#include "savedir.h"
#include "stat-size.h"
#include "stat-time.h"
#include "utimecmp.h"
#include "utimens.h"
#include "write-any-file.h"
#include "areadlink.h"
#include "yesno.h"
#include "selinux.h"

#ifndef USE_XATTR
# define USE_XATTR false
#endif

#if USE_XATTR
# include <attr/error_context.h>
# include <attr/libattr.h>
# include <stdarg.h>
# include "verror.h"
#endif

#if HAVE_LINUX_FALLOC_H
# include <linux/falloc.h>
#endif

/* See HAVE_FALLOCATE workaround when including this file.  */
#ifdef HAVE_LINUX_FS_H
# include <linux/fs.h>
#endif

#if !defined FICLONE && defined __linux__
# define FICLONE _IOW (0x94, 9, int)
#endif

#if HAVE_FCLONEFILEAT && !USE_XATTR
# include <sys/clonefile.h>
#endif

#ifndef HAVE_FCHOWN
# define HAVE_FCHOWN false
# define fchown(fd, uid, gid) (-1)
#endif

#ifndef USE_ACL
# define USE_ACL 0
#endif

#define SAME_OWNER(A, B) ((A).st_uid == (B).st_uid)
#define SAME_GROUP(A, B) ((A).st_gid == (B).st_gid)
#define SAME_OWNER_AND_GROUP(A, B) (SAME_OWNER (A, B) && SAME_GROUP (A, B))

/* LINK_FOLLOWS_SYMLINKS is tri-state; if it is -1, we don't know
   how link() behaves, so assume we can't hardlink symlinks in that case.  */
#if (defined HAVE_LINKAT && ! LINKAT_SYMLINK_NOTSUP) || ! LINK_FOLLOWS_SYMLINKS
# define CAN_HARDLINK_SYMLINKS 1
#else
# define CAN_HARDLINK_SYMLINKS 0
#endif

struct dir_list
{
  struct dir_list *parent;
  ino_t ino;
  dev_t dev;
};

/* Initial size of the cp.dest_info hash table.  */
#define DEST_INFO_INITIAL_CAPACITY 61

static bool copy_internal (char const *src_name, char const *dst_name,
                           int dst_dirfd, char const *dst_relname,
                           int nonexistent_dst, struct stat const *parent,
                           struct dir_list *ancestors,
                           const struct cp_options *x,
                           bool command_line_arg,
                           bool *first_dir_created_per_command_line_arg,
                           bool *copy_into_self,
                           bool *rename_succeeded);
static bool owner_failure_ok (struct cp_options const *x);

/* Pointers to the file names:  they're used in the diagnostic that is issued
   when we detect the user is trying to copy a directory into itself.  */
static char const *top_level_src_name;
static char const *top_level_dst_name;

enum copy_debug_val
  {
   COPY_DEBUG_UNKNOWN,
   COPY_DEBUG_NO,
   COPY_DEBUG_YES,
   COPY_DEBUG_EXTERNAL,
   COPY_DEBUG_EXTERNAL_INTERNAL,
   COPY_DEBUG_AVOIDED,
   COPY_DEBUG_UNSUPPORTED,
  };

/* debug info about the last file copy.  */
static struct copy_debug
{
  enum copy_debug_val offload;
  enum copy_debug_val reflink;
  enum copy_debug_val sparse_detection;
} copy_debug;

static const char*
copy_debug_string (enum copy_debug_val debug_val)
{
  switch (debug_val)
    {
    case COPY_DEBUG_NO: return "no";
    case COPY_DEBUG_YES: return "yes";
    case COPY_DEBUG_AVOIDED: return "avoided";
    case COPY_DEBUG_UNSUPPORTED: return "unsupported";
    default: return "unknown";
    }
}

static const char*
copy_debug_sparse_string (enum copy_debug_val debug_val)
{
  switch (debug_val)
    {
    case COPY_DEBUG_NO: return "no";
    case COPY_DEBUG_YES: return "zeros";
    case COPY_DEBUG_EXTERNAL: return "SEEK_HOLE";
    case COPY_DEBUG_EXTERNAL_INTERNAL: return "SEEK_HOLE + zeros";
    default: return "unknown";
    }
}

/* Print --debug output on standard output.  */
static void
emit_debug (const struct cp_options *x)
{
  if (! x->hard_link && ! x->symbolic_link && x->data_copy_required)
    printf ("copy offload: %s, reflink: %s, sparse detection: %s\n",
            copy_debug_string (copy_debug.offload),
            copy_debug_string (copy_debug.reflink),
            copy_debug_sparse_string (copy_debug.sparse_detection));
}

#ifndef DEV_FD_MIGHT_BE_CHR
# define DEV_FD_MIGHT_BE_CHR false
#endif

/* Act like fstat (DIRFD, FILENAME, ST, FLAGS), except when following
   symbolic links on Solaris-like systems, treat any character-special
   device like /dev/fd/0 as if it were the file it is open on.  */
static int
follow_fstatat (int dirfd, char const *filename, struct stat *st, int flags)
{
  int result = fstatat (dirfd, filename, st, flags);

  if (DEV_FD_MIGHT_BE_CHR && result == 0 && !(flags & AT_SYMLINK_NOFOLLOW)
      && S_ISCHR (st->st_mode))
    {
      static dev_t stdin_rdev;
      static signed char stdin_rdev_status;
      if (stdin_rdev_status == 0)
        {
          struct stat stdin_st;
          if (stat ("/dev/stdin", &stdin_st) == 0 && S_ISCHR (stdin_st.st_mode)
              && minor (stdin_st.st_rdev) == STDIN_FILENO)
            {
              stdin_rdev = stdin_st.st_rdev;
              stdin_rdev_status = 1;
            }
          else
            stdin_rdev_status = -1;
        }
      if (0 < stdin_rdev_status && major (stdin_rdev) == major (st->st_rdev))
        result = fstat (minor (st->st_rdev), st);
    }

  return result;
}

/* Attempt to punch a hole to avoid any permanent
   speculative preallocation on file systems such as XFS.
   Return values as per fallocate(2) except ENOSYS etc. are ignored.  */

static int
punch_hole (int fd, off_t offset, off_t length)
{
  int ret = 0;
/* +0 is to work around older <linux/fs.h> defining HAVE_FALLOCATE to empty.  */
#if HAVE_FALLOCATE + 0
# if defined FALLOC_FL_PUNCH_HOLE && defined FALLOC_FL_KEEP_SIZE
  ret = fallocate (fd, FALLOC_FL_PUNCH_HOLE | FALLOC_FL_KEEP_SIZE,
                   offset, length);
  if (ret < 0 && (is_ENOTSUP (errno) || errno == ENOSYS))
    ret = 0;
# endif
#endif
  return ret;
}

/* Create a hole at the end of a file,
   avoiding preallocation if requested.  */

static bool
create_hole (int fd, char const *name, bool punch_holes, off_t size)
{
  off_t file_end = lseek (fd, size, SEEK_CUR);

  if (file_end < 0)
    {
      error (0, errno, _("cannot lseek %s"), quoteaf (name));
      return false;
    }

  /* Some file systems (like XFS) preallocate when write extending a file.
     I.e., a previous write() may have preallocated extra space
     that the seek above will not discard.  A subsequent write() could
     then make this allocation permanent.  */
  if (punch_holes && punch_hole (fd, file_end - size, size) < 0)
    {
      error (0, errno, _("error deallocating %s"), quoteaf (name));
      return false;
    }

  return true;
}


/* Whether an errno value ERR, set by FICLONE or copy_file_range,
   indicates that the copying operation has terminally failed, even
   though it was invoked correctly (so that, e.g, EBADF cannot occur)
   and even though !is_CLONENOTSUP (ERR).  */

static bool
is_terminal_error (int err)
{
  return err == EIO || err == ENOMEM || err == ENOSPC || err == EDQUOT;
}

/* Similarly, whether ERR indicates that the copying operation is not
   supported or allowed for this file or process, even though the
   operation was invoked correctly.  */

static bool
is_CLONENOTSUP (int err)
{
  return err == ENOSYS || err == ENOTTY || is_ENOTSUP (err)
         || err == EINVAL || err == EBADF
         || err == EXDEV || err == ETXTBSY
         || err == EPERM || err == EACCES;
}


/* Copy the regular file open on SRC_FD/SRC_NAME to DST_FD/DST_NAME,
   honoring the MAKE_HOLES setting and using the BUF_SIZE-byte buffer
   *ABUF for temporary storage, allocating it lazily if *ABUF is null.
   Copy no more than MAX_N_READ bytes.
   Return true upon successful completion;
   print a diagnostic and return false upon error.
   Note that for best results, BUF should be "well"-aligned.
   Set *LAST_WRITE_MADE_HOLE to true if the final operation on
   DEST_FD introduced a hole.  Set *TOTAL_N_READ to the number of
   bytes read.  */
static bool
sparse_copy (int src_fd, int dest_fd, char **abuf, size_t buf_size,
             size_t hole_size, bool punch_holes, bool allow_reflink,
             char const *src_name, char const *dst_name,
             uintmax_t max_n_read, off_t *total_n_read,
             bool *last_write_made_hole)
{
  *last_write_made_hole = false;
  *total_n_read = 0;

  if (copy_debug.sparse_detection == COPY_DEBUG_UNKNOWN)
    copy_debug.sparse_detection = hole_size ? COPY_DEBUG_YES : COPY_DEBUG_NO;
  else if (hole_size && copy_debug.sparse_detection == COPY_DEBUG_EXTERNAL)
    copy_debug.sparse_detection = COPY_DEBUG_EXTERNAL_INTERNAL;

  /* If not looking for holes, use copy_file_range if functional,
     but don't use if reflink disallowed as that may be implicit.  */
  if (!hole_size && allow_reflink)
    while (max_n_read)
      {
        /* Copy at most COPY_MAX bytes at a time; this is min
           (SSIZE_MAX, SIZE_MAX) truncated to a value that is
           surely aligned well.  */
        ssize_t copy_max = MIN (SSIZE_MAX, SIZE_MAX) >> 30 << 30;
        ssize_t n_copied = copy_file_range (src_fd, nullptr, dest_fd, nullptr,
                                            MIN (max_n_read, copy_max), 0);
        if (n_copied == 0)
          {
            /* copy_file_range incorrectly returns 0 when reading from
               the proc file system on the Linux kernel through at
               least 5.6.19 (2020), so fall back on 'read' if the
               input file seems empty.  */
            if (*total_n_read == 0)
              break;
            copy_debug.offload = COPY_DEBUG_YES;
            return true;
          }
        if (n_copied < 0)
          {
            copy_debug.offload = COPY_DEBUG_UNSUPPORTED;

            /* Consider operation unsupported only if no data copied.
               For example, EPERM could occur if copy_file_range not enabled
               in seccomp filters, so retry with a standard copy.  EPERM can
               also occur for immutable files, but that would only be in the
               edge case where the file is made immutable after creating,
               in which case the (more accurate) error is still shown.  */
            if (*total_n_read == 0 && is_CLONENOTSUP (errno))
              break;

            /* ENOENT was seen sometimes across CIFS shares, resulting in
               no data being copied, but subsequent standard copies succeed.  */
            if (*total_n_read == 0 && errno == ENOENT)
              break;

            if (errno == EINTR)
              n_copied = 0;
            else
              {
                error (0, errno, _("error copying %s to %s"),
                       quoteaf_n (0, src_name), quoteaf_n (1, dst_name));
                return false;
              }
          }
        copy_debug.offload = COPY_DEBUG_YES;
        max_n_read -= n_copied;
        *total_n_read += n_copied;
      }
  else
    copy_debug.offload = COPY_DEBUG_AVOIDED;


  bool make_hole = false;
  off_t psize = 0;

  while (max_n_read)
    {
      if (!*abuf)
        *abuf = xalignalloc (getpagesize (), buf_size);
      char *buf = *abuf;
      ssize_t n_read = read (src_fd, buf, MIN (max_n_read, buf_size));
      if (n_read < 0)
        {
          if (errno == EINTR)
            continue;
          error (0, errno, _("error reading %s"), quoteaf (src_name));
          return false;
        }
      if (n_read == 0)
        break;
      max_n_read -= n_read;
      *total_n_read += n_read;

      /* Loop over the input buffer in chunks of hole_size.  */
      size_t csize = hole_size ? hole_size : buf_size;
      char *cbuf = buf;
      char *pbuf = buf;

      while (n_read)
        {
          bool prev_hole = make_hole;
          csize = MIN (csize, n_read);

          if (hole_size && csize)
            make_hole = is_nul (cbuf, csize);

          bool transition = (make_hole != prev_hole) && psize;
          bool last_chunk = (n_read == csize && ! make_hole) || ! csize;

          if (transition || last_chunk)
            {
              if (! transition)
                psize += csize;

              if (! prev_hole)
                {
                  if (full_write (dest_fd, pbuf, psize) != psize)
                    {
                      error (0, errno, _("error writing %s"),
                             quoteaf (dst_name));
                      return false;
                    }
                }
              else
                {
                  if (! create_hole (dest_fd, dst_name, punch_holes, psize))
                    return false;
                }

              pbuf = cbuf;
              psize = csize;

              if (last_chunk)
                {
                  if (! csize)
                    n_read = 0; /* Finished processing buffer.  */

                  if (transition)
                    csize = 0;  /* Loop again to deal with last chunk.  */
                  else
                    psize = 0;  /* Reset for next read loop.  */
                }
            }
          else  /* Coalesce writes/seeks.  */
            {
              if (ckd_add (&psize, psize, csize))
                {
                  error (0, 0, _("overflow reading %s"), quoteaf (src_name));
                  return false;
                }
            }

          n_read -= csize;
          cbuf += csize;
        }

      *last_write_made_hole = make_hole;

      /* It's tempting to break early here upon a short read from
         a regular file.  That would save the final read syscall
         for each file.  Unfortunately that doesn't work for
         certain files in /proc or /sys with linux kernels.  */
    }

  /* Ensure a trailing hole is created, so that subsequent
     calls of sparse_copy() start at the correct offset.  */
  if (make_hole && ! create_hole (dest_fd, dst_name, punch_holes, psize))
    return false;
  else
    return true;
}

/* Perform the O(1) btrfs clone operation, if possible.
   Upon success, return 0.  Otherwise, return -1 and set errno.  */
static inline int
clone_file (int dest_fd, int src_fd)
{
#ifdef FICLONE
  return ioctl (dest_fd, FICLONE, src_fd);
#else
  (void) dest_fd;
  (void) src_fd;
  errno = ENOTSUP;
  return -1;
#endif
}

/* Write N_BYTES zero bytes to file descriptor FD.  Return true if successful.
   Upon write failure, set errno and return false.  */
static bool
write_zeros (int fd, off_t n_bytes)
{
  static char *zeros;
  static size_t nz = IO_BUFSIZE;

  /* Attempt to use a relatively large calloc'd source buffer for
     efficiency, but if that allocation fails, resort to a smaller
     statically allocated one.  */
  if (zeros == nullptr)
    {
      static char fallback[1024];
      zeros = calloc (nz, 1);
      if (zeros == nullptr)
        {
          zeros = fallback;
          nz = sizeof fallback;
        }
    }

  while (n_bytes)
    {
      size_t n = MIN (nz, n_bytes);
      if ((full_write (fd, zeros, n)) != n)
        return false;
      n_bytes -= n;
    }

  return true;
}

#ifdef SEEK_HOLE
/* Perform an efficient extent copy, if possible.  This avoids
   the overhead of detecting holes in hole-introducing/preserving
   copy, and thus makes copying sparse files much more efficient.
   Copy from SRC_FD to DEST_FD, using *ABUF (of size BUF_SIZE) for a buffer.
   Allocate *ABUF lazily if *ABUF is null.
   Look for holes of size HOLE_SIZE in the input.
   The input file is of size SRC_TOTAL_SIZE.
   Use SPARSE_MODE to determine whether to create holes in the output.
   SRC_NAME and DST_NAME are the input and output file names.
   Return true if successful, false (with a diagnostic) otherwise.  */

static bool
lseek_copy (int src_fd, int dest_fd, char **abuf, size_t buf_size,
            size_t hole_size, off_t ext_start, off_t src_total_size,
            enum Sparse_type sparse_mode,
            bool allow_reflink,
            char const *src_name, char const *dst_name)
{
  off_t last_ext_start = 0;
  off_t last_ext_len = 0;
  off_t dest_pos = 0;
  bool wrote_hole_at_eof = true;

  copy_debug.sparse_detection = COPY_DEBUG_EXTERNAL;

  while (0 <= ext_start)
    {
      off_t ext_end = lseek (src_fd, ext_start, SEEK_HOLE);
      if (ext_end < 0)
        {
          if (errno != ENXIO)
            goto cannot_lseek;
          ext_end = src_total_size;
          if (ext_end <= ext_start)
            {
              /* The input file grew; get its current size.  */
              src_total_size = lseek (src_fd, 0, SEEK_END);
              if (src_total_size < 0)
                goto cannot_lseek;

              /* If the input file shrank after growing, stop copying.  */
              if (src_total_size <= ext_start)
                break;

              ext_end = src_total_size;
            }
        }
      /* If the input file must have grown, increase its measured size.  */
      if (src_total_size < ext_end)
        src_total_size = ext_end;

      if (lseek (src_fd, ext_start, SEEK_SET) < 0)
        goto cannot_lseek;

      wrote_hole_at_eof = false;
      off_t ext_hole_size = ext_start - last_ext_start - last_ext_len;

      if (ext_hole_size)
        {
          if (sparse_mode != SPARSE_NEVER)
            {
              if (! create_hole (dest_fd, dst_name,
                                 sparse_mode == SPARSE_ALWAYS,
                                 ext_hole_size))
                return false;
              wrote_hole_at_eof = true;
            }
          else
            {
              /* When not inducing holes and when there is a hole between
                 the end of the previous extent and the beginning of the
                 current one, write zeros to the destination file.  */
              if (! write_zeros (dest_fd, ext_hole_size))
                {
                  error (0, errno, _("%s: write failed"),
                         quotef (dst_name));
                  return false;
                }
            }
        }

      off_t ext_len = ext_end - ext_start;
      last_ext_start = ext_start;
      last_ext_len = ext_len;

      /* Copy this extent, looking for further opportunities to not
         bother to write zeros if --sparse=always, since SEEK_HOLE
         is conservative and may miss some holes.  */
      off_t n_read;
      bool read_hole;
      if ( ! sparse_copy (src_fd, dest_fd, abuf, buf_size,
                          sparse_mode != SPARSE_ALWAYS ? 0 : hole_size,
                          true, allow_reflink, src_name, dst_name,
                          ext_len, &n_read, &read_hole))
        return false;

      dest_pos = ext_start + n_read;
      if (n_read)
        wrote_hole_at_eof = read_hole;
      if (n_read < ext_len)
        {
          /* The input file shrank.  */
          src_total_size = dest_pos;
          break;
        }

      ext_start = lseek (src_fd, dest_pos, SEEK_DATA);
      if (ext_start < 0 && errno != ENXIO)
        goto cannot_lseek;
    }

  /* When the source file ends with a hole, we have to do a little more work,
     since the above copied only up to and including the final extent.
     In order to complete the copy, we may have to insert a hole or write
     zeros in the destination corresponding to the source file's hole-at-EOF.

     In addition, if the final extent was a block of zeros at EOF and we've
     just converted them to a hole in the destination, we must call ftruncate
     here in order to record the proper length in the destination.  */
  if ((dest_pos < src_total_size || wrote_hole_at_eof)
      && ! (sparse_mode == SPARSE_NEVER
            ? write_zeros (dest_fd, src_total_size - dest_pos)
            : ftruncate (dest_fd, src_total_size) == 0))
    {
      error (0, errno, _("failed to extend %s"), quoteaf (dst_name));
      return false;
    }

  if (sparse_mode == SPARSE_ALWAYS && dest_pos < src_total_size
      && punch_hole (dest_fd, dest_pos, src_total_size - dest_pos) < 0)
    {
      error (0, errno, _("error deallocating %s"), quoteaf (dst_name));
      return false;
    }

  return true;

 cannot_lseek:
  error (0, errno, _("cannot lseek %s"), quoteaf (src_name));
  return false;
}
#endif

/* FIXME: describe */
/* FIXME: rewrite this to use a hash table so we avoid the quadratic
   performance hit that's probably noticeable only on trees deeper
   than a few hundred levels.  See use of active_dir_map in remove.c  */

ATTRIBUTE_PURE
static bool
is_ancestor (const struct stat *sb, const struct dir_list *ancestors)
{
  while (ancestors != 0)
    {
      if (ancestors->ino == sb->st_ino && ancestors->dev == sb->st_dev)
        return true;
      ancestors = ancestors->parent;
    }
  return false;
}

static bool
errno_unsupported (int err)
{
  return err == ENOTSUP || err == ENODATA;
}

#if USE_XATTR
ATTRIBUTE_FORMAT ((printf, 2, 3))
static void
copy_attr_error (MAYBE_UNUSED struct error_context *ctx,
                 char const *fmt, ...)
{
  if (!errno_unsupported (errno))
    {
      int err = errno;
      va_list ap;

      /* use verror module to print error message */
      va_start (ap, fmt);
      verror (0, err, fmt, ap);
      va_end (ap);
    }
}

ATTRIBUTE_FORMAT ((printf, 2, 3))
static void
copy_attr_allerror (MAYBE_UNUSED struct error_context *ctx,
                    char const *fmt, ...)
{
  int err = errno;
  va_list ap;

  /* use verror module to print error message */
  va_start (ap, fmt);
  verror (0, err, fmt, ap);
  va_end (ap);
}

static char const *
copy_attr_quote (MAYBE_UNUSED struct error_context *ctx, char const *str)
{
  return quoteaf (str);
}

static void
copy_attr_free (MAYBE_UNUSED struct error_context *ctx,
                MAYBE_UNUSED char const *str)
{
}

/* Exclude SELinux extended attributes that are otherwise handled,
   and are problematic to copy again.  Also honor attributes
   configured for exclusion in /etc/xattr.conf.
   FIXME: Should we handle POSIX ACLs similarly?
   Return zero to skip.  */
static int
check_selinux_attr (char const *name, struct error_context *ctx)
{
  return STRNCMP_LIT (name, "security.selinux")
         && attr_copy_check_permissions (name, ctx);
}

/* If positive SRC_FD and DST_FD descriptors are passed,
   then copy by fd, otherwise copy by name.  */

static bool
copy_attr (char const *src_path, int src_fd,
           char const *dst_path, int dst_fd, struct cp_options const *x)
{
  bool all_errors = (!x->data_copy_required || x->require_preserve_xattr);
  bool some_errors = (!all_errors && !x->reduce_diagnostics);
  int (*check) (char const *, struct error_context *)
    = (x->preserve_security_context || x->set_security_context
       ? check_selinux_attr : nullptr);

# if 4 < __GNUC__ + (8 <= __GNUC_MINOR__)
  /* Pacify gcc -Wsuggest-attribute=format through at least GCC 11.2.1.  */
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wsuggest-attribute=format"
# endif
  struct error_context *ctx
    = (all_errors || some_errors
       ? (&(struct error_context) {
           .error = all_errors ? copy_attr_allerror : copy_attr_error,
           .quote = copy_attr_quote,
           .quote_free = copy_attr_free
         })
       : nullptr);
# if 4 < __GNUC__ + (8 <= __GNUC_MINOR__)
#  pragma GCC diagnostic pop
# endif

  return ! (0 <= src_fd && 0 <= dst_fd
            ? attr_copy_fd (src_path, src_fd, dst_path, dst_fd, check, ctx)
            : attr_copy_file (src_path, dst_path, check, ctx));
}
#else /* USE_XATTR */

static bool
copy_attr (MAYBE_UNUSED char const *src_path,
           MAYBE_UNUSED int src_fd,
           MAYBE_UNUSED char const *dst_path,
           MAYBE_UNUSED int dst_fd,
           MAYBE_UNUSED struct cp_options const *x)
{
  return true;
}
#endif /* USE_XATTR */

/* Read the contents of the directory SRC_NAME_IN, and recursively
   copy the contents to DST_NAME_IN aka DST_DIRFD+DST_RELNAME_IN.
   NEW_DST is true if DST_NAME_IN is a directory
   that was created previously in the recursion.
   SRC_SB and ANCESTORS describe SRC_NAME_IN.
   Set *COPY_INTO_SELF if SRC_NAME_IN is a parent of
   (or the same as) DST_NAME_IN; otherwise, clear it.
   Propagate *FIRST_DIR_CREATED_PER_COMMAND_LINE_ARG from
   caller to each invocation of copy_internal.  Be careful to
   pass the address of a temporary, and to update
   *FIRST_DIR_CREATED_PER_COMMAND_LINE_ARG only upon completion.
   Return true if successful.  */

static bool
copy_dir (char const *src_name_in, char const *dst_name_in,
          int dst_dirfd, char const *dst_relname_in, bool new_dst,
          const struct stat *src_sb, struct dir_list *ancestors,
          const struct cp_options *x,
          bool *first_dir_created_per_command_line_arg,
          bool *copy_into_self)
{
  char *name_space;
  char *namep;
  struct cp_options non_command_line_options = *x;
  bool ok = true;

  name_space = savedir (src_name_in, SAVEDIR_SORT_FASTREAD);
  if (name_space == nullptr)
    {
      /* This diagnostic is a bit vague because savedir can fail in
         several different ways.  */
      error (0, errno, _("cannot access %s"), quoteaf (src_name_in));
      return false;
    }

  /* For cp's -H option, dereference command line arguments, but do not
     dereference symlinks that are found via recursive traversal.  */
  if (x->dereference == DEREF_COMMAND_LINE_ARGUMENTS)
    non_command_line_options.dereference = DEREF_NEVER;

  bool new_first_dir_created = false;
  namep = name_space;
  while (*namep != '\0')
    {
      bool local_copy_into_self;
      char *src_name = file_name_concat (src_name_in, namep, nullptr);
      char *dst_name = file_name_concat (dst_name_in, namep, nullptr);
      bool first_dir_created = *first_dir_created_per_command_line_arg;
      bool rename_succeeded;

      ok &= copy_internal (src_name, dst_name, dst_dirfd,
                           dst_name + (dst_relname_in - dst_name_in),
                           new_dst, src_sb,
                           ancestors, &non_command_line_options, false,
                           &first_dir_created,
                           &local_copy_into_self, &rename_succeeded);
      *copy_into_self |= local_copy_into_self;

      free (dst_name);
      free (src_name);

      /* If we're copying into self, there's no point in continuing,
         and in fact, that would even infloop, now that we record only
         the first created directory per command line argument.  */
      if (local_copy_into_self)
        break;

      new_first_dir_created |= first_dir_created;
      namep += strlen (namep) + 1;
    }
  free (name_space);
  *first_dir_created_per_command_line_arg = new_first_dir_created;

  return ok;
}

/* Set the owner and owning group of DEST_DESC to the st_uid and
   st_gid fields of SRC_SB.  If DEST_DESC is undefined (-1), set
   the owner and owning group of DST_NAME aka DST_DIRFD+DST_RELNAME
   instead; for safety prefer lchownat since no
   symbolic links should be involved.  DEST_DESC must
   refer to the same file as DST_NAME if defined.
   Upon failure to set both UID and GID, try to set only the GID.
   NEW_DST is true if the file was newly created; otherwise,
   DST_SB is the status of the destination.
   Return 1 if the initial syscall succeeds, 0 if it fails but it's OK
   not to preserve ownership, -1 otherwise.  */

static int
set_owner (const struct cp_options *x, char const *dst_name,
           int dst_dirfd, char const *dst_relname, int dest_desc,
           struct stat const *src_sb, bool new_dst,
           struct stat const *dst_sb)
{
  uid_t uid = src_sb->st_uid;
  gid_t gid = src_sb->st_gid;

  /* Naively changing the ownership of an already-existing file before
     changing its permissions would create a window of vulnerability if
     the file's old permissions are too generous for the new owner and
     group.  Avoid the window by first changing to a restrictive
     temporary mode if necessary.  */

  if (!new_dst && (x->preserve_mode || x->move_mode || x->set_mode))
    {
      mode_t old_mode = dst_sb->st_mode;
      mode_t new_mode =
        (x->preserve_mode || x->move_mode ? src_sb->st_mode : x->mode);
      mode_t restrictive_temp_mode = old_mode & new_mode & S_IRWXU;

      if ((USE_ACL
           || (old_mode & CHMOD_MODE_BITS
               & (~new_mode | S_ISUID | S_ISGID | S_ISVTX)))
          && qset_acl (dst_name, dest_desc, restrictive_temp_mode) != 0)
        {
          if (! owner_failure_ok (x))
            error (0, errno, _("clearing permissions for %s"),
                   quoteaf (dst_name));
          return -x->require_preserve;
        }
    }

  if (HAVE_FCHOWN && dest_desc != -1)
    {
      if (fchown (dest_desc, uid, gid) == 0)
        return 1;
      if (errno == EPERM || errno == EINVAL)
        {
          /* We've failed to set *both*.  Now, try to set just the group
             ID, but ignore any failure here, and don't change errno.  */
          int saved_errno = errno;
          ignore_value (fchown (dest_desc, -1, gid));
          errno = saved_errno;
        }
    }
  else
    {
      if (lchownat (dst_dirfd, dst_relname, uid, gid) == 0)
        return 1;
      if (errno == EPERM || errno == EINVAL)
        {
          /* We've failed to set *both*.  Now, try to set just the group
             ID, but ignore any failure here, and don't change errno.  */
          int saved_errno = errno;
          ignore_value (lchownat (dst_dirfd, dst_relname, -1, gid));
          errno = saved_errno;
        }
    }

  if (! chown_failure_ok (x))
    {
      error (0, errno, _("failed to preserve ownership for %s"),
             quoteaf (dst_name));
      if (x->require_preserve)
        return -1;
    }

  return 0;
}

/* Set the st_author field of DEST_DESC to the st_author field of
   SRC_SB. If DEST_DESC is undefined (-1), set the st_author field
   of DST_NAME instead.  DEST_DESC must refer to the same file as
   DST_NAME if defined.  */

static void
set_author (char const *dst_name, int dest_desc, const struct stat *src_sb)
{
#if HAVE_STRUCT_STAT_ST_AUTHOR
  /* FIXME: Modify the following code so that it does not
     follow symbolic links.  */

  /* Preserve the st_author field.  */
  file_t file = (dest_desc < 0
                 ? file_name_lookup (dst_name, 0, 0)
                 : getdport (dest_desc));
  if (file == MACH_PORT_nullptr)
    error (0, errno, _("failed to lookup file %s"), quoteaf (dst_name));
  else
    {
      error_t err = file_chauthor (file, src_sb->st_author);
      if (err)
        error (0, err, _("failed to preserve authorship for %s"),
               quoteaf (dst_name));
      mach_port_deallocate (mach_task_self (), file);
    }
#else
  (void) dst_name;
  (void) dest_desc;
  (void) src_sb;
#endif
}

/* Set the default security context for the process.  New files will
   have this security context set.  Also existing files can have their
   context adjusted based on this process context, by
   set_file_security_ctx() called with PROCESS_LOCAL=true.
   This should be called before files are created so there is no race
   where a file may be present without an appropriate security context.
   Based on CP_OPTIONS, diagnose warnings and fail when appropriate.
   Return FALSE on failure, TRUE on success.  */

bool
set_process_security_ctx (char const *src_name, char const *dst_name,
                          mode_t mode, bool new_dst, const struct cp_options *x)
{
  if (x->preserve_security_context)
    {
      /* Set the default context for the process to match the source.  */
      bool all_errors = !x->data_copy_required || x->require_preserve_context;
      bool some_errors = !all_errors && !x->reduce_diagnostics;
      char *con;

      if (0 <= lgetfilecon (src_name, &con))
        {
          if (setfscreatecon (con) < 0)
            {
              if (all_errors || (some_errors && !errno_unsupported (errno)))
                error (0, errno,
                       _("failed to set default file creation context to %s"),
                       quote (con));
              if (x->require_preserve_context)
                {
                  freecon (con);
                  return false;
                }
            }
          freecon (con);
        }
      else
        {
          if (all_errors || (some_errors && !errno_unsupported (errno)))
            {
              error (0, errno,
                     _("failed to get security context of %s"),
                     quoteaf (src_name));
            }
          if (x->require_preserve_context)
            return false;
        }
    }
  else if (x->set_security_context)
    {
      /* With -Z, adjust the default context for the process
         to have the type component adjusted as per the destination path.  */
      if (new_dst && defaultcon (x->set_security_context, dst_name, mode) < 0
          && ! ignorable_ctx_err (errno))
        {
          error (0, errno,
                 _("failed to set default file creation context for %s"),
                 quoteaf (dst_name));
        }
    }

  return true;
}

/* Reset the security context of DST_NAME, to that already set
   as the process default if !X->set_security_context.  Otherwise
   adjust the type component of DST_NAME's security context as
   per the system default for that path.  Issue warnings upon
   failure, when allowed by various settings in X.
   Return false on failure, true on success.  */

bool
set_file_security_ctx (char const *dst_name,
                       bool recurse, const struct cp_options *x)
{
  bool all_errors = (!x->data_copy_required
                     || x->require_preserve_context);
  bool some_errors = !all_errors && !x->reduce_diagnostics;

  if (! restorecon (x->set_security_context, dst_name, recurse))
    {
      if (all_errors || (some_errors && !errno_unsupported (errno)))
        error (0, errno, _("failed to set the security context of %s"),
               quoteaf_n (0, dst_name));
      return false;
    }

  return true;
}

/* Change the file mode bits of the file identified by DESC or
   DIRFD+NAME to MODE.  Use DESC if DESC is valid and fchmod is
   available, DIRFD+NAME otherwise.  */

static int
fchmod_or_lchmod (int desc, int dirfd, char const *name, mode_t mode)
{
#if HAVE_FCHMOD
  if (0 <= desc)
    return fchmod (desc, mode);
#endif
  return lchmodat (dirfd, name, mode);
}

#ifndef HAVE_STRUCT_STAT_ST_BLOCKS
# define HAVE_STRUCT_STAT_ST_BLOCKS 0
#endif

/* Type of scan being done on the input when looking for sparseness.  */
enum scantype
  {
   /* An error was found when determining scantype.  */
   ERROR_SCANTYPE,

   /* No fancy scanning; just read and write.  */
   PLAIN_SCANTYPE,

   /* Read and examine data looking for zero blocks; useful when
      attempting to create sparse output.  */
   ZERO_SCANTYPE,

   /* lseek information is available.  */
   LSEEK_SCANTYPE,
  };

/* Result of infer_scantype.  */
union scan_inference
{
  /* Used if infer_scantype returns LSEEK_SCANTYPE.  This is the
     offset of the first data block, or -1 if the file has no data.  */
  off_t ext_start;
};

/* Return how to scan a file with descriptor FD and stat buffer SB.
   *SCAN_INFERENCE is set to a valid value if returning LSEEK_SCANTYPE.  */
static enum scantype
infer_scantype (int fd, struct stat const *sb,
                union scan_inference *scan_inference)
{
  scan_inference->ext_start = -1;  /* avoid -Wmaybe-uninitialized */

  if (! (HAVE_STRUCT_STAT_ST_BLOCKS
         && S_ISREG (sb->st_mode)
         && ST_NBLOCKS (*sb) < sb->st_size / ST_NBLOCKSIZE))
    return PLAIN_SCANTYPE;

#ifdef SEEK_HOLE
  off_t ext_start = lseek (fd, 0, SEEK_DATA);
  if (0 <= ext_start || errno == ENXIO)
    {
      scan_inference->ext_start = ext_start;
      return LSEEK_SCANTYPE;
    }
  else if (errno != EINVAL && !is_ENOTSUP (errno))
    return ERROR_SCANTYPE;
#endif

  return ZERO_SCANTYPE;
}

#if HAVE_FCLONEFILEAT && !USE_XATTR
# include <sys/acl.h>
/* Return true if FD has a nontrivial ACL.  */
static bool
fd_has_acl (int fd)
{
  /* Every platform with fclonefileat (macOS 10.12 or later) also has
     acl_get_fd_np.  */
  bool has_acl = false;
  acl_t acl = acl_get_fd_np (fd, ACL_TYPE_EXTENDED);
  if (acl)
    {
      acl_entry_t ace;
      has_acl = 0 <= acl_get_entry (acl, ACL_FIRST_ENTRY, &ace);
      acl_free (acl);
    }
  return has_acl;
}
#endif

/* Handle failure from FICLONE or fclonefileat.
   Return FALSE if it's a terminal failure for this file.  */

static bool
handle_clone_fail (int dst_dirfd, char const *dst_relname,
                   char const *src_name, char const *dst_name,
                   int dest_desc, bool new_dst, enum Reflink_type reflink_mode)
{
  /* When the clone operation fails, report failure only with errno values
     known to mean trouble when the clone is supported and called properly.
     Do not report failure merely because !is_CLONENOTSUP (errno),
     as systems may yield oddball errno values here with FICLONE,
     and is_CLONENOTSUP is not appropriate for fclonefileat.  */
  bool report_failure = is_terminal_error (errno);

  if (reflink_mode == REFLINK_ALWAYS || report_failure)
    error (0, errno, _("failed to clone %s from %s"),
           quoteaf_n (0, dst_name), quoteaf_n (1, src_name));

  /* Remove the destination if cp --reflink=always created it
     but cloned no data.  */
  if (new_dst /* currently not for fclonefileat().  */
      && reflink_mode == REFLINK_ALWAYS
      && ((! report_failure) || lseek (dest_desc, 0, SEEK_END) == 0)
      && unlinkat (dst_dirfd, dst_relname, 0) != 0 && errno != ENOENT)
    error (0, errno, _("cannot remove %s"), quoteaf (dst_name));

  if (! report_failure)
    copy_debug.reflink = COPY_DEBUG_UNSUPPORTED;

  if (reflink_mode == REFLINK_ALWAYS || report_failure)
    return false;

  return true;
}


/* Copy a regular file from SRC_NAME to DST_NAME aka DST_DIRFD+DST_RELNAME.
   If the source file contains holes, copies holes and blocks of zeros
   in the source file as holes in the destination file.
   (Holes are read as zeroes by the 'read' system call.)
   When creating the destination, use DST_MODE & ~OMITTED_PERMISSIONS
   as the third argument in the call to open, adding
   OMITTED_PERMISSIONS after copying as needed.
   X provides many option settings.
   Return true if successful.
   *NEW_DST is initially as in copy_internal.
   If successful, set *NEW_DST to true if the destination file was created and
   to false otherwise; if unsuccessful, perhaps set *NEW_DST to some value.
   SRC_SB is the result of calling follow_fstatat on SRC_NAME.  */

static bool
copy_reg (char const *src_name, char const *dst_name,
          int dst_dirfd, char const *dst_relname,
          const struct cp_options *x,
          mode_t dst_mode, mode_t omitted_permissions, bool *new_dst,
          struct stat const *src_sb)
{
  char *buf = nullptr;
  int dest_desc;
  int dest_errno;
  int source_desc;
  mode_t src_mode = src_sb->st_mode;
  mode_t extra_permissions;
  struct stat sb;
  struct stat src_open_sb;
  union scan_inference scan_inference;
  bool return_val = true;
  bool data_copy_required = x->data_copy_required;
  bool preserve_xattr = USE_XATTR & x->preserve_xattr;

  copy_debug.offload = COPY_DEBUG_UNKNOWN;
  copy_debug.reflink = x->reflink_mode ? COPY_DEBUG_UNKNOWN : COPY_DEBUG_NO;
  copy_debug.sparse_detection = COPY_DEBUG_UNKNOWN;

  source_desc = open (src_name,
                      (O_RDONLY | O_BINARY
                       | (x->dereference == DEREF_NEVER ? O_NOFOLLOW : 0)));
  if (source_desc < 0)
    {
      error (0, errno, _("cannot open %s for reading"), quoteaf (src_name));
      return false;
    }

  if (fstat (source_desc, &src_open_sb) != 0)
    {
      error (0, errno, _("cannot fstat %s"), quoteaf (src_name));
      return_val = false;
      goto close_src_desc;
    }

  /* Compare the source dev/ino from the open file to the incoming,
     saved ones obtained via a previous call to stat.  */
  if (! SAME_INODE (*src_sb, src_open_sb))
    {
      error (0, 0,
             _("skipping file %s, as it was replaced while being copied"),
             quoteaf (src_name));
      return_val = false;
      goto close_src_desc;
    }

  /* The semantics of the following open calls are mandated
     by the specs for both cp and mv.  */
  if (! *new_dst)
    {
      int open_flags =
        O_WRONLY | O_BINARY | (data_copy_required ? O_TRUNC : 0);
      dest_desc = openat (dst_dirfd, dst_relname, open_flags);
      dest_errno = errno;

      /* When using cp --preserve=context to copy to an existing destination,
         reset the context as per the default context, which has already been
         set according to the src.
         When using the mutually exclusive -Z option, then adjust the type of
         the existing context according to the system default for the dest.
         Note we set the context here, _after_ the file is opened, lest the
         new context disallow that.  */
      if (0 <= dest_desc
          && (x->set_security_context || x->preserve_security_context))
        {
          if (! set_file_security_ctx (dst_name, false, x))
            {
              if (x->require_preserve_context)
                {
                  return_val = false;
                  goto close_src_and_dst_desc;
                }
            }
        }

      if (dest_desc < 0 && dest_errno != ENOENT
          && x->unlink_dest_after_failed_open)
        {
          if (unlinkat (dst_dirfd, dst_relname, 0) == 0)
            {
              if (x->verbose)
                printf (_("removed %s\n"), quoteaf (dst_name));
            }
          else if (errno != ENOENT)
            {
              error (0, errno, _("cannot remove %s"), quoteaf (dst_name));
              return_val = false;
              goto close_src_desc;
            }

          dest_errno = ENOENT;
        }

      if (dest_desc < 0 && dest_errno == ENOENT)
        {
          /* Ensure there is no race where a file may be left without
             an appropriate security context.  */
          if (x->set_security_context)
            {
              if (! set_process_security_ctx (src_name, dst_name, dst_mode,
                                              true, x))
                {
                  return_val = false;
                  goto close_src_desc;
                }
            }

          /* Tell caller that the destination file is created.  */
          *new_dst = true;
        }
    }

  if (*new_dst)
    {
#if HAVE_FCLONEFILEAT && !USE_XATTR
# ifndef CLONE_ACL
#  define CLONE_ACL 0 /* Added in macOS 12.6.  */
# endif
# ifndef CLONE_NOOWNERCOPY
#  define CLONE_NOOWNERCOPY 0 /* Added in macOS 10.13.  */
# endif
      /* Try fclonefileat if copying data in reflink mode.
         Use CLONE_NOFOLLOW to avoid security issues that could occur
         if writing through dangling symlinks.  Although the circa
         2023 macOS documentation doesn't say so, CLONE_NOFOLLOW
         affects the destination file too.  */
      if (data_copy_required && x->reflink_mode
          && (CLONE_NOOWNERCOPY || x->preserve_ownership))
        {
          /* Try fclonefileat so long as it won't create the
             destination with unwanted permissions, which could lead
             to a security race.  */
          mode_t cloned_mode_bits = S_ISVTX | S_IRWXUGO;
          mode_t cloned_mode = src_mode & cloned_mode_bits;
          mode_t desired_mode
            = (x->preserve_mode ? src_mode & CHMOD_MODE_BITS
               : x->set_mode ? x->mode
               : ((x->explicit_no_preserve_mode ? MODE_RW_UGO : dst_mode)
                  & ~ cached_umask ()));
          if (! (cloned_mode & ~desired_mode))
            {
              int fc_flags
                = (CLONE_NOFOLLOW
                   | (x->preserve_mode ? CLONE_ACL : 0)
                   | (x->preserve_ownership ? 0 : CLONE_NOOWNERCOPY));
              int s = fclonefileat (source_desc, dst_dirfd, dst_relname,
                                    fc_flags);
              if (s != 0 && (fc_flags & CLONE_ACL) && errno == EINVAL)
                {
                  fc_flags &= ~CLONE_ACL;
                  s = fclonefileat (source_desc, dst_dirfd, dst_relname,
                                    fc_flags);
                }
              if (s == 0)
                {
                  copy_debug.reflink = COPY_DEBUG_YES;

                  /* Update the clone's timestamps and permissions
                     as needed.  */

                  if (!x->preserve_timestamps)
                    {
                      struct timespec timespec[2];
                      timespec[0].tv_nsec = timespec[1].tv_nsec = UTIME_NOW;
                      if (utimensat (dst_dirfd, dst_relname, timespec,
                                     AT_SYMLINK_NOFOLLOW)
                          != 0)
                        {
                          error (0, errno, _("updating times for %s"),
                                 quoteaf (dst_name));
                          return_val = false;
                          goto close_src_desc;
                        }
                    }

                  extra_permissions = desired_mode & ~cloned_mode;
                  if (!extra_permissions
                      && (!x->preserve_mode || (fc_flags & CLONE_ACL)
                          || !fd_has_acl (source_desc)))
                    {
                      goto close_src_desc;
                    }

                  /* Either some desired permissions were not cloned,
                     or ACLs were not cloned despite that being requested.  */
                  omitted_permissions = 0;
                  dest_desc = -1;
                  goto set_dest_mode;
                }
              if (! handle_clone_fail (dst_dirfd, dst_relname, src_name,
                                       dst_name,
                                       -1, false /* We didn't create dst  */,
                                       x->reflink_mode))
                {
                  return_val = false;
                  goto close_src_desc;
                }
            }
          else
            copy_debug.reflink = COPY_DEBUG_AVOIDED;
        }
      else if (data_copy_required && x->reflink_mode)
        {
          if (! CLONE_NOOWNERCOPY)
            copy_debug.reflink = COPY_DEBUG_AVOIDED;
        }
#endif

      /* To allow copying xattrs on read-only files, create with u+w.
         This satisfies an inode permission check done by
         xattr_permission in fs/xattr.c of the GNU/Linux kernel.  */
      mode_t open_mode =
        ((dst_mode & ~omitted_permissions)
         | (preserve_xattr && !x->owner_privileges ? S_IWUSR : 0));
      extra_permissions = open_mode & ~dst_mode; /* either 0 or S_IWUSR */

      int open_flags = O_WRONLY | O_CREAT | O_BINARY;
      dest_desc = openat (dst_dirfd, dst_relname, open_flags | O_EXCL,
                          open_mode);
      dest_errno = errno;

      /* When trying to copy through a dangling destination symlink,
         the above open fails with EEXIST.  If that happens, and
         readlinkat shows that it is a symlink, then we
         have a problem: trying to resolve this dangling symlink to
         a directory/destination-entry pair is fundamentally racy,
         so punt.  If x->open_dangling_dest_symlink is set (cp sets
         that when POSIXLY_CORRECT is set in the environment), simply
         call open again, but without O_EXCL (potentially dangerous).
         If not, fail with a diagnostic.  These shenanigans are necessary
         only when copying, i.e., not in move_mode.  */
      if (dest_desc < 0 && dest_errno == EEXIST && ! x->move_mode)
        {
          char dummy[1];
          if (0 <= readlinkat (dst_dirfd, dst_relname, dummy, sizeof dummy))
            {
              if (x->open_dangling_dest_symlink)
                {
                  dest_desc = openat (dst_dirfd, dst_relname,
                                      open_flags, open_mode);
                  dest_errno = errno;
                }
              else
                {
                  error (0, 0, _("not writing through dangling symlink %s"),
                         quoteaf (dst_name));
                  return_val = false;
                  goto close_src_desc;
                }
            }
        }

      /* Improve quality of diagnostic when a nonexistent dst_name
         ends in a slash and open fails with errno == EISDIR.  */
      if (dest_desc < 0 && dest_errno == EISDIR
          && *dst_name && dst_name[strlen (dst_name) - 1] == '/')
        dest_errno = ENOTDIR;
    }
  else
    {
      omitted_permissions = extra_permissions = 0;
    }

  if (dest_desc < 0)
    {
      error (0, dest_errno, _("cannot create regular file %s"),
             quoteaf (dst_name));
      return_val = false;
      goto close_src_desc;
    }

  /* --attributes-only overrides --reflink.  */
  if (data_copy_required && x->reflink_mode)
    {
      if (clone_file (dest_desc, source_desc) == 0)
        {
          data_copy_required = false;
          copy_debug.reflink = COPY_DEBUG_YES;
        }
      else
        {
          if (! handle_clone_fail (dst_dirfd, dst_relname, src_name, dst_name,
                                   dest_desc, *new_dst, x->reflink_mode))
           {
             return_val = false;
             goto close_src_and_dst_desc;
           }
        }
    }

  if (! (data_copy_required | x->preserve_ownership | extra_permissions))
    sb.st_mode = 0;
  else if (fstat (dest_desc, &sb) != 0)
    {
      error (0, errno, _("cannot fstat %s"), quoteaf (dst_name));
      return_val = false;
      goto close_src_and_dst_desc;
    }

  /* If extra permissions needed for copy_xattr didn't happen (e.g.,
     due to umask) chmod to add them temporarily; if that fails give
     up with extra permissions, letting copy_attr fail later.  */
  mode_t temporary_mode = sb.st_mode | extra_permissions;
  if (temporary_mode != sb.st_mode
      && (fchmod_or_lchmod (dest_desc, dst_dirfd, dst_relname, temporary_mode)
          != 0))
    extra_permissions = 0;

  if (data_copy_required)
    {
      /* Choose a suitable buffer size; it may be adjusted later.  */
      size_t buf_size = io_blksize (sb);
      size_t hole_size = ST_BLKSIZE (sb);

      /* Deal with sparse files.  */
      enum scantype scantype = infer_scantype (source_desc, &src_open_sb,
                                               &scan_inference);
      if (scantype == ERROR_SCANTYPE)
        {
          error (0, errno, _("cannot lseek %s"), quoteaf (src_name));
          return_val = false;
          goto close_src_and_dst_desc;
        }
      bool make_holes
        = (S_ISREG (sb.st_mode)
           && (x->sparse_mode == SPARSE_ALWAYS
               || (x->sparse_mode == SPARSE_AUTO
                   && scantype != PLAIN_SCANTYPE)));

      fdadvise (source_desc, 0, 0, FADVISE_SEQUENTIAL);

      /* If not making a sparse file, try to use a more-efficient
         buffer size.  */
      if (! make_holes)
        {
          /* Compute the least common multiple of the input and output
             buffer sizes, adjusting for outlandish values.
             Note we read in multiples of the reported block size
             to support (unusual) devices that have this constraint.  */
          size_t blcm_max = MIN (SIZE_MAX, SSIZE_MAX);
          size_t blcm = buffer_lcm (io_blksize (src_open_sb), buf_size,
                                    blcm_max);

          /* Do not bother with a buffer larger than the input file, plus one
             byte to make sure the file has not grown while reading it.  */
          if (S_ISREG (src_open_sb.st_mode) && src_open_sb.st_size < buf_size)
            buf_size = src_open_sb.st_size + 1;

          /* However, stick with a block size that is a positive multiple of
             blcm, overriding the above adjustments.  Watch out for
             overflow.  */
          buf_size += blcm - 1;
          buf_size -= buf_size % blcm;
          if (buf_size == 0 || blcm_max < buf_size)
            buf_size = blcm;
        }

      off_t n_read;
      bool wrote_hole_at_eof = false;
      if (! (
#ifdef SEEK_HOLE
             scantype == LSEEK_SCANTYPE
             ? lseek_copy (source_desc, dest_desc, &buf, buf_size, hole_size,
                           scan_inference.ext_start, src_open_sb.st_size,
                           make_holes ? x->sparse_mode : SPARSE_NEVER,
                           x->reflink_mode != REFLINK_NEVER,
                           src_name, dst_name)
             :
#endif
               sparse_copy (source_desc, dest_desc, &buf, buf_size,
                            make_holes ? hole_size : 0,
                            x->sparse_mode == SPARSE_ALWAYS,
                            x->reflink_mode != REFLINK_NEVER,
                            src_name, dst_name, UINTMAX_MAX, &n_read,
                            &wrote_hole_at_eof)))
        {
          return_val = false;
          goto close_src_and_dst_desc;
        }
      else if (wrote_hole_at_eof && ftruncate (dest_desc, n_read) < 0)
        {
          error (0, errno, _("failed to extend %s"), quoteaf (dst_name));
          return_val = false;
          goto close_src_and_dst_desc;
        }
    }

  if (x->preserve_timestamps)
    {
      struct timespec timespec[2];
      timespec[0] = get_stat_atime (src_sb);
      timespec[1] = get_stat_mtime (src_sb);

      if (fdutimensat (dest_desc, dst_dirfd, dst_relname, timespec, 0) != 0)
        {
          error (0, errno, _("preserving times for %s"), quoteaf (dst_name));
          if (x->require_preserve)
            {
              return_val = false;
              goto close_src_and_dst_desc;
            }
        }
    }

  /* Set ownership before xattrs as changing owners will
     clear capabilities.  */
  if (x->preserve_ownership && ! SAME_OWNER_AND_GROUP (*src_sb, sb))
    {
      switch (set_owner (x, dst_name, dst_dirfd, dst_relname, dest_desc,
                         src_sb, *new_dst, &sb))
        {
        case -1:
          return_val = false;
          goto close_src_and_dst_desc;

        case 0:
          src_mode &= ~ (S_ISUID | S_ISGID | S_ISVTX);
          break;
        }
    }

  if (preserve_xattr)
    {
      if (!copy_attr (src_name, source_desc, dst_name, dest_desc, x)
          && x->require_preserve_xattr)
        return_val = false;
    }

  set_author (dst_name, dest_desc, src_sb);

#if HAVE_FCLONEFILEAT && !USE_XATTR
set_dest_mode:
#endif
  if (x->preserve_mode || x->move_mode)
    {
      if (copy_acl (src_name, source_desc, dst_name, dest_desc, src_mode) != 0
          && x->require_preserve)
        return_val = false;
    }
  else if (x->set_mode)
    {
      if (set_acl (dst_name, dest_desc, x->mode) != 0)
        return_val = false;
    }
  else if (x->explicit_no_preserve_mode && *new_dst)
    {
      if (set_acl (dst_name, dest_desc, MODE_RW_UGO & ~cached_umask ()) != 0)
        return_val = false;
    }
  else if (omitted_permissions | extra_permissions)
    {
      omitted_permissions &= ~ cached_umask ();
      if ((omitted_permissions | extra_permissions)
          && (fchmod_or_lchmod (dest_desc, dst_dirfd, dst_relname,
                                dst_mode & ~ cached_umask ())
              != 0))
        {
          error (0, errno, _("preserving permissions for %s"),
                 quoteaf (dst_name));
          if (x->require_preserve)
            return_val = false;
        }
    }

  if (dest_desc < 0)
    goto close_src_desc;

close_src_and_dst_desc:
  if (close (dest_desc) < 0)
    {
      error (0, errno, _("failed to close %s"), quoteaf (dst_name));
      return_val = false;
    }
close_src_desc:
  if (close (source_desc) < 0)
    {
      error (0, errno, _("failed to close %s"), quoteaf (src_name));
      return_val = false;
    }

  /* Output debug info for data copying operations.  */
  if (x->debug)
    emit_debug (x);

  alignfree (buf);
  return return_val;
}

/* Return whether it's OK that two files are the "same" by some measure.
   The first file is SRC_NAME and has status SRC_SB.
   The second is DST_DIRFD+DST_RELNAME and has status DST_SB.
   The copying options are X.  The goal is to avoid
   making the 'copy' operation remove both copies of the file
   in that case, while still allowing the user to e.g., move or
   copy a regular file onto a symlink that points to it.
   Try to minimize the cost of this function in the common case.
   Set *RETURN_NOW if we've determined that the caller has no more
   work to do and should return successfully, right away.  */

static bool
same_file_ok (char const *src_name, struct stat const *src_sb,
              int dst_dirfd, char const *dst_relname, struct stat const *dst_sb,
              const struct cp_options *x, bool *return_now)
{
  const struct stat *src_sb_link;
  const struct stat *dst_sb_link;
  struct stat tmp_dst_sb;
  struct stat tmp_src_sb;

  bool same_link;
  bool same = SAME_INODE (*src_sb, *dst_sb);

  *return_now = false;

  /* FIXME: this should (at the very least) be moved into the following
     if-block.  More likely, it should be removed, because it inhibits
     making backups.  But removing it will result in a change in behavior
     that will probably have to be documented -- and tests will have to
     be updated.  */
  if (same && x->hard_link)
    {
      *return_now = true;
      return true;
    }

  if (x->dereference == DEREF_NEVER)
    {
      same_link = same;

      /* If both the source and destination files are symlinks (and we'll
         know this here IFF preserving symlinks), then it's usually ok
         when they are distinct.  */
      if (S_ISLNK (src_sb->st_mode) && S_ISLNK (dst_sb->st_mode))
        {
          bool sn = same_nameat (AT_FDCWD, src_name, dst_dirfd, dst_relname);
          if ( ! sn)
            {
              /* It's fine when we're making any type of backup.  */
              if (x->backup_type != no_backups)
                return true;

              /* Here we have two symlinks that are hard-linked together,
                 and we're not making backups.  In this unusual case, simply
                 returning true would lead to mv calling "rename(A,B)",
                 which would do nothing and return 0.  */
              if (same_link)
                {
                  *return_now = true;
                  return ! x->move_mode;
                }
            }

          return ! sn;
        }

      src_sb_link = src_sb;
      dst_sb_link = dst_sb;
    }
  else
    {
      if (!same)
        return true;

      if (fstatat (dst_dirfd, dst_relname, &tmp_dst_sb,
                   AT_SYMLINK_NOFOLLOW) != 0
          || lstat (src_name, &tmp_src_sb) != 0)
        return true;

      src_sb_link = &tmp_src_sb;
      dst_sb_link = &tmp_dst_sb;

      same_link = SAME_INODE (*src_sb_link, *dst_sb_link);

      /* If both are symlinks, then it's ok, but only if the destination
         will be unlinked before being opened.  This is like the test
         above, but with the addition of the unlink_dest_before_opening
         conjunct because otherwise, with two symlinks to the same target,
         we'd end up truncating the source file.  */
      if (S_ISLNK (src_sb_link->st_mode) && S_ISLNK (dst_sb_link->st_mode)
          && x->unlink_dest_before_opening)
        return true;
    }

  /* The backup code ensures there's a copy, so it's usually ok to
     remove any destination file.  One exception is when both
     source and destination are the same directory entry.  In that
     case, moving the destination file aside (in making the backup)
     would also rename the source file and result in an error.  */
  if (x->backup_type != no_backups)
    {
      if (!same_link)
        {
          /* In copy mode when dereferencing symlinks, if the source is a
             symlink and the dest is not, then backing up the destination
             (moving it aside) would make it a dangling symlink, and the
             subsequent attempt to open it in copy_reg would fail with
             a misleading diagnostic.  Avoid that by returning zero in
             that case so the caller can make cp (or mv when it has to
             resort to reading the source file) fail now.  */

          /* FIXME-note: even with the following kludge, we can still provoke
             the offending diagnostic.  It's just a little harder to do :-)
             $ rm -f a b c; touch c; ln -s c b; ln -s b a; cp -b a b
             cp: cannot open 'a' for reading: No such file or directory
             That's misleading, since a subsequent 'ls' shows that 'a'
             is still there.
             One solution would be to open the source file *before* moving
             aside the destination, but that'd involve a big rewrite. */
          if ( ! x->move_mode
               && x->dereference != DEREF_NEVER
               && S_ISLNK (src_sb_link->st_mode)
               && ! S_ISLNK (dst_sb_link->st_mode))
            return false;

          return true;
        }

      /* FIXME: What about case insensitive file systems ?  */
      return ! same_nameat (AT_FDCWD, src_name, dst_dirfd, dst_relname);
    }

#if 0
  /* FIXME: use or remove */

  /* If we're making a backup, we'll detect the problem case in
     copy_reg because SRC_NAME will no longer exist.  Allowing
     the test to be deferred lets cp do some useful things.
     But when creating hardlinks and SRC_NAME is a symlink
     but DST_RELNAME is not we must test anyway.  */
  if (x->hard_link
      || !S_ISLNK (src_sb_link->st_mode)
      || S_ISLNK (dst_sb_link->st_mode))
    return true;

  if (x->dereference != DEREF_NEVER)
    return true;
#endif

  if (x->move_mode || x->unlink_dest_before_opening)
    {
      /* They may refer to the same file if we're in move mode and the
         target is a symlink.  That is ok, since we remove any existing
         destination file before opening it -- via 'rename' if they're on
         the same file system, via unlinkat otherwise.  */
      if (S_ISLNK (dst_sb_link->st_mode))
        return true;

      /* It's not ok if they're distinct hard links to the same file as
         this causes a race condition and we may lose data in this case.  */
      if (same_link
          && 1 < dst_sb_link->st_nlink
          && ! same_nameat (AT_FDCWD, src_name, dst_dirfd, dst_relname))
        return ! x->move_mode;
    }

  /* If neither is a symlink, then it's ok as long as they aren't
     hard links to the same file.  */
  if (!S_ISLNK (src_sb_link->st_mode) && !S_ISLNK (dst_sb_link->st_mode))
    {
      if (!SAME_INODE (*src_sb_link, *dst_sb_link))
        return true;

      /* If they are the same file, it's ok if we're making hard links.  */
      if (x->hard_link)
        {
          *return_now = true;
          return true;
        }
    }

  /* At this point, it is normally an error (data loss) to move a symlink
     onto its referent, but in at least one narrow case, it is not:
     In move mode, when
     1) src is a symlink,
     2) dest has a link count of 2 or more and
     3) dest and the referent of src are not the same directory entry,
     then it's ok, since while we'll lose one of those hard links,
     src will still point to a remaining link.
     Note that technically, condition #3 obviates condition #2, but we
     retain the 1 < st_nlink condition because that means fewer invocations
     of the more expensive #3.

     Given this,
       $ touch f && ln f l && ln -s f s
       $ ls -og f l s
       -rw-------. 2  0 Jan  4 22:46 f
       -rw-------. 2  0 Jan  4 22:46 l
       lrwxrwxrwx. 1  1 Jan  4 22:46 s -> f
     this must fail: mv s f
     this must succeed: mv s l */
  if (x->move_mode
      && S_ISLNK (src_sb->st_mode)
      && 1 < dst_sb_link->st_nlink)
    {
      char *abs_src = canonicalize_file_name (src_name);
      if (abs_src)
        {
          bool result = ! same_nameat (AT_FDCWD, abs_src,
                                       dst_dirfd, dst_relname);
          free (abs_src);
          return result;
        }
    }

  /* It's ok to recreate a destination symlink. */
  if (x->symbolic_link && S_ISLNK (dst_sb_link->st_mode))
    return true;

  if (x->dereference == DEREF_NEVER)
    {
      if ( ! S_ISLNK (src_sb_link->st_mode))
        tmp_src_sb = *src_sb_link;
      else if (stat (src_name, &tmp_src_sb) != 0)
        return true;

      if ( ! S_ISLNK (dst_sb_link->st_mode))
        tmp_dst_sb = *dst_sb_link;
      else if (fstatat (dst_dirfd, dst_relname, &tmp_dst_sb, 0) != 0)
        return true;

      if ( ! SAME_INODE (tmp_src_sb, tmp_dst_sb))
        return true;

      if (x->hard_link)
        {
          /* It's ok to attempt to hardlink the same file,
            and return early if not replacing a symlink.
            Note we need to return early to avoid a later
            unlink() of DST (when SRC is a symlink).  */
          *return_now = ! S_ISLNK (dst_sb_link->st_mode);
          return true;
        }
    }

  return false;
}

/* Return whether DST_DIRFD+DST_RELNAME, with mode MODE,
   is writable in the sense of 'mv'.
   Always consider a symbolic link to be writable.  */
static bool
writable_destination (int dst_dirfd, char const *dst_relname, mode_t mode)
{
  return (S_ISLNK (mode)
          || can_write_any_file ()
          || faccessat (dst_dirfd, dst_relname, W_OK, AT_EACCESS) == 0);
}

static bool
overwrite_ok (struct cp_options const *x, char const *dst_name,
              int dst_dirfd, char const *dst_relname,
              struct stat const *dst_sb)
{
  if (! writable_destination (dst_dirfd, dst_relname, dst_sb->st_mode))
    {
      char perms[12];		/* "-rwxrwxrwx " ls-style modes. */
      strmode (dst_sb->st_mode, perms);
      perms[10] = '\0';
      fprintf (stderr,
               (x->move_mode || x->unlink_dest_before_opening
                || x->unlink_dest_after_failed_open)
               ? _("%s: replace %s, overriding mode %04lo (%s)? ")
               : _("%s: unwritable %s (mode %04lo, %s); try anyway? "),
               program_name, quoteaf (dst_name),
               (unsigned long int) (dst_sb->st_mode & CHMOD_MODE_BITS),
               &perms[1]);
    }
  else
    {
      fprintf (stderr, _("%s: overwrite %s? "),
               program_name, quoteaf (dst_name));
    }

  return yesno ();
}

/* Initialize the hash table implementing a set of F_triple entries
   corresponding to destination files.  */
extern void
dest_info_init (struct cp_options *x)
{
  x->dest_info
    = hash_initialize (DEST_INFO_INITIAL_CAPACITY,
                       nullptr,
                       triple_hash,
                       triple_compare,
                       triple_free);
  if (! x->dest_info)
    xalloc_die ();
}

/* Initialize the hash table implementing a set of F_triple entries
   corresponding to source files listed on the command line.  */
extern void
src_info_init (struct cp_options *x)
{

  /* Note that we use triple_hash_no_name here.
     Contrast with the use of triple_hash above.
     That is necessary because a source file may be specified
     in many different ways.  We want to warn about this
       cp a a d/
     as well as this:
       cp a ./a d/
  */
  x->src_info
    = hash_initialize (DEST_INFO_INITIAL_CAPACITY,
                       nullptr,
                       triple_hash_no_name,
                       triple_compare,
                       triple_free);
  if (! x->src_info)
    xalloc_die ();
}

/* When effecting a move (e.g., for mv(1)), and given the name DST_NAME
   aka DST_DIRFD+DST_RELNAME
   of the destination and a corresponding stat buffer, DST_SB, return
   true if the logical 'move' operation should _not_ proceed.
   Otherwise, return false.
   Depending on options specified in X, this code may issue an
   interactive prompt asking whether it's ok to overwrite DST_NAME.  */
static bool
abandon_move (const struct cp_options *x,
              char const *dst_name,
              int dst_dirfd, char const *dst_relname,
              struct stat const *dst_sb)
{
  affirm (x->move_mode);
  return (x->interactive == I_ALWAYS_NO
          || x->interactive == I_ALWAYS_SKIP
          || ((x->interactive == I_ASK_USER
               || (x->interactive == I_UNSPECIFIED
                   && x->stdin_tty
                   && ! writable_destination (dst_dirfd, dst_relname,
                                              dst_sb->st_mode)))
              && ! overwrite_ok (x, dst_name, dst_dirfd, dst_relname, dst_sb)));
}

/* Print --verbose output on standard output, e.g. 'new' -> 'old'.
   If BACKUP_DST_NAME is non-null, then also indicate that it is
   the name of a backup file.  */
static void
emit_verbose (char const *src, char const *dst, char const *backup_dst_name)
{
  printf ("%s -> %s", quoteaf_n (0, src), quoteaf_n (1, dst));
  if (backup_dst_name)
    printf (_(" (backup: %s)"), quoteaf (backup_dst_name));
  putchar ('\n');
}

/* A wrapper around "setfscreatecon (nullptr)" that exits upon failure.  */
static void
restore_default_fscreatecon_or_die (void)
{
  if (setfscreatecon (nullptr) != 0)
    error (EXIT_FAILURE, errno,
           _("failed to restore the default file creation context"));
}

/* Return a newly-allocated string that is like STR
   except replace its suffix SUFFIX with NEWSUFFIX.  */
static char *
subst_suffix (char const *str, char const *suffix, char const *newsuffix)
{
  idx_t prefixlen = suffix - str;
  idx_t newsuffixsize = strlen (newsuffix) + 1;
  char *r = ximalloc (prefixlen + newsuffixsize);
  memcpy (r + prefixlen, newsuffix, newsuffixsize);
  return memcpy (r, str, prefixlen);
}

/* Create a hard link to SRC_NAME aka SRC_DIRFD+SRC_RELNAME;
   the new link is at DST_NAME aka DST_DIRFD+DST_RELNAME.
   A null SRC_NAME stands for the file whose name is like DST_NAME
   except with DST_RELNAME replaced with SRC_RELNAME.
   Honor the REPLACE, VERBOSE and DEREFERENCE settings.
   Return true upon success.  Otherwise, diagnose the
   failure and return false.  If SRC_NAME is a symbolic link, then it will not
   be followed unless DEREFERENCE is true.
   If the system doesn't support hard links to symbolic links, then DST_NAME
   will be created as a symbolic link to SRC_NAME.  */
static bool
create_hard_link (char const *src_name, int src_dirfd, char const *src_relname,
                  char const *dst_name, int dst_dirfd, char const *dst_relname,
                  bool replace, bool verbose, bool dereference)
{
  int err = force_linkat (src_dirfd, src_relname, dst_dirfd, dst_relname,
                          dereference ? AT_SYMLINK_FOLLOW : 0,
                          replace, -1);
  if (0 < err)
    {

      char *a_src_name = nullptr;
      if (!src_name)
        src_name = a_src_name = subst_suffix (dst_name, dst_relname,
                                              src_relname);
      error (0, err, _("cannot create hard link %s to %s"),
             quoteaf_n (0, dst_name), quoteaf_n (1, src_name));
      free (a_src_name);
      return false;
    }
  if (err < 0 && verbose)
    printf (_("removed %s\n"), quoteaf (dst_name));
  return true;
}

/* Return true if the current file should be (tried to be) dereferenced:
   either for DEREF_ALWAYS or for DEREF_COMMAND_LINE_ARGUMENTS in the case
   where the current file is a COMMAND_LINE_ARG; otherwise return false.  */
ATTRIBUTE_PURE
static inline bool
should_dereference (const struct cp_options *x, bool command_line_arg)
{
  return x->dereference == DEREF_ALWAYS
         || (x->dereference == DEREF_COMMAND_LINE_ARGUMENTS
             && command_line_arg);
}

/* Return true if the source file with basename SRCBASE and status SRC_ST
   is likely to be the simple backup file for DST_DIRFD+DST_RELNAME.  */
static bool
source_is_dst_backup (char const *srcbase, struct stat const *src_st,
                      int dst_dirfd, char const *dst_relname)
{
  size_t srcbaselen = strlen (srcbase);
  char const *dstbase = last_component (dst_relname);
  size_t dstbaselen = strlen (dstbase);
  size_t suffixlen = strlen (simple_backup_suffix);
  if (! (srcbaselen == dstbaselen + suffixlen
         && memcmp (srcbase, dstbase, dstbaselen) == 0
         && STREQ (srcbase + dstbaselen, simple_backup_suffix)))
    return false;
  char *dst_back = subst_suffix (dst_relname,
                                 dst_relname + strlen (dst_relname),
                                 simple_backup_suffix);
  struct stat dst_back_sb;
  int dst_back_status = fstatat (dst_dirfd, dst_back, &dst_back_sb, 0);
  free (dst_back);
  return dst_back_status == 0 && SAME_INODE (*src_st, dst_back_sb);
}

/* Copy the file SRC_NAME to the file DST_NAME aka DST_DIRFD+DST_RELNAME.
   If NONEXISTENT_DST is positive, DST_NAME does not exist even as a
   dangling symlink; if negative, it does not exist except possibly
   as a dangling symlink; if zero, its existence status is unknown.
   A non-null PARENT describes the parent directory.
   ANCESTORS points to a linked, null terminated list of
   devices and inodes of parent directories of SRC_NAME.
   X summarizes the command-line options.
   COMMAND_LINE_ARG means SRC_NAME was specified on the command line.
   FIRST_DIR_CREATED_PER_COMMAND_LINE_ARG is both input and output.
   Set *COPY_INTO_SELF if SRC_NAME is a parent of (or the
   same as) DST_NAME; otherwise, clear it.
   If X->move_mode, set *RENAME_SUCCEEDED according to whether
   the source was simply renamed to the destination.
   Return true if successful.  */
static bool
copy_internal (char const *src_name, char const *dst_name,
               int dst_dirfd, char const *dst_relname,
               int nonexistent_dst,
               struct stat const *parent,
               struct dir_list *ancestors,
               const struct cp_options *x,
               bool command_line_arg,
               bool *first_dir_created_per_command_line_arg,
               bool *copy_into_self,
               bool *rename_succeeded)
{
  struct stat src_sb;
  struct stat dst_sb;
  mode_t src_mode IF_LINT ( = 0);
  mode_t dst_mode IF_LINT ( = 0);
  mode_t dst_mode_bits;
  mode_t omitted_permissions;
  bool restore_dst_mode = false;
  char *earlier_file = nullptr;
  char *dst_backup = nullptr;
  char const *drelname = *dst_relname ? dst_relname : ".";
  bool delayed_ok;
  bool copied_as_regular = false;
  bool dest_is_symlink = false;
  bool have_dst_lstat = false;

  /* Whether the destination is (or was) known to be new, updated as
     more info comes in.  This may become true if the destination is a
     dangling symlink, in contexts where dangling symlinks should be
     treated the same as nonexistent files.  */
  bool new_dst = 0 < nonexistent_dst;

  *copy_into_self = false;

  int rename_errno = x->rename_errno;
  if (x->move_mode)
    {
      if (rename_errno < 0)
        rename_errno = (renameatu (AT_FDCWD, src_name, dst_dirfd, drelname,
                                   RENAME_NOREPLACE)
                        ? errno : 0);
      nonexistent_dst = *rename_succeeded = new_dst = rename_errno == 0;
    }

  if (rename_errno == 0
      ? !x->last_file
      : rename_errno != EEXIST
        || (x->interactive != I_ALWAYS_NO && x->interactive != I_ALWAYS_SKIP))
    {
      char const *name = rename_errno == 0 ? dst_name : src_name;
      int dirfd = rename_errno == 0 ? dst_dirfd : AT_FDCWD;
      char const *relname = rename_errno == 0 ? drelname : src_name;
      int fstatat_flags
        = x->dereference == DEREF_NEVER ? AT_SYMLINK_NOFOLLOW : 0;
      if (follow_fstatat (dirfd, relname, &src_sb, fstatat_flags) != 0)
        {
          error (0, errno, _("cannot stat %s"), quoteaf (name));
          return false;
        }

      src_mode = src_sb.st_mode;

      if (S_ISDIR (src_mode) && !x->recursive)
        {
          error (0, 0, ! x->install_mode /* cp */
                 ? _("-r not specified; omitting directory %s")
                 : _("omitting directory %s"),
                 quoteaf (src_name));
          return false;
        }
    }
  else
    {
#if defined lint && (defined __clang__ || defined __COVERITY__)
      affirm (x->move_mode);
      memset (&src_sb, 0, sizeof src_sb);
#endif
    }

  /* Detect the case in which the same source file appears more than
     once on the command line and no backup option has been selected.
     If so, simply warn and don't copy it the second time.
     This check is enabled only if x->src_info is non-null.  */
  if (command_line_arg && x->src_info)
    {
      if ( ! S_ISDIR (src_mode)
           && x->backup_type == no_backups
           && seen_file (x->src_info, src_name, &src_sb))
        {
          error (0, 0, _("warning: source file %s specified more than once"),
                 quoteaf (src_name));
          return true;
        }

      record_file (x->src_info, src_name, &src_sb);
    }

  bool dereference = should_dereference (x, command_line_arg);

  if (nonexistent_dst <= 0)
    {
      if (! (rename_errno == EEXIST
             && (x->interactive == I_ALWAYS_NO
                 || x->interactive == I_ALWAYS_SKIP)))
        {
          /* Regular files can be created by writing through symbolic
             links, but other files cannot.  So use stat on the
             destination when copying a regular file, and lstat otherwise.
             However, if we intend to unlink or remove the destination
             first, use lstat, since a copy won't actually be made to the
             destination in that case.  */
          bool use_lstat
            = ((! S_ISREG (src_mode)
                && (! x->copy_as_regular
                    || S_ISDIR (src_mode) || S_ISLNK (src_mode)))
               || x->move_mode || x->symbolic_link || x->hard_link
               || x->backup_type != no_backups
               || x->unlink_dest_before_opening);
          int fstatat_flags = use_lstat ? AT_SYMLINK_NOFOLLOW : 0;
          if (!use_lstat && nonexistent_dst < 0)
            new_dst = true;
          else if (follow_fstatat (dst_dirfd, drelname, &dst_sb, fstatat_flags)
                   == 0)
            {
              have_dst_lstat = use_lstat;
              rename_errno = EEXIST;
            }
          else
            {
              if (errno == ELOOP && x->unlink_dest_after_failed_open)
                /* leave new_dst=false so we unlink later.  */;
              else if (errno != ENOENT)
                {
                  error (0, errno, _("cannot stat %s"), quoteaf (dst_name));
                  return false;
                }
              else
                new_dst = true;
            }
        }

      if (rename_errno == EEXIST)
        {
          bool return_now = false;
          bool return_val = true;
          bool skipped = false;

          if ((x->interactive != I_ALWAYS_NO && x->interactive != I_ALWAYS_SKIP)
              && ! same_file_ok (src_name, &src_sb, dst_dirfd, drelname,
                                 &dst_sb, x, &return_now))
            {
              error (0, 0, _("%s and %s are the same file"),
                     quoteaf_n (0, src_name), quoteaf_n (1, dst_name));
              return false;
            }

          if (x->update && !S_ISDIR (src_mode))
            {
              /* When preserving timestamps (but not moving within a file
                 system), don't worry if the destination timestamp is
                 less than the source merely because of timestamp
                 truncation.  */
              int options = ((x->preserve_timestamps
                              && ! (x->move_mode
                                    && dst_sb.st_dev == src_sb.st_dev))
                             ? UTIMECMP_TRUNCATE_SOURCE
                             : 0);

              if (0 <= utimecmpat (dst_dirfd, dst_relname, &dst_sb,
                                   &src_sb, options))
                {
                  /* We're using --update and the destination is not older
                     than the source, so do not copy or move.  Pretend the
                     rename succeeded, so the caller (if it's mv) doesn't
                     end up removing the source file.  */
                  if (rename_succeeded)
                    *rename_succeeded = true;

                  /* However, we still must record that we've processed
                     this src/dest pair, in case this source file is
                     hard-linked to another one.  In that case, we'll use
                     the mapping information to link the corresponding
                     destination names.  */
                  earlier_file = remember_copied (dst_relname, src_sb.st_ino,
                                                  src_sb.st_dev);
                  if (earlier_file)
                    {
                      /* Note we currently replace DST_NAME unconditionally,
                         even if it was a newer separate file.  */
                      if (! create_hard_link (nullptr, dst_dirfd, earlier_file,
                                              dst_name, dst_dirfd, dst_relname,
                                              true,
                                              x->verbose, dereference))
                        {
                          goto un_backup;
                        }
                    }

                  skipped = true;
                  goto skip;
                }
            }

          /* When there is an existing destination file, we may end up
             returning early, and hence not copying/moving the file.
             This may be due to an interactive 'negative' reply to the
             prompt about the existing file.  It may also be due to the
             use of the --no-clobber option.

             cp and mv treat -i and -f differently.  */
          if (x->move_mode)
            {
              if (abandon_move (x, dst_name, dst_dirfd, drelname, &dst_sb))
                {
                  /* Pretend the rename succeeded, so the caller (mv)
                     doesn't end up removing the source file.  */
                  if (rename_succeeded)
                    *rename_succeeded = true;

                  skipped = true;
                  return_val = x->interactive == I_ALWAYS_SKIP;
                }
            }
          else
            {
              if (! S_ISDIR (src_mode)
                  && (x->interactive == I_ALWAYS_NO
                      || x->interactive == I_ALWAYS_SKIP
                      || (x->interactive == I_ASK_USER
                          && ! overwrite_ok (x, dst_name, dst_dirfd,
                                             dst_relname, &dst_sb))))
                {
                  skipped = true;
                  return_val = x->interactive == I_ALWAYS_SKIP;
                }
            }

skip:
          if (skipped)
            {
              if (x->interactive == I_ALWAYS_NO)
                error (0, 0, _("not replacing %s"), quoteaf (dst_name));
              else if (x->debug)
                printf (_("skipped %s\n"), quoteaf (dst_name));

              return_now = true;
            }

          if (return_now)
            return return_val;

          if (!S_ISDIR (dst_sb.st_mode))
            {
              if (S_ISDIR (src_mode))
                {
                  if (x->move_mode && x->backup_type != no_backups)
                    {
                      /* Moving a directory onto an existing
                         non-directory is ok only with --backup.  */
                    }
                  else
                    {
                      error (0, 0,
                       _("cannot overwrite non-directory %s with directory %s"),
                             quoteaf_n (0, dst_name), quoteaf_n (1, src_name));
                      return false;
                    }
                }

              /* Don't let the user destroy their data, even if they try hard:
                 This mv command must fail (likewise for cp):
                   rm -rf a b c; mkdir a b c; touch a/f b/f; mv a/f b/f c
                 Otherwise, the contents of b/f would be lost.
                 In the case of 'cp', b/f would be lost if the user simulated
                 a move using cp and rm.
                 Note that it works fine if you use --backup=numbered.  */
              if (command_line_arg
                  && x->backup_type != numbered_backups
                  && seen_file (x->dest_info, dst_relname, &dst_sb))
                {
                  error (0, 0,
                         _("will not overwrite just-created %s with %s"),
                         quoteaf_n (0, dst_name), quoteaf_n (1, src_name));
                  return false;
                }
            }

          if (!S_ISDIR (src_mode))
            {
              if (S_ISDIR (dst_sb.st_mode))
                {
                  if (x->move_mode && x->backup_type != no_backups)
                    {
                      /* Moving a non-directory onto an existing
                         directory is ok only with --backup.  */
                    }
                  else
                    {
                      error (0, 0,
                         _("cannot overwrite directory %s with non-directory"),
                             quoteaf (dst_name));
                      return false;
                    }
                }
            }

          if (x->move_mode)
            {
              /* Don't allow user to move a directory onto a non-directory.  */
              if (S_ISDIR (src_sb.st_mode) && !S_ISDIR (dst_sb.st_mode)
                  && x->backup_type == no_backups)
                {
                  error (0, 0,
                       _("cannot move directory onto non-directory: %s -> %s"),
                         quotef_n (0, src_name), quotef_n (0, dst_name));
                  return false;
                }
            }

          char const *srcbase;
          if (x->backup_type != no_backups
              /* Don't try to back up a destination if the last
                 component of src_name is "." or "..".  */
              && ! dot_or_dotdot (srcbase = last_component (src_name))
              /* Create a backup of each destination directory in move mode,
                 but not in copy mode.  FIXME: it might make sense to add an
                 option to suppress backup creation also for move mode.
                 That would let one use mv to merge new content into an
                 existing hierarchy.  */
              && (x->move_mode || ! S_ISDIR (dst_sb.st_mode)))
            {
              /* Fail if creating the backup file would likely destroy
                 the source file.  Otherwise, the commands:
                 cd /tmp; rm -f a a~; : > a; echo A > a~; cp --b=simple a~ a
                 would leave two zero-length files: a and a~.  */
              if (x->backup_type != numbered_backups
                  && source_is_dst_backup (srcbase, &src_sb,
                                           dst_dirfd, dst_relname))
                {
                  char const *fmt;
                  fmt = (x->move_mode
                 ? _("backing up %s might destroy source;  %s not moved")
                 : _("backing up %s might destroy source;  %s not copied"));
                  error (0, 0, fmt,
                         quoteaf_n (0, dst_name),
                         quoteaf_n (1, src_name));
                  return false;
                }

              char *tmp_backup = backup_file_rename (dst_dirfd, dst_relname,
                                                     x->backup_type);

              /* FIXME: use fts:
                 Using alloca for a file name that may be arbitrarily
                 long is not recommended.  In fact, even forming such a name
                 should be discouraged.  Eventually, this code will be rewritten
                 to use fts, so using alloca here will be less of a problem.  */
              if (tmp_backup)
                {
                  idx_t dirlen = dst_relname - dst_name;
                  idx_t backupsize = strlen (tmp_backup) + 1;
                  dst_backup = alloca (dirlen + backupsize);
                  memcpy (mempcpy (dst_backup, dst_name, dirlen),
                          tmp_backup, backupsize);
                  free (tmp_backup);
                }
              else if (errno != ENOENT)
                {
                  error (0, errno, _("cannot backup %s"), quoteaf (dst_name));
                  return false;
                }
              new_dst = true;
            }
          else if (! S_ISDIR (dst_sb.st_mode)
                   /* Never unlink dst_name when in move mode.  */
                   && ! x->move_mode
                   && (x->unlink_dest_before_opening
                       || (x->data_copy_required
                           && ((x->preserve_links && 1 < dst_sb.st_nlink)
                               || (x->dereference == DEREF_NEVER
                                   && ! S_ISREG (src_sb.st_mode))))
                      ))
            {
              if (unlinkat (dst_dirfd, dst_relname, 0) != 0 && errno != ENOENT)
                {
                  error (0, errno, _("cannot remove %s"), quoteaf (dst_name));
                  return false;
                }
              new_dst = true;
              if (x->verbose)
                printf (_("removed %s\n"), quoteaf (dst_name));
            }
        }
    }

  /* Ensure we don't try to copy through a symlink that was
     created by a prior call to this function.  */
  if (command_line_arg
      && x->dest_info
      && ! x->move_mode
      && x->backup_type == no_backups)
    {
      bool lstat_ok = true;
      struct stat tmp_buf;
      struct stat *dst_lstat_sb;

      /* If we did not follow symlinks above, good: use that data.
         Otherwise, use AT_SYMLINK_NOFOLLOW, in case dst_name is a symlink.  */
      if (have_dst_lstat)
        dst_lstat_sb = &dst_sb;
      else if (fstatat (dst_dirfd, drelname, &tmp_buf, AT_SYMLINK_NOFOLLOW)
               == 0)
        dst_lstat_sb = &tmp_buf;
      else
        lstat_ok = false;

      /* Never copy through a symlink we've just created.  */
      if (lstat_ok
          && S_ISLNK (dst_lstat_sb->st_mode)
          && seen_file (x->dest_info, dst_relname, dst_lstat_sb))
        {
          error (0, 0,
                 _("will not copy %s through just-created symlink %s"),
                 quoteaf_n (0, src_name), quoteaf_n (1, dst_name));
          return false;
        }
    }

  /* If the source is a directory, we don't always create the destination
     directory.  So --verbose should not announce anything until we're
     sure we'll create a directory.  Also don't announce yet when moving
     so we can distinguish renames versus copies.  */
  if (x->verbose && !x->move_mode && !S_ISDIR (src_mode))
    emit_verbose (src_name, dst_name, dst_backup);

  /* Associate the destination file name with the source device and inode
     so that if we encounter a matching dev/ino pair in the source tree
     we can arrange to create a hard link between the corresponding names
     in the destination tree.

     When using the --link (-l) option, there is no need to take special
     measures, because (barring race conditions) files that are hard-linked
     in the source tree will also be hard-linked in the destination tree.

     Sometimes, when preserving links, we have to record dev/ino even
     though st_nlink == 1:
     - when in move_mode, since we may be moving a group of N hard-linked
        files (via two or more command line arguments) to a different
        partition; the links may be distributed among the command line
        arguments (possibly hierarchies) so that the link count of
        the final, once-linked source file is reduced to 1 when it is
        considered below.  But in this case (for mv) we don't need to
        incur the expense of recording the dev/ino => name mapping; all we
        really need is a lookup, to see if the dev/ino pair has already
        been copied.
     - when using -H and processing a command line argument;
        that command line argument could be a symlink pointing to another
        command line argument.  With 'cp -H --preserve=link', we hard-link
        those two destination files.
     - likewise for -L except that it applies to all files, not just
        command line arguments.

     Also, with --recursive, record dev/ino of each command-line directory.
     We'll use that info to detect this problem: cp -R dir dir.  */

  if (rename_errno == 0)
    earlier_file = nullptr;
  else if (x->recursive && S_ISDIR (src_mode))
    {
      if (command_line_arg)
        earlier_file = remember_copied (dst_relname,
                                        src_sb.st_ino, src_sb.st_dev);
      else
        earlier_file = src_to_dest_lookup (src_sb.st_ino, src_sb.st_dev);
    }
  else if (x->move_mode && src_sb.st_nlink == 1)
    {
      earlier_file = src_to_dest_lookup (src_sb.st_ino, src_sb.st_dev);
    }
  else if (x->preserve_links
           && !x->hard_link
           && (1 < src_sb.st_nlink
               || (command_line_arg
                   && x->dereference == DEREF_COMMAND_LINE_ARGUMENTS)
               || x->dereference == DEREF_ALWAYS))
    {
      earlier_file = remember_copied (dst_relname,
                                      src_sb.st_ino, src_sb.st_dev);
    }

  /* Did we copy this inode somewhere else (in this command line argument)
     and therefore this is a second hard link to the inode?  */

  if (earlier_file)
    {
      /* Avoid damaging the destination file system by refusing to preserve
         hard-linked directories (which are found at least in Netapp snapshot
         directories).  */
      if (S_ISDIR (src_mode))
        {
          /* If src_name and earlier_file refer to the same directory entry,
             then warn about copying a directory into itself.  */
          if (same_nameat (AT_FDCWD, src_name, dst_dirfd, earlier_file))
            {
              error (0, 0, _("cannot copy a directory, %s, into itself, %s"),
                     quoteaf_n (0, top_level_src_name),
                     quoteaf_n (1, top_level_dst_name));
              *copy_into_self = true;
              goto un_backup;
            }
          else if (same_nameat (dst_dirfd, dst_relname,
                                dst_dirfd, earlier_file))
            {
              error (0, 0, _("warning: source directory %s "
                             "specified more than once"),
                     quoteaf (top_level_src_name));
              /* In move mode, if a previous rename succeeded, then
                 we won't be in this path as the source is missing.  If the
                 rename previously failed, then that has been handled, so
                 pretend this attempt succeeded so the source isn't removed.  */
              if (x->move_mode && rename_succeeded)
                *rename_succeeded = true;
              /* We only do backups in move mode, and for non directories.
                 So just ignore this repeated entry.  */
              return true;
            }
          else if (x->dereference == DEREF_ALWAYS
                   || (command_line_arg
                       && x->dereference == DEREF_COMMAND_LINE_ARGUMENTS))
            {
              /* This happens when e.g., encountering a directory for the
                 second or subsequent time via symlinks when cp is invoked
                 with -R and -L.  E.g.,
                 rm -rf a b c d; mkdir a b c d; ln -s ../c a; ln -s ../c b;
                 cp -RL a b d
              */
            }
          else
            {
              char *earlier = subst_suffix (dst_name, dst_relname,
                                            earlier_file);
              error (0, 0, _("will not create hard link %s to directory %s"),
                     quoteaf_n (0, dst_name), quoteaf_n (1, earlier));
              free (earlier);
              goto un_backup;
            }
        }
      else
        {
          if (! create_hard_link (nullptr, dst_dirfd, earlier_file,
                                  dst_name, dst_dirfd, dst_relname,
                                  true, x->verbose, dereference))
            goto un_backup;

          return true;
        }
    }

  if (x->move_mode)
    {
      if (rename_errno == EEXIST)
        rename_errno = (renameat (AT_FDCWD, src_name, dst_dirfd, drelname) == 0
                        ? 0 : errno);

      if (rename_errno == 0)
        {
          if (x->verbose)
            {
              printf (_("renamed "));
              emit_verbose (src_name, dst_name, dst_backup);
            }

          if (x->set_security_context)
            {
              /* -Z failures are only warnings currently.  */
              (void) set_file_security_ctx (dst_name, true, x);
            }

          if (rename_succeeded)
            *rename_succeeded = true;

          if (command_line_arg && !x->last_file)
            {
              /* Record destination dev/ino/name, so that if we are asked
                 to overwrite that file again, we can detect it and fail.  */
              /* It's fine to use the _source_ stat buffer (src_sb) to get the
                 _destination_ dev/ino, since the rename above can't have
                 changed those, and 'mv' always uses lstat.
                 We could limit it further by operating
                 only on non-directories.  */
              record_file (x->dest_info, dst_relname, &src_sb);
            }

          return true;
        }

      /* FIXME: someday, consider what to do when moving a directory into
         itself but when source and destination are on different devices.  */

      /* This happens when attempting to rename a directory to a
         subdirectory of itself.  */
      if (rename_errno == EINVAL)
        {
          /* FIXME: this is a little fragile in that it relies on rename(2)
             failing with a specific errno value.  Expect problems on
             non-POSIX systems.  */
          error (0, 0, _("cannot move %s to a subdirectory of itself, %s"),
                 quoteaf_n (0, top_level_src_name),
                 quoteaf_n (1, top_level_dst_name));

          /* Note that there is no need to call forget_created here,
             (compare with the other calls in this file) since the
             destination directory didn't exist before.  */

          *copy_into_self = true;
          /* FIXME-cleanup: Don't return true here; adjust mv.c accordingly.
             The only caller that uses this code (mv.c) ends up setting its
             exit status to nonzero when copy_into_self is nonzero.  */
          return true;
        }

      /* WARNING: there probably exist systems for which an inter-device
         rename fails with a value of errno not handled here.
         If/as those are reported, add them to the condition below.
         If this happens to you, please do the following and send the output
         to the bug-reporting address (e.g., in the output of cp --help):
           touch k; perl -e 'rename "k","/tmp/k" or print "$!(",$!+0,")\n"'
         where your current directory is on one partition and /tmp is the other.
         Also, please try to find the E* errno macro name corresponding to
         the diagnostic and parenthesized integer, and include that in your
         e-mail.  One way to do that is to run a command like this
           find /usr/include/. -type f \
             | xargs grep 'define.*\<E[A-Z]*\>.*\<18\>' /dev/null
         where you'd replace '18' with the integer in parentheses that
         was output from the perl one-liner above.
         If necessary, of course, change '/tmp' to some other directory.  */
      if (rename_errno != EXDEV || x->no_copy)
        {
          /* There are many ways this can happen due to a race condition.
             When something happens between the initial follow_fstatat and the
             subsequent rename, we can get many different types of errors.
             For example, if the destination is initially a non-directory
             or non-existent, but it is created as a directory, the rename
             fails.  If two 'mv' commands try to rename the same file at
             about the same time, one will succeed and the other will fail.
             If the permissions on the directory containing the source or
             destination file are made too restrictive, the rename will
             fail.  Etc.  */
          char const *quoted_dst_name = quoteaf_n (1, dst_name);
          switch (rename_errno)
            {
            case EDQUOT: case EEXIST: case EISDIR: case EMLINK:
            case ENOSPC: case ENOTEMPTY: case ETXTBSY:
              /* The destination must be the problem.  Don't mention
                 the source as that is more likely to confuse the user
                 than be helpful.  */
              error (0, rename_errno, _("cannot overwrite %s"),
                     quoted_dst_name);
              break;

            default:
              error (0, rename_errno, _("cannot move %s to %s"),
                     quoteaf_n (0, src_name), quoted_dst_name);
              break;
            }
          forget_created (src_sb.st_ino, src_sb.st_dev);
          return false;
        }

      /* The rename attempt has failed.  Remove any existing destination
         file so that a cross-device 'mv' acts as if it were really using
         the rename syscall.  Note both src and dst must both be directories
         or not, and this is enforced above.  Therefore we check the src_mode
         and operate on dst_name here as a tighter constraint and also because
         src_mode is readily available here.  */
      if ((unlinkat (dst_dirfd, drelname,
                     S_ISDIR (src_mode) ? AT_REMOVEDIR : 0)
           != 0)
          && errno != ENOENT)
        {
          error (0, errno,
             _("inter-device move failed: %s to %s; unable to remove target"),
                 quoteaf_n (0, src_name), quoteaf_n (1, dst_name));
          forget_created (src_sb.st_ino, src_sb.st_dev);
          return false;
        }

      if (x->verbose && !S_ISDIR (src_mode))
        {
          printf (_("copied "));
          emit_verbose (src_name, dst_name, dst_backup);
        }
      new_dst = true;
    }

  /* If the ownership might change, or if it is a directory (whose
     special mode bits may change after the directory is created),
     omit some permissions at first, so unauthorized users cannot nip
     in before the file is ready.  */
  dst_mode_bits = (x->set_mode ? x->mode : src_mode) & CHMOD_MODE_BITS;
  omitted_permissions =
    (dst_mode_bits
     & (x->preserve_ownership ? S_IRWXG | S_IRWXO
        : S_ISDIR (src_mode) ? S_IWGRP | S_IWOTH
        : 0));

  delayed_ok = true;

  /* If required, set the default security context for new files.
     Also for existing files this is used as a reference
     when copying the context with --preserve=context.
     FIXME: Do we need to consider dst_mode_bits here?  */
  if (! set_process_security_ctx (src_name, dst_name, src_mode, new_dst, x))
    return false;

  if (S_ISDIR (src_mode))
    {
      struct dir_list *dir;

      /* If this directory has been copied before during the
         recursion, there is a symbolic link to an ancestor
         directory of the symbolic link.  It is impossible to
         continue to copy this, unless we've got an infinite file system.  */

      if (is_ancestor (&src_sb, ancestors))
        {
          error (0, 0, _("cannot copy cyclic symbolic link %s"),
                 quoteaf (src_name));
          goto un_backup;
        }

      /* Insert the current directory in the list of parents.  */

      dir = alloca (sizeof *dir);
      dir->parent = ancestors;
      dir->ino = src_sb.st_ino;
      dir->dev = src_sb.st_dev;

      if (new_dst || !S_ISDIR (dst_sb.st_mode))
        {
          /* POSIX says mkdir's behavior is implementation-defined when
             (src_mode & ~S_IRWXUGO) != 0.  However, common practice is
             to ask mkdir to copy all the CHMOD_MODE_BITS, letting mkdir
             decide what to do with S_ISUID | S_ISGID | S_ISVTX.  */
          mode_t mode = dst_mode_bits & ~omitted_permissions;
          if (mkdirat (dst_dirfd, drelname, mode) != 0)
            {
              error (0, errno, _("cannot create directory %s"),
                     quoteaf (dst_name));
              goto un_backup;
            }

          /* We need search and write permissions to the new directory
             for writing the directory's contents. Check if these
             permissions are there.  */

          if (fstatat (dst_dirfd, drelname, &dst_sb, AT_SYMLINK_NOFOLLOW) != 0)
            {
              error (0, errno, _("cannot stat %s"), quoteaf (dst_name));
              goto un_backup;
            }
          else if ((dst_sb.st_mode & S_IRWXU) != S_IRWXU)
            {
              /* Make the new directory searchable and writable.  */

              dst_mode = dst_sb.st_mode;
              restore_dst_mode = true;

              if (lchmodat (dst_dirfd, drelname, dst_mode | S_IRWXU) != 0)
                {
                  error (0, errno, _("setting permissions for %s"),
                         quoteaf (dst_name));
                  goto un_backup;
                }
            }

          /* Record the created directory's inode and device numbers into
             the search structure, so that we can avoid copying it again.
             Do this only for the first directory that is created for each
             source command line argument.  */
          if (!*first_dir_created_per_command_line_arg)
            {
              remember_copied (dst_relname, dst_sb.st_ino, dst_sb.st_dev);
              *first_dir_created_per_command_line_arg = true;
            }

          if (x->verbose)
            {
              if (x->move_mode)
                printf (_("created directory %s\n"), quoteaf (dst_name));
              else
                emit_verbose (src_name, dst_name, nullptr);
            }
        }
      else
        {
          omitted_permissions = 0;

          /* For directories, the process global context could be reset for
             descendents, so use it to set the context for existing dirs here.
             This will also give earlier indication of failure to set ctx.  */
          if (x->set_security_context || x->preserve_security_context)
            if (! set_file_security_ctx (dst_name, false, x))
              {
                if (x->require_preserve_context)
                  goto un_backup;
              }
        }

      /* Decide whether to copy the contents of the directory.  */
      if (x->one_file_system && parent && parent->st_dev != src_sb.st_dev)
        {
          /* Here, we are crossing a file system boundary and cp's -x option
             is in effect: so don't copy the contents of this directory. */
        }
      else
        {
          /* Copy the contents of the directory.  Don't just return if
             this fails -- otherwise, the failure to read a single file
             in a source directory would cause the containing destination
             directory not to have owner/perms set properly.  */
          delayed_ok = copy_dir (src_name, dst_name, dst_dirfd, dst_relname,
                                 new_dst, &src_sb, dir, x,
                                 first_dir_created_per_command_line_arg,
                                 copy_into_self);
        }
    }
  else if (x->symbolic_link)
    {
      dest_is_symlink = true;
      if (*src_name != '/')
        {
          /* Check that DST_NAME denotes a file in the current directory.  */
          struct stat dot_sb;
          struct stat dst_parent_sb;
          char *dst_parent;
          bool in_current_dir;

          dst_parent = dir_name (dst_relname);

          in_current_dir = ((dst_dirfd == AT_FDCWD && STREQ (".", dst_parent))
                            /* If either stat call fails, it's ok not to report
                               the failure and say dst_name is in the current
                               directory.  Other things will fail later.  */
                            || stat (".", &dot_sb) != 0
                            || (fstatat (dst_dirfd, dst_parent, &dst_parent_sb,
                                         0) != 0)
                            || SAME_INODE (dot_sb, dst_parent_sb));
          free (dst_parent);

          if (! in_current_dir)
            {
              error (0, 0,
           _("%s: can make relative symbolic links only in current directory"),
                     quotef (dst_name));
              goto un_backup;
            }
        }

      int err = force_symlinkat (src_name, dst_dirfd, dst_relname,
                                 x->unlink_dest_after_failed_open, -1);
      if (0 < err)
        {
          error (0, err, _("cannot create symbolic link %s to %s"),
                 quoteaf_n (0, dst_name), quoteaf_n (1, src_name));
          goto un_backup;
        }
    }

  /* POSIX 2008 states that it is implementation-defined whether
     link() on a symlink creates a hard-link to the symlink, or only
     to the referent (effectively dereferencing the symlink) (POSIX
     2001 required the latter behavior, although many systems provided
     the former).  Yet cp, invoked with '--link --no-dereference',
     should not follow the link.  We can approximate the desired
     behavior by skipping this hard-link creating block and instead
     copying the symlink, via the 'S_ISLNK'- copying code below.

     Note gnulib's linkat module, guarantees that the symlink is not
     dereferenced.  However its emulation currently doesn't maintain
     timestamps or ownership so we only call it when we know the
     emulation will not be needed.  */
  else if (x->hard_link
           && !(! CAN_HARDLINK_SYMLINKS && S_ISLNK (src_mode)
                && x->dereference == DEREF_NEVER))
    {
      bool replace = (x->unlink_dest_after_failed_open
                      || x->interactive == I_ASK_USER);
      if (! create_hard_link (src_name, AT_FDCWD, src_name,
                              dst_name, dst_dirfd, dst_relname,
                              replace, false, dereference))
        goto un_backup;
    }
  else if (S_ISREG (src_mode)
           || (x->copy_as_regular && !S_ISLNK (src_mode)))
    {
      copied_as_regular = true;
      /* POSIX says the permission bits of the source file must be
         used as the 3rd argument in the open call.  Historical
         practice passed all the source mode bits to 'open', but the extra
         bits were ignored, so it should be the same either way.

         This call uses DST_MODE_BITS, not SRC_MODE.  These are
         normally the same, and the exception (where x->set_mode) is
         used only by 'install', which POSIX does not specify and
         where DST_MODE_BITS is what's wanted.  */
      if (! copy_reg (src_name, dst_name, dst_dirfd, dst_relname,
                      x, dst_mode_bits & S_IRWXUGO,
                      omitted_permissions, &new_dst, &src_sb))
        goto un_backup;
    }
  else if (S_ISFIFO (src_mode))
    {
      /* Use mknodat, rather than mkfifoat, because the former preserves
         the special mode bits of a fifo on Solaris 10, while mkfifoat
         does not.  But fall back on mkfifoat, because on some BSD systems,
         mknodat always fails when asked to create a FIFO.  */
      mode_t mode = src_mode & ~omitted_permissions;
      if (mknodat (dst_dirfd, dst_relname, mode, 0) != 0)
        if (mkfifoat (dst_dirfd, dst_relname, mode & ~S_IFIFO) != 0)
          {
            error (0, errno, _("cannot create fifo %s"), quoteaf (dst_name));
            goto un_backup;
          }
    }
  else if (S_ISBLK (src_mode) || S_ISCHR (src_mode) || S_ISSOCK (src_mode))
    {
      mode_t mode = src_mode & ~omitted_permissions;
      if (mknodat (dst_dirfd, dst_relname, mode, src_sb.st_rdev) != 0)
        {
          error (0, errno, _("cannot create special file %s"),
                 quoteaf (dst_name));
          goto un_backup;
        }
    }
  else if (S_ISLNK (src_mode))
    {
      char *src_link_val = areadlink_with_size (src_name, src_sb.st_size);
      dest_is_symlink = true;
      if (src_link_val == nullptr)
        {
          error (0, errno, _("cannot read symbolic link %s"),
                 quoteaf (src_name));
          goto un_backup;
        }

      int symlink_err = force_symlinkat (src_link_val, dst_dirfd, dst_relname,
                                         x->unlink_dest_after_failed_open, -1);
      if (0 < symlink_err && x->update && !new_dst && S_ISLNK (dst_sb.st_mode)
          && dst_sb.st_size == strlen (src_link_val))
        {
          /* See if the destination is already the desired symlink.
             FIXME: This behavior isn't documented, and seems wrong
             in some cases, e.g., if the destination symlink has the
             wrong ownership, permissions, or timestamps.  */
          char *dest_link_val =
            areadlinkat_with_size (dst_dirfd, dst_relname, dst_sb.st_size);
          if (dest_link_val)
            {
              if (STREQ (dest_link_val, src_link_val))
                symlink_err = 0;
              free (dest_link_val);
            }
        }
      free (src_link_val);
      if (0 < symlink_err)
        {
          error (0, symlink_err, _("cannot create symbolic link %s"),
                 quoteaf (dst_name));
          goto un_backup;
        }

      if (x->preserve_security_context)
        restore_default_fscreatecon_or_die ();

      if (x->preserve_ownership)
        {
          /* Preserve the owner and group of the just-'copied'
             symbolic link, if possible.  */
          if (HAVE_LCHOWN
              && (lchownat (dst_dirfd, dst_relname,
                            src_sb.st_uid, src_sb.st_gid)
                  != 0)
              && ! chown_failure_ok (x))
            {
              error (0, errno, _("failed to preserve ownership for %s"),
                     dst_name);
              if (x->require_preserve)
                goto un_backup;
            }
          else
            {
              /* Can't preserve ownership of symlinks.
                 FIXME: maybe give a warning or even error for symlinks
                 in directories with the sticky bit set -- there, not
                 preserving owner/group is a potential security problem.  */
            }
        }
    }
  else
    {
      error (0, 0, _("%s has unknown file type"), quoteaf (src_name));
      goto un_backup;
    }

  /* With -Z or --preserve=context, set the context for existing files.
     Note this is done already for copy_reg() for reasons described therein.  */
  if (!new_dst && !x->copy_as_regular && !S_ISDIR (src_mode)
      && (x->set_security_context || x->preserve_security_context))
    {
      if (! set_file_security_ctx (dst_name, false, x))
        {
           if (x->require_preserve_context)
             goto un_backup;
        }
    }

  if (command_line_arg && x->dest_info)
    {
      /* Now that the destination file is very likely to exist,
         add its info to the set.  */
      struct stat sb;
      if (fstatat (dst_dirfd, drelname, &sb, AT_SYMLINK_NOFOLLOW) == 0)
        record_file (x->dest_info, dst_relname, &sb);
    }

  /* If we've just created a hard-link due to cp's --link option,
     we're done.  */
  if (x->hard_link && ! S_ISDIR (src_mode)
      && !(! CAN_HARDLINK_SYMLINKS && S_ISLNK (src_mode)
           && x->dereference == DEREF_NEVER))
    return delayed_ok;

  if (copied_as_regular)
    return delayed_ok;

  /* POSIX says that 'cp -p' must restore the following:
     - permission bits
     - setuid, setgid bits
     - owner and group
     If it fails to restore any of those, we may give a warning but
     the destination must not be removed.
     FIXME: implement the above. */

  /* Adjust the times (and if possible, ownership) for the copy.
     chown turns off set[ug]id bits for non-root,
     so do the chmod last.  */

  if (x->preserve_timestamps)
    {
      struct timespec timespec[2];
      timespec[0] = get_stat_atime (&src_sb);
      timespec[1] = get_stat_mtime (&src_sb);

      int utimensat_flags = dest_is_symlink ? AT_SYMLINK_NOFOLLOW : 0;
      if (utimensat (dst_dirfd, drelname, timespec, utimensat_flags) != 0)
        {
          error (0, errno, _("preserving times for %s"), quoteaf (dst_name));
          if (x->require_preserve)
            return false;
        }
    }

  /* Avoid calling chown if we know it's not necessary.  */
  if (!dest_is_symlink && x->preserve_ownership
      && (new_dst || !SAME_OWNER_AND_GROUP (src_sb, dst_sb)))
    {
      switch (set_owner (x, dst_name, dst_dirfd, drelname, -1,
                         &src_sb, new_dst, &dst_sb))
        {
        case -1:
          return false;

        case 0:
          src_mode &= ~ (S_ISUID | S_ISGID | S_ISVTX);
          break;
        }
    }

  /* Set xattrs after ownership as changing owners will clear capabilities.  */
  if (x->preserve_xattr && ! copy_attr (src_name, -1, dst_name, -1, x)
      && x->require_preserve_xattr)
    return false;

  /* The operations beyond this point may dereference a symlink.  */
  if (dest_is_symlink)
    return delayed_ok;

  set_author (dst_name, -1, &src_sb);

  if (x->preserve_mode || x->move_mode)
    {
      if (copy_acl (src_name, -1, dst_name, -1, src_mode) != 0
          && x->require_preserve)
        return false;
    }
  else if (x->set_mode)
    {
      if (set_acl (dst_name, -1, x->mode) != 0)
        return false;
    }
  else if (x->explicit_no_preserve_mode && new_dst)
    {
      int default_permissions = S_ISDIR (src_mode) || S_ISSOCK (src_mode)
                                ? S_IRWXUGO : MODE_RW_UGO;
      if (set_acl (dst_name, -1, default_permissions & ~cached_umask ()) != 0)
        return false;
    }
  else
    {
      if (omitted_permissions)
        {
          omitted_permissions &= ~ cached_umask ();

          if (omitted_permissions && !restore_dst_mode)
            {
              /* Permissions were deliberately omitted when the file
                 was created due to security concerns.  See whether
                 they need to be re-added now.  It'd be faster to omit
                 the lstat, but deducing the current destination mode
                 is tricky in the presence of implementation-defined
                 rules for special mode bits.  */
              if (new_dst && (fstatat (dst_dirfd, drelname, &dst_sb,
                                       AT_SYMLINK_NOFOLLOW)
                              != 0))
                {
                  error (0, errno, _("cannot stat %s"), quoteaf (dst_name));
                  return false;
                }
              dst_mode = dst_sb.st_mode;
              if (omitted_permissions & ~dst_mode)
                restore_dst_mode = true;
            }
        }

      if (restore_dst_mode)
        {
          if (lchmodat (dst_dirfd, drelname, dst_mode | omitted_permissions)
              != 0)
            {
              error (0, errno, _("preserving permissions for %s"),
                     quoteaf (dst_name));
              if (x->require_preserve)
                return false;
            }
        }
    }

  return delayed_ok;

un_backup:

  if (x->preserve_security_context)
    restore_default_fscreatecon_or_die ();

  /* We have failed to create the destination file.
     If we've just added a dev/ino entry via the remember_copied
     call above (i.e., unless we've just failed to create a hard link),
     remove the entry associating the source dev/ino with the
     destination file name, so we don't try to 'preserve' a link
     to a file we didn't create.  */
  if (earlier_file == nullptr)
    forget_created (src_sb.st_ino, src_sb.st_dev);

  if (dst_backup)
    {
      char const *dst_relbackup = &dst_backup[dst_relname - dst_name];
      if (renameat (dst_dirfd, dst_relbackup, dst_dirfd, drelname) != 0)
        error (0, errno, _("cannot un-backup %s"), quoteaf (dst_name));
      else
        {
          if (x->verbose)
            printf (_("%s -> %s (unbackup)\n"),
                    quoteaf_n (0, dst_backup), quoteaf_n (1, dst_name));
        }
    }
  return false;
}

static void
valid_options (const struct cp_options *co)
{
  affirm (VALID_BACKUP_TYPE (co->backup_type));
  affirm (VALID_SPARSE_MODE (co->sparse_mode));
  affirm (VALID_REFLINK_MODE (co->reflink_mode));
  affirm (!(co->hard_link && co->symbolic_link));
  affirm (!
          (co->reflink_mode == REFLINK_ALWAYS
           && co->sparse_mode != SPARSE_AUTO));
}

/* Copy the file SRC_NAME to the file DST_NAME aka DST_DIRFD+DST_RELNAME.
   If NONEXISTENT_DST is positive, DST_NAME does not exist even as a
   dangling symlink; if negative, it does not exist except possibly
   as a dangling symlink; if zero, its existence status is unknown.
   OPTIONS summarizes the command-line options.
   Set *COPY_INTO_SELF if SRC_NAME is a parent of (or the
   same as) DST_NAME; otherwise, set clear it.
   If X->move_mode, set *RENAME_SUCCEEDED according to whether
   the source was simply renamed to the destination.
   Return true if successful.  */

extern bool
copy (char const *src_name, char const *dst_name,
      int dst_dirfd, char const *dst_relname,
      int nonexistent_dst, const struct cp_options *options,
      bool *copy_into_self, bool *rename_succeeded)
{
  valid_options (options);

  /* Record the file names: they're used in case of error, when copying
     a directory into itself.  I don't like to make these tools do *any*
     extra work in the common case when that work is solely to handle
     exceptional cases, but in this case, I don't see a way to derive the
     top level source and destination directory names where they're used.
     An alternative is to use COPY_INTO_SELF and print the diagnostic
     from every caller -- but I don't want to do that.  */
  top_level_src_name = src_name;
  top_level_dst_name = dst_name;

  bool first_dir_created_per_command_line_arg = false;
  return copy_internal (src_name, dst_name, dst_dirfd, dst_relname,
                        nonexistent_dst, nullptr, nullptr,
                        options, true,
                        &first_dir_created_per_command_line_arg,
                        copy_into_self, rename_succeeded);
}

/* Set *X to the default options for a value of type struct cp_options.  */

extern void
cp_options_default (struct cp_options *x)
{
  memset (x, 0, sizeof *x);
#ifdef PRIV_FILE_CHOWN
  {
    priv_set_t *pset = priv_allocset ();
    if (!pset)
      xalloc_die ();
    if (getppriv (PRIV_EFFECTIVE, pset) == 0)
      {
        x->chown_privileges = priv_ismember (pset, PRIV_FILE_CHOWN);
        x->owner_privileges = priv_ismember (pset, PRIV_FILE_OWNER);
      }
    priv_freeset (pset);
  }
#else
  x->chown_privileges = x->owner_privileges = (geteuid () == ROOT_UID);
#endif
  x->rename_errno = -1;
}

/* Return true if it's OK for chown to fail, where errno is
   the error number that chown failed with and X is the copying
   option set.  */

extern bool
chown_failure_ok (struct cp_options const *x)
{
  /* If non-root uses -p, it's ok if we can't preserve ownership.
     But root probably wants to know, e.g. if NFS disallows it,
     or if the target system doesn't support file ownership.  */

  return ((errno == EPERM || errno == EINVAL) && !x->chown_privileges);
}

/* Similarly, return true if it's OK for chmod and similar operations
   to fail, where errno is the error number that chmod failed with and
   X is the copying option set.  */

static bool
owner_failure_ok (struct cp_options const *x)
{
  return ((errno == EPERM || errno == EINVAL) && !x->owner_privileges);
}

/* Return the user's umask, caching the result.

   FIXME: If the destination's parent directory has has a default ACL,
   some operating systems (e.g., GNU/Linux's "POSIX" ACLs) use that
   ACL's mask rather than the process umask.  Currently, the callers
   of cached_umask incorrectly assume that this situation cannot occur.  */
extern mode_t
cached_umask (void)
{
  static mode_t mask = (mode_t) -1;
  if (mask == (mode_t) -1)
    {
      mask = umask (0);
      umask (mask);
    }
  return mask;
}
