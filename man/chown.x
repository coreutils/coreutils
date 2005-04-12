[NAME]
chown \- change file owner and group
[DESCRIPTION]
This manual page
documents the GNU version of
.BR chown .
.B chown
changes the user and/or group ownership of each given file.  If
only an owner (a user name or numeric user ID) is given, that user is made the
owner of each given file, and the files' group is not changed.  If the
owner is followed by a colon and a group name (or numeric group ID),
with no spaces between them, the group ownership of the files is
changed as well.  If a colon but no group name follows the user name,
that user is made the owner of the files and the group of the files is
changed to that user's login group.  If the colon and group are given,
but the owner is omitted, only the group of the files is changed;
in this case,
.B chown
performs the same function as
.BR chgrp .
If only a colon is given, or if the entire operand is empty, neither the
owner nor the group is changed.
.SH OPTIONS
