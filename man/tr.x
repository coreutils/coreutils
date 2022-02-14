[NAME]
tr \- translate or delete characters
[DESCRIPTION]
.\" Add any additional description here
[BUGS]
.PP
Full support is available only for safe single-byte locales,
in which every possible input byte represents a single character.
The C locale is safe in GNU systems, so you can avoid this issue
in the shell by running
.B "LC_ALL=C tr"
instead of plain
.BR tr .
