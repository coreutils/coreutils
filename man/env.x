'\" Copyright (C) 1998-2018 Free Software Foundation, Inc.
'\"
'\" This is free software.  You may redistribute copies of it under the terms
'\" of the GNU General Public License <https://www.gnu.org/licenses/gpl.html>.
'\" There is NO WARRANTY, to the extent permitted by law.
[NAME]
env \- run a program in a modified environment
[DESCRIPTION]
.\" Add any additional description here
[OPTIONS]
.SS "\-S/\-\-split\-string usage in scripts"
The
.B \-S
option allows specifing multiple parameters in a script.
Running a script named
.B 1.pl
containing the following first line:
.PP
.RS
.nf
#!/usr/bin/env \-S perl \-w \-T
...
.fi
.RE
.PP
Will execute
.B "perl \-w \-T 1.pl".
.PP
Without the
.B '\-S'
parameter the script will likely fail with:
.PP
.RS
.nf
/usr/bin/env: 'perl \-w \-T': No such file or directory
.fi
.RE
.PP
See the full documentation for more details.
