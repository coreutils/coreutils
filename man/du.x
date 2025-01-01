'\" Copyright (C) 1998-2025 Free Software Foundation, Inc.
'\"
'\" This is free software.  You may redistribute copies of it under the terms
'\" of the GNU General Public License <https://www.gnu.org/licenses/gpl.html>.
'\" There is NO WARRANTY, to the extent permitted by law.
[NAME]
du \- estimate file space usage
[DESCRIPTION]
.\" Add any additional description here
[PATTERNS]
PATTERN is a shell pattern (not a regular expression).  The pattern
.B ?\&
matches any one character, whereas
.B *
matches any string (composed of zero, one or multiple characters).  For
example,
.B *.o
will match any files whose names end in
.BR .o .
Therefore, the command
.IP
.B du \-\-exclude=\(aq*.o\(aq
.PP
will skip all files and subdirectories ending in
.B .o
(including the file
.B .o
itself).
