[NAME]
chmod \- change file access permissions
[DESCRIPTION]
This manual page
documents the GNU version of
.BR chmod .
.B chmod
changes the permissions of each given file according to
.IR mode ,
which can be either a symbolic representation of changes to make, or
an octal number representing the bit pattern for the new permissions.
.PP
The format of a symbolic mode is
`[ugoa...][[+-=][rwxXstugo...]...][,...]'.  Multiple symbolic
operations can be given, separated by commas.
.PP
A combination of the letters `ugoa' controls which users' access to
the file will be changed: the user who owns it (u), other users in the
file's group (g), other users not in the file's group (o), or all
users (a).  If none of these are given, the effect is as if `a' were
given, but bits that are set in the umask are not affected.
.PP
The operator `+' causes the permissions selected to be added to the
existing permissions of each file; `-' causes them to be removed; and
`=' causes them to be the only permissions that the file has.
.PP
The letters `rwxXstugo' select the new permissions for the affected
users: read (r), write (w), execute (or access for directories) (x),
execute only if the file is a directory or already has execute
permission for some user (X), set user or group ID on execution (s),
save program text on swap device (t), the permissions that the user
who owns the file currently has for it (u), the permissions that other
users in the file's group have for it (g), and the permissions that
other users not in the file's group have for it (o).
.PP
A numeric mode is from one to four octal digits (0-7), derived by
adding up the bits with values 4, 2, and 1.  Any omitted digits are
assumed to be leading zeros.  The first digit selects the set user ID
(4) and set group ID (2) and save text image (1) attributes.  The
second digit selects permissions for the user who owns the file: read
(4), write (2), and execute (1); the third selects permissions for
other users in the file's group, with the same values; and the fourth
for other users not in the file's group, with the same values.
.PP
.B chmod
never changes the permissions of symbolic links; the
.B chmod
system call cannot change their permissions.  This is not a problem
since the permissions of symbolic links are never used.
However, for each symbolic link listed on the command line,
.B chmod
changes the permissions of the pointed-to file.
In contrast,
.B chmod
ignores symbolic links encountered during recursive directory
traversals.
.SH OPTIONS
