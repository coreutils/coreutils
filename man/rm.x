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
does not begin with `y' or `Y', the file is skipped.
.LP
GNU
.BR rm ,
like every program that uses the getopt function to parse its
arguments, lets you use the
.I \-\-
option to indicate that all following arguments are non-options.  To
remove a file called `\-f' in the current directory, you could type
either
.RS
rm \-\- \-f
.RE
or
.RS
rm ./\-f
.RE
The Unix
.B rm
program's use of a single `\-' for this purpose predates the
development of the getopt standard syntax.
.SH OPTIONS
