[NAME]
du \- estimate file space usage
[DESCRIPTION]
.\" Add any additional description here
[PATTERNS]
PATTERN is a shell pattern (not a regular expression).  The pattern
.BR ?
matches any one character, whereas
.BR *
matches any string (composed of zero, one or multiple characters).  For
example,
.BR *.o
will match any files whose names end in
.BR .o .
Therefore, the command
.IP
.B du --exclude='*.o'
.PP
will skip all files and subdirectories ending in
.BR .o
(including the file
.BR .o
itself).
