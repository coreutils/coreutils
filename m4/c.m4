# c.m4 serial 1
dnl Copyright (C) 2001, 2002, 2003, 2004, 2005 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl This code stolen from CVS Autoconf lib/autoconf/c.m4 on 2005-11-17.
dnl It can be removed once we assume Autoconf 2.60 or later.

# -------------------------------- #
# 4b. C compiler characteristics.  #
# -------------------------------- #

# _AC_PROG_CC_C89 ([ACTION-IF-AVAILABLE], [ACTION-IF-UNAVAILABLE])
# ----------------------------------------------------------------
# If the C compiler is not in ANSI C89 (ISO C90) mode by default, try
# to add an option to output variable CC to make it so.  This macro
# tries various options that select ANSI C89 on some system or
# another.  It considers the compiler to be in ANSI C89 mode if it
# handles function prototypes correctly.
AC_DEFUN([_AC_PROG_CC_C89],
[_AC_C_STD_TRY([c89],
[[#include <stdarg.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
/* Most of the following tests are stolen from RCS 5.7's src/conf.sh.  */
struct buf { int x; };
FILE * (*rcsopen) (struct buf *, struct stat *, int);
static char *e (p, i)
     char **p;
     int i;
{
  return p[i];
}
static char *f (char * (*g) (char **, int), char **p, ...)
{
  char *s;
  va_list v;
  va_start (v,p);
  s = g (p, va_arg (v,int));
  va_end (v);
  return s;
}

/* OSF 4.0 Compaq cc is some sort of almost-ANSI by default.  It has
   function prototypes and stuff, but not '\xHH' hex character constants.
   These don't provoke an error unfortunately, instead are silently treated
   as 'x'.  The following induces an error, until -std is added to get
   proper ANSI mode.  Curiously '\x00'!='x' always comes out true, for an
   array size at least.  It's necessary to write '\x00'==0 to get something
   that's true only with -std.  */
int osf4_cc_array ['\x00' == 0 ? 1 : -1];

int test (int i, double x);
struct s1 {int (*f) (int a);};
struct s2 {int (*f) (double a);};
int pairnames (int, char **, FILE *(*)(struct buf *, struct stat *, int), int, int);
int argc;
char **argv;]],
[[return f (e, argv, 0) != argv[0]  ||  f (e, argv, 1) != argv[1];]],
dnl Don't try gcc -ansi; that turns off useful extensions and
dnl breaks some systems' header files.
dnl AIX circa 2003	-qlanglvl=extc89
dnl old AIX		-qlanglvl=ansi
dnl Ultrix, OSF/1, Tru64	-std
dnl HP-UX 10.20 and later	-Ae
dnl HP-UX older versions	-Aa -D_HPUX_SOURCE
dnl SVR4			-Xc -D__EXTENSIONS__
[-qlanglvl=extc89 -qlanglvl=ansi -std \
	-Ae "-Aa -D_HPUX_SOURCE" "-Xc -D__EXTENSIONS__"], [$1], [$2])[]dnl
])# _AC_PROG_CC_C89


# _AC_C_STD_TRY(STANDARD, TEST-PROLOGUE, TEST-BODY, OPTION-LIST,
#		ACTION-IF-AVAILABLE, ACTION-IF-UNAVAILABLE)
# --------------------------------------------------------------
# Check whether the C compiler accepts features of STANDARD (e.g `c89', `c99')
# by trying to compile a program of TEST-PROLOGUE and TEST-BODY.  If this fails,
# try again with each compiler option in the space-separated OPTION-LIST; if one
# helps, append it to CC.  If eventually successful, run ACTION-IF-AVAILABLE,
# else ACTION-IF-UNAVAILABLE.
AC_DEFUN([_AC_C_STD_TRY],
[AC_MSG_CHECKING([for $CC option to accept ISO ]m4_translit($1, [c], [C]))
AC_CACHE_VAL(ac_cv_prog_cc_$1,
[ac_cv_prog_cc_$1=no
ac_save_CC=$CC
AC_LANG_CONFTEST([AC_LANG_PROGRAM([$2], [$3])])
for ac_arg in '' $4
do
  CC="$ac_save_CC $ac_arg"
  _AC_COMPILE_IFELSE([], [ac_cv_prog_cc_$1=$ac_arg])
  test "x$ac_cv_prog_cc_$1" != "xno" && break
done
rm -f conftest.$ac_ext
CC=$ac_save_CC
])# AC_CACHE_VAL
case "x$ac_cv_prog_cc_$1" in
  x)
    AC_MSG_RESULT([none needed]) ;;
  xno)
    AC_MSG_RESULT([unsupported]) ;;
  *)
    CC="$CC $ac_cv_prog_cc_$1"
    AC_MSG_RESULT([$ac_cv_prog_cc_$1]) ;;
esac
AS_IF([test "x$ac_cv_prog_cc_$1" != xno], [$5], [$6])
])# _AC_C_STD_TRY


# _AC_PROG_CC_C99 ([ACTION-IF-AVAILABLE], [ACTION-IF-UNAVAILABLE])
# ----------------------------------------------------------------
# If the C compiler is not in ISO C99 mode by default, try to add an
# option to output variable CC to make it so.  This macro tries
# various options that select ISO C99 on some system or another.  It
# considers the compiler to be in ISO C99 mode if it handles mixed
# code and declarations, _Bool, inline and restrict.
AC_DEFUN([_AC_PROG_CC_C99],
[_AC_C_STD_TRY([c99],
[[#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <wchar.h>
#include <stdio.h>

struct incomplete_array
{
  int datasize;
  double data[];
};

struct named_init {
  int number;
  const wchar_t *name;
  double average;
};

static inline int
test_restrict(const char *restrict text)
{
  // See if C++-style comments work.
  // Iterate through items via the restricted pointer.
  // Also check for declarations in for loops.
  for (unsigned int i = 0; *(text+i) != '\0'; ++i)
    continue;
  return 0;
}

// Check varargs and va_copy work.
static void
test_varargs(const char *format, ...)
{
  va_list args;
  va_start(args, format);
  va_list args_copy;
  va_copy(args_copy, args);

  const char *str;
  int number;
  float fnumber;

  while (*format)
    {
      switch (*format++)
	{
	case 's': // string
	  str = va_arg(args_copy, const char *);
	  break;
	case 'd': // int
	  number = va_arg(args_copy, int);
	  break;
	case 'f': // float
	  fnumber = (float) va_arg(args_copy, double);
	  break;
	default:
	  break;
	}
    }
  va_end(args_copy);
  va_end(args);
}
]],
[[
  // Check bool and long long datatypes.
  _Bool success = false;
  long long int bignum = -1234567890LL;
  unsigned long long int ubignum = 1234567890uLL;

  // Check restrict.
  if (test_restrict("String literal") != 0)
    success = true;
  char *restrict newvar = "Another string";

  // Check varargs.
  test_varargs("s, d' f .", "string", 65, 34.234);

  // Check incomplete arrays work.
  struct incomplete_array *ia =
    malloc(sizeof(struct incomplete_array) + (sizeof(double) * 10));
  ia->datasize = 10;
  for (int i = 0; i < ia->datasize; ++i)
    ia->data[i] = (double) i * 1.234;

  // Check named initialisers.
  struct named_init ni = {
    .number = 34,
    .name = L"Test wide string",
    .average = 543.34343,
  };

  ni.number = 58;

  int dynamic_array[ni.number];
  dynamic_array[43] = 543;
]],
dnl Try
dnl GCC		-std=gnu99 (unused restrictive modes: -std=c99 -std=iso9899:1999)
dnl AIX		-qlanglvl=extc99 (unused restrictive mode: -qlanglvl=stdc99)
dnl Intel ICC	-c99
dnl IRIX	-c99
dnl Solaris	(unused because it causes the compiler to assume C99 semantics for
dnl		library functions, and this is invalid before Solaris 10: -xc99)
dnl Tru64	-c99
dnl with extended modes being tried first.
[[-std=gnu99 -c99 -qlanglvl=extc99]], [$1], [$2])[]dnl
])# _AC_PROG_CC_C99


# AC_PROG_CC_C89
# --------------
AC_DEFUN([AC_PROG_CC_C89],
[ _AC_PROG_CC_C89
])


# AC_PROG_CC_C99
# --------------
AC_DEFUN([AC_PROG_CC_C99],
[ _AC_PROG_CC_C99
])


# _AC_PROG_CC_STDC
# ----------------
AC_DEFUN([_AC_PROG_CC_STDC],
[ _AC_PROG_CC_C99([ac_cv_prog_cc_stdc=$ac_cv_prog_cc_c99],
                  [_AC_PROG_CC_C89([ac_cv_prog_cc_stdc=$ac_cv_prog_cc_c89], [no])])dnl
  AC_MSG_CHECKING([for $CC option to accept ISO Standard C])
  AC_CACHE_VAL([ac_cv_prog_cc_stdc], [])
  case "x$ac_cv_prog_cc_stdc" in
    xno)
      AC_MSG_RESULT([unsupported])
      ;;
    *)
      if test "x$ac_cv_prog_cc_stdc" = x; then
        AC_MSG_RESULT([none needed])
      else
        AC_MSG_RESULT([$ac_cv_prog_cc_stdc])
      fi
      ;;
  esac
])
