[NAME]
chown \- change file owner and group
[DESCRIPTION]
This manual page
documents the GNU version of
.BR chown .
.B chown
changes the user and/or group ownership of each given file, according
to its first non-option argument, which is interpreted as follows.  If
only a user name (or numeric user ID) is given, that user is made the
owner of each given file, and the files' group is not changed.  If the
user name is followed by a colon or dot and a group name (or numeric group ID),
with no spaces between them, the group ownership of the files is
changed as well.  If a colon or dot but no group name follows the user name,
that user is made the owner of the files and the group of the files is
changed to that user's login group.  If the colon or dot and group are given,
but the user name is omitted, only the group of the files is changed;
in this case,
.B chown
performs the same function as
.BR chgrp .
.SH OPTIONS
