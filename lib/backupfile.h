/* backupfile.h -- declarations for making Emacs style backup file names

   Copyright (C) 1990, 1991, 1992, 1997, 1998, 1999, 2003, 2004 Free
   Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.
   If not, write to the Free Software Foundation,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

#ifndef BACKUPFILE_H_
# define BACKUPFILE_H_

# ifdef __cplusplus
extern "C" {
# endif


/* When to make backup files. */
enum backup_type
{
  /* Never make backups. */
  no_backups,

  /* Make simple backups of every file. */
  simple_backups,

  /* Make numbered backups of files that already have numbered backups,
     and simple backups of the others. */
  numbered_existing_backups,

  /* Make numbered backups of every file. */
  numbered_backups
};

# define VALID_BACKUP_TYPE(Type)	\
  ((unsigned int) (Type) <= numbered_backups)

extern char const *simple_backup_suffix;

char *find_backup_file_name (char const *, enum backup_type);
enum backup_type get_version (char const *context, char const *arg);
enum backup_type xget_version (char const *context, char const *arg);
void addext (char *, char const *, int);


# ifdef __cplusplus
}
# endif

#endif /* ! BACKUPFILE_H_ */
