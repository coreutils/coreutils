.\" Copyright (C) 1994, 95, 96, 97, 98 Free Software Foundation, Inc.
.\"
.\" Permission is granted to make and distribute verbatim copies of this
.\" manual provided the copyright notice and this permission notice are
.\" preserved on all copies.
.\"
.\" Permission is granted to copy and distribute modified versions of
.\" this manual under the conditions for verbatim copying, provided that
.\" the entire resulting derived work is distributed under the terms of a
.\" permission notice identical to this one.
.\"
.\" Permission is granted to copy and distribute translations of this
.\" manual into another language, under the above conditions for modified
.\" versions, except that this permission notice may be stated in a
.\" translation approved by the Foundation.
.\"
[DESCRIPTION]
This manual page
documents the GNU version of
.BR df .
.B df
displays the amount of disk space available on the filesystem
containing each file name argument.  If no file name is given, the
space available on all currently mounted filesystems is shown.  Disk
space is shown in 1K blocks by default, unless the environment
variable POSIXLY_CORRECT is set, in which case 512-byte blocks are
used.
.PP
If an argument is the absolute file name of a disk device node containing a
mounted filesystem,
.B df
shows the space available on that filesystem rather than on the
filesystem containing the device node (which is always the root
filesystem).  This version of
.B df
cannot show the space available on unmounted filesystems, because on
most kinds of systems doing so requires very nonportable intimate
knowledge of filesystem structures.
.SH OPTIONS
