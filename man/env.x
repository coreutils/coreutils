'\" Copyright (C) 1998-2020 Free Software Foundation, Inc.
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
option allows specifying multiple parameters in a script.
Running a script named
.B 1.pl
containing the following first line:
.PP
.RS
.nf
#!/usr/bin/env \-S perl \-w \-T
\&...
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
.PP
.SS "\-\-default-signal[=SIG]" usage
This option allows setting a signal handler to its default
action, which is not possible using the traditional shell
trap command.  The following example ensures that seq
will be terminated by SIGPIPE no matter how this signal
is being handled in the process invoking the command.

.PP
.RS
.nf
sh \-c 'env \-\-default-signal=PIPE seq inf | head \-n1'
.fi
.RE
.PP

[NOTES]
POSIX's exec(2) pages says:
.RS
"many existing applications wrongly assume that they start with certain
signals set to the default action and/or unblocked.... Therefore, it is best
not to block or ignore signals across execs without explicit reason to do so,
and especially not to block signals across execs of arbitrary (not closely
cooperating) programs."
.RE

[SEE ALSO]
sigaction(2), sigprocmask(2), signal(7)
