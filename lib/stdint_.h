/* Copyright (C) 2001-2002, 2004-2006 Free Software Foundation, Inc.
   Written by Bruno Haible, Sam Steingold, Peter Burwood.
   This file is part of gnulib.

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

#ifndef _STDINT_H
#define _STDINT_H

/*
 * ISO C 99 <stdint.h> for platforms that lack it.
 * <http://www.opengroup.org/onlinepubs/007904975/basedefs/stdint.h.html>
 */

/* Get wchar_t, WCHAR_MIN, WCHAR_MAX.  */
#include <stddef.h>
/* Get CHAR_BIT, LONG_MIN, LONG_MAX, ULONG_MAX.  */
#include <limits.h>

/* Get those types that are already defined in other system include files.  */
#if defined(__FreeBSD__)
# include <sys/inttypes.h>
#endif
#if defined(__linux__) && HAVE_SYS_BITYPES_H
  /* Linux libc4 >= 4.6.7 and libc5 have a <sys/bitypes.h> that defines
     int{8,16,32,64}_t and __BIT_TYPES_DEFINED__.  In libc5 >= 5.2.2 it is
     included by <sys/types.h>.  */
# include <sys/bitypes.h>
#endif
#if defined(__sun) && HAVE_SYS_INTTYPES_H
# include <sys/inttypes.h>
  /* Solaris 7 <sys/inttypes.h> has the types except the *_fast*_t types, and
     the macros except for *_FAST*_*, INTPTR_MIN, PTRDIFF_MIN, PTRDIFF_MAX.
     But note that <sys/int_types.h> contains only the type definitions!  */
# define _STDINT_H_HAVE_SYSTEM_INTTYPES
#endif
#if (defined(__hpux) || defined(_AIX)) && HAVE_INTTYPES_H
# include <inttypes.h>
  /* HP-UX 10 <inttypes.h> has nearly everything, except UINT_LEAST8_MAX,
     UINT_FAST8_MAX, PTRDIFF_MIN, PTRDIFF_MAX.  */
  /* AIX 4 <inttypes.h> has nearly everything, except INTPTR_MIN, INTPTR_MAX,
     UINTPTR_MAX, PTRDIFF_MIN, PTRDIFF_MAX.  */
# define _STDINT_H_HAVE_SYSTEM_INTTYPES
#endif
#if !((defined(UNIX_CYGWIN32) || defined(__linux__)) && defined(__BIT_TYPES_DEFINED__))
# define _STDINT_H_NEED_SIGNED_INT_TYPES
#endif

#if !defined(_STDINT_H_HAVE_SYSTEM_INTTYPES)

/* 7.18.1.1. Exact-width integer types */

#if !defined(__FreeBSD__)

#ifdef _STDINT_H_NEED_SIGNED_INT_TYPES
typedef signed char    int8_t;
#endif
typedef unsigned char  uint8_t;

#ifdef _STDINT_H_NEED_SIGNED_INT_TYPES
typedef short          int16_t;
#endif
typedef unsigned short uint16_t;

#ifdef _STDINT_H_NEED_SIGNED_INT_TYPES
typedef int            int32_t;
#endif
typedef unsigned int   uint32_t;

#if @HAVE_LONG_64BIT@
#ifdef _STDINT_H_NEED_SIGNED_INT_TYPES
typedef long           int64_t;
#endif
typedef unsigned long  uint64_t;
#define _STDINT_H_HAVE_INT64
#elif @HAVE_LONG_LONG_64BIT@
#ifdef _STDINT_H_NEED_SIGNED_INT_TYPES
typedef long long          int64_t;
#endif
typedef unsigned long long uint64_t;
#define _STDINT_H_HAVE_INT64
#elif defined(_MSC_VER)
typedef __int64          int64_t;
typedef unsigned __int64 uint64_t;
#define _STDINT_H_HAVE_INT64
#endif

#endif /* !FreeBSD */

/* 7.18.1.2. Minimum-width integer types */

typedef int8_t   int_least8_t;
typedef uint8_t  uint_least8_t;
typedef int16_t  int_least16_t;
typedef uint16_t uint_least16_t;
typedef int32_t  int_least32_t;
typedef uint32_t uint_least32_t;
#ifdef _STDINT_H_HAVE_INT64
typedef int64_t  int_least64_t;
typedef uint64_t uint_least64_t;
#endif

/* 7.18.1.3. Fastest minimum-width integer types */

typedef int32_t  int_fast8_t;
typedef uint32_t uint_fast8_t;
typedef int32_t  int_fast16_t;
typedef uint32_t uint_fast16_t;
typedef int32_t  int_fast32_t;
typedef uint32_t uint_fast32_t;
#ifdef _STDINT_H_HAVE_INT64
typedef int64_t  int_fast64_t;
typedef uint64_t uint_fast64_t;
#endif

/* 7.18.1.4. Integer types capable of holding object pointers */

#if !defined(__FreeBSD__)

/* On some platforms (like IRIX6 MIPS with -n32) sizeof(void*) < sizeof(long),
   but this doesn't matter here.  */
typedef long          intptr_t;
typedef unsigned long uintptr_t;

#endif /* !FreeBSD */

/* 7.18.1.5. Greatest-width integer types */

#ifdef _STDINT_H_HAVE_INT64
# ifndef intmax_t
typedef int64_t  intmax_t;
# endif
# ifndef uintmax_t
typedef uint64_t uintmax_t;
# endif
#else
# ifndef intmax_t
typedef int32_t  intmax_t;
# endif
# ifndef uintmax_t
typedef uint32_t uintmax_t;
# endif
#endif

/* 7.18.2. Limits of specified-width integer types */

#if !defined(__cplusplus) || defined(__STDC_LIMIT_MACROS)

/* 7.18.2.1. Limits of exact-width integer types */

#define INT8_MIN  -128
#define INT8_MAX   127
#define UINT8_MAX  255U
#define INT16_MIN  -32768
#define INT16_MAX   32767
#define UINT16_MAX  65535U
#define INT32_MIN   (~INT32_MAX)
#define INT32_MAX   2147483647
#define UINT32_MAX  4294967295U
#ifdef _STDINT_H_HAVE_INT64
#define INT64_MIN   (~INT64_MAX)
#if @HAVE_LONG_64BIT@
#define INT64_MAX   9223372036854775807L
#define UINT64_MAX 18446744073709551615UL
#elif @HAVE_LONG_LONG_64BIT@
#define INT64_MAX   9223372036854775807LL
#define UINT64_MAX 18446744073709551615ULL
#elif defined(_MSC_VER)
#define INT64_MAX   9223372036854775807i64
#define UINT64_MAX 18446744073709551615ui64
#endif
#endif

/* 7.18.2.2. Limits of minimum-width integer types */

#define INT_LEAST8_MIN INT8_MIN
#define INT_LEAST8_MAX INT8_MAX
#define UINT_LEAST8_MAX UINT8_MAX
#define INT_LEAST16_MIN INT16_MIN
#define INT_LEAST16_MAX INT16_MAX
#define UINT_LEAST16_MAX UINT16_MAX
#define INT_LEAST32_MIN INT32_MIN
#define INT_LEAST32_MAX INT32_MAX
#define UINT_LEAST32_MAX UINT32_MAX
#ifdef _STDINT_H_HAVE_INT64
#define INT_LEAST64_MIN INT64_MIN
#define INT_LEAST64_MAX INT64_MAX
#define UINT_LEAST64_MAX UINT64_MAX
#endif

/* 7.18.2.3. Limits of fastest minimum-width integer types */

#define INT_FAST8_MIN INT32_MIN
#define INT_FAST8_MAX INT32_MAX
#define UINT_FAST8_MAX UINT32_MAX
#define INT_FAST16_MIN INT32_MIN
#define INT_FAST16_MAX INT32_MAX
#define UINT_FAST16_MAX UINT32_MAX
#define INT_FAST32_MIN INT32_MIN
#define INT_FAST32_MAX INT32_MAX
#define UINT_FAST32_MAX UINT32_MAX
#ifdef _STDINT_H_HAVE_INT64
#define INT_FAST64_MIN INT64_MIN
#define INT_FAST64_MAX INT64_MAX
#define UINT_FAST64_MAX UINT64_MAX
#endif

/* 7.18.2.4. Limits of integer types capable of holding object pointers */

#define INTPTR_MIN LONG_MIN
#define INTPTR_MAX LONG_MAX
#define UINTPTR_MAX ULONG_MAX

/* 7.18.2.5. Limits of greatest-width integer types */

#ifdef _STDINT_H_HAVE_INT64
#define INTMAX_MIN INT64_MIN
#define INTMAX_MAX INT64_MAX
#define UINTMAX_MAX UINT64_MAX
#else
#define INTMAX_MIN INT32_MIN
#define INTMAX_MAX INT32_MAX
#define UINTMAX_MAX UINT32_MAX
#endif

/* 7.18.3. Limits of other integer types */

#define PTRDIFF_MIN (~(ptrdiff_t)0 << (sizeof(ptrdiff_t)*CHAR_BIT-1))
#define PTRDIFF_MAX (~PTRDIFF_MIN)

/* This may be wrong...  */
#define SIG_ATOMIC_MIN 0
#define SIG_ATOMIC_MAX 127

#ifndef SIZE_MAX /* SIZE_MAX may also be defined in config.h. */
# define SIZE_MAX ((size_t)~(size_t)0)
#endif

/* wchar_t limits already defined in <stddef.h>.  */
/* wint_t limits already defined in <wchar.h>.  */

#endif

/* 7.18.4. Macros for integer constants */

#if !defined(__cplusplus) || defined(__STDC_CONSTANT_MACROS)

/* 7.18.4.1. Macros for minimum-width integer constants */

#define INT8_C(x) x
#define UINT8_C(x) x##U
#define INT16_C(x) x
#define UINT16_C(x) x##U
#define INT32_C(x) x
#define UINT32_C(x) x##U
#if @HAVE_LONG_64BIT@
#define INT64_C(x) x##L
#define UINT64_C(x) x##UL
#elif @HAVE_LONG_LONG_64BIT@
#define INT64_C(x) x##LL
#define UINT64_C(x) x##ULL
#elif defined(_MSC_VER)
#define INT64_C(x) x##i64
#define UINT64_C(x) x##ui64
#endif

/* 7.18.4.2. Macros for greatest-width integer constants */

#if @HAVE_LONG_64BIT@
#define INTMAX_C(x) x##L
#define UINTMAX_C(x) x##UL
#elif @HAVE_LONG_LONG_64BIT@
#define INTMAX_C(x) x##LL
#define UINTMAX_C(x) x##ULL
#elif defined(_MSC_VER)
#define INTMAX_C(x) x##i64
#define UINTMAX_C(x) x##ui64
#else
#define INTMAX_C(x) x
#define UINTMAX_C(x) x##U
#endif

#endif

#endif  /* !_STDINT_H_HAVE_SYSTEM_INTTYPES */

#endif /* _STDINT_H */
