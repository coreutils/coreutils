/* Copyright (C) 2001, 2002, 2003, 2005 Free Software Foundation, Inc.
   Written by Bruno Haible <haible@clisp.cons.org>, 2001.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

#ifndef _STDBOOL_H
#define _STDBOOL_H

/* ISO C 99 <stdbool.h> for platforms that lack it.  */

/* Usage suggestions:

   Programs that use <stdbool.h> should be aware of some limitations
   and standards compliance issues.

   Standards compliance:

       - <stdbool.h> must be #included before 'bool', 'false', 'true'
         can be used.

       - You cannot assume that sizeof (bool) == 1.

       - Programs should not undefine the macros bool, true, and false,
         as C99 lists that as an "obsolescent feature".

   Limitations of this substitute, when used in a C89 environment:

       - <stdbool.h> must be #included before the '_Bool' type can be used.

       - You cannot assume that _Bool is a typedef; it might be a macro.

       - In C99, casts and automatic conversions to '_Bool' or 'bool' are
         performed in such a way that every nonzero value gets converted
         to 'true', and zero gets converted to 'false'.  This doesn't work
         with this substitute.  With this substitute, only the values 0 and 1
         give the expected result when converted to _Bool' or 'bool'.

   Also, it is suggested that programs use 'bool' rather than '_Bool';
   this isn't required, but 'bool' is more common.  */


/* 7.16. Boolean type and values */

/* BeOS <sys/socket.h> already #defines false 0, true 1.  We use the same
   definitions below, which is OK.  */
#ifdef __BEOS__
# include <OS.h> /* defines bool but not _Bool */
#endif

/* C++ and BeOS have a reliable bool (and _Bool, if it exists).
   Otherwise, since this file is being compiled, the system
   <stdbool.h> is not reliable so assume that the system _Bool is not
   reliable either.  Under that assumption, it is tempting to write

      typedef enum { false, true } _Bool;

   so that gdb prints values of type 'bool' symbolically.  But if we do
   this, values of type '_Bool' may promote to 'int' or 'unsigned int'
   (see ISO C 99 6.7.2.2.(4)); however, '_Bool' must promote to 'int'
   (see ISO C 99 6.3.1.1.(2)).  We could instead try this:

      typedef enum { _Bool_dummy = -1, false, true } _Bool;

   as the negative value ensures that '_Bool' promotes to 'int'.
   However, this runs into some other problems.  First, Sun's C
   compiler when (__SUNPRO_C < 0x550 || __STDC__ == 1) issues a stupid
   "warning: _Bool is a keyword in ISO C99".  Second, IBM's AIX cc
   compiler 6.0.0.0 (and presumably other versions) mishandles
   subscripts involving _Bool (effectively, _Bool promotes to unsigned
   int in this case), and we need to redefine _Bool in that case.
   Third, HP-UX 10.20's C compiler lacks <stdbool.h> but has _Bool and
   mishandles comparisons of _Bool to int (it promotes _Bool to
   unsigned int).

   The simplest way to work around these problems is to ignore any
   existing definition of _Bool and use our own.  */

#if defined __cplusplus || defined __BEOS__
# if !@HAVE__BOOL@
typedef bool _Bool;
# endif
#else
# define _Bool signed char
#endif

#define bool _Bool

/* The other macros must be usable in preprocessor directives.  */
#define false 0
#define true 1
#define __bool_true_false_are_defined 1

#endif /* _STDBOOL_H */
