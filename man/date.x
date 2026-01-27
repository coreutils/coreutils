'\" Copyright (C) 1998-2026 Free Software Foundation, Inc.
'\"
'\" This is free software.  You may redistribute copies of it under the terms
'\" of the GNU General Public License <https://www.gnu.org/licenses/gpl.html>.
'\" There is NO WARRANTY, to the extent permitted by law.
[NAME]
date \- print or set the system date and time
[SYNOPSIS]
.B date
[\fI\,OPTION\/\fR]... [\fB+\fR\fI\,FORMAT\/\fR]
.br
.B date
[\fI\,OPTION\/\fR]... \
\fIMMDDhhmm\/\fR[[\fI\,CC\/\fR]\fI\,YY\/\fR][\fB.\fI\,ss\/\fR]
[DESCRIPTION]
.\" Add any additional description here
[DATE STRING]
.\" NOTE: keep this paragraph in sync with the one in touch.x
The --date=STRING is a mostly free format human readable date string
such as "Sun, 29 Feb 2004 16:21:42 -0800" or "2004-02-29 16:21:42" or
even "next Thursday".  A date string may contain items indicating
calendar date, time of day, time zone, day of week, relative time,
relative date, and numbers.  An empty string indicates the beginning
of the day.  The date string format is more complex than is easily
documented here but is fully described in the info documentation.
