[NAME]
df \- report file system disk space usage
[DESCRIPTION]
This manual page
documents the GNU version of
.BR df .
.B df
displays the amount of disk space available on the file system
containing each file name argument.  If no file name is given, the
space available on all currently mounted file systems is shown.  Disk
space is shown in 1K blocks by default, unless the environment
variable POSIXLY_CORRECT is set, in which case 512-byte blocks are
used.
.PP
If an argument is the absolute file name of a disk device node containing a
mounted file system,
.B df
shows the space available on that file system rather than on the
file system containing the device node (which is always the root
file system).  This version of
.B df
cannot show the space available on unmounted file systems, because on
most kinds of systems doing so requires very nonportable intimate
knowledge of file system structures.
.SH OPTIONS
