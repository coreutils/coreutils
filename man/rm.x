[NAME]
rm \- remove files or directories
[DESCRIPTION]
This manual page
documents the GNU version of
.BR rm .
.B rm
removes each specified file.  By default, it does not remove
directories.
.P
If a file is unwritable, the standard input is a tty, and
the \fI\-f\fR or \fI\-\-force\fR option is not given,
.B rm
prompts the user for whether to remove the file.  If the response
is not affirmative, the file is skipped.
.SH OPTIONS
[SEE ALSO]
chattr(1), shred(1)
