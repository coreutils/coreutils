/* Definitions for data structures and routines for the regular
   expression library.
   Copyright (C) 1985,1989-93,1995-98,2000,2001,2002,2003,2005
   Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA. */

#ifndef _REGEX_H
#define _REGEX_H 1

#include <sys/types.h>

/* Allow the use in C++ code.  */
#ifdef __cplusplus
extern "C" {
#endif

/* Define _REGEX_SOURCE to get definitions that are incompatible with
   POSIX.  */
#if (!defined _REGEX_SOURCE					\
     && (defined _GNU_SOURCE					\
	 || (!defined _POSIX_C_SOURCE && !defined _POSIX_SOURCE	\
	     && !defined _XOPEN_SOURCE)))
# define _REGEX_SOURCE 1
#endif

#if defined _REGEX_SOURCE && defined VMS
/* VMS doesn't have `size_t' in <sys/types.h>, even though POSIX says it
   should be there.  */
# include <stddef.h>
#endif

#ifdef _REGEX_LARGE_OFFSETS

/* Use types and values that are wide enough to represent signed and
   unsigned byte offsets in memory.  This currently works only when
   the regex code is used outside of the GNU C library; it is not yet
   supported within glibc itself, and glibc users should not define
   _REGEX_LARGE_OFFSETS.  */

/* The type of the offset of a byte within a string.
   For historical reasons POSIX 1003.1-2004 requires that regoff_t be
   at least as wide as off_t.  This is a bit odd (and many common
   POSIX platforms set it to the more-sensible ssize_t) but we might
   as well conform.  We don't know of any hosts where ssize_t is wider
   than off_t, so off_t is safe.  */
typedef off_t regoff_t;

/* The type of nonnegative object indexes.  Traditionally, GNU regex
   uses 'int' for these.  Code that uses __re_idx_t should work
   regardless of whether the type is signed.  */
typedef size_t __re_idx_t;

/* The type of object sizes.  */
typedef size_t __re_size_t;

/* The type of object sizes, in places where the traditional code
   uses unsigned long int.  */
typedef size_t __re_long_size_t;

#else

/* Use types that are binary-compatible with the traditional GNU regex
   implementation, which mishandles strings longer than INT_MAX.  */

typedef int regoff_t;
typedef int __re_idx_t;
typedef unsigned int __re_size_t;
typedef unsigned long int __re_long_size_t;

#endif

/* The following two types have to be signed and unsigned integer type
   wide enough to hold a value of a pointer.  For most ANSI compilers
   ptrdiff_t and size_t should be likely OK.  Still size of these two
   types is 2 for Microsoft C.  Ugh... */
typedef long int s_reg_t;
typedef unsigned long int active_reg_t;

/* The following bits are used to determine the regexp syntax we
   recognize.  The set/not-set meanings are chosen so that Emacs syntax
   remains the value 0.  The bits are given in alphabetical order, and
   the definitions shifted by one from the previous bit; thus, when we
   add or remove a bit, only one other definition need change.  */
typedef unsigned long int reg_syntax_t;

/* If this bit is not set, then \ inside a bracket expression is literal.
   If set, then such a \ quotes the following character.  */
#define REG_BACKSLASH_ESCAPE_IN_LISTS 1ul

/* If this bit is not set, then + and ? are operators, and \+ and \? are
     literals.
   If set, then \+ and \? are operators and + and ? are literals.  */
#define REG_BK_PLUS_QM (1ul << 1)

/* If this bit is set, then character classes are supported.  They are:
     [:alpha:], [:upper:], [:lower:],  [:digit:], [:alnum:], [:xdigit:],
     [:space:], [:print:], [:punct:], [:graph:], and [:cntrl:].
   If not set, then character classes are not supported.  */
#define REG_CHAR_CLASSES (1ul << 2)

/* If this bit is set, then ^ and $ are always anchors (outside bracket
     expressions, of course).
   If this bit is not set, then it depends:
        ^  is an anchor if it is at the beginning of a regular
           expression or after an open-group or an alternation operator;
        $  is an anchor if it is at the end of a regular expression, or
           before a close-group or an alternation operator.

   This bit could be (re)combined with REG_CONTEXT_INDEP_OPS, because
   POSIX draft 11.2 says that * etc. in leading positions is undefined.
   We already implemented a previous draft which made those constructs
   invalid, though, so we haven't changed the code back.  */
#define REG_CONTEXT_INDEP_ANCHORS (1ul << 3)

/* If this bit is set, then special characters are always special
     regardless of where they are in the pattern.
   If this bit is not set, then special characters are special only in
     some contexts; otherwise they are ordinary.  Specifically,
     * + ? and intervals are only special when not after the beginning,
     open-group, or alternation operator.  */
#define REG_CONTEXT_INDEP_OPS (1ul << 4)

/* If this bit is set, then *, +, ?, and { cannot be first in an re or
     immediately after an alternation or begin-group operator.  */
#define REG_CONTEXT_INVALID_OPS (1ul << 5)

/* If this bit is set, then . matches newline.
   If not set, then it doesn't.  */
#define REG_DOT_NEWLINE (1ul << 6)

/* If this bit is set, then . doesn't match NUL.
   If not set, then it does.  */
#define REG_DOT_NOT_NULL (1ul << 7)

/* If this bit is set, nonmatching lists [^...] do not match newline.
   If not set, they do.  */
#define REG_HAT_LISTS_NOT_NEWLINE (1ul << 8)

/* If this bit is set, either \{...\} or {...} defines an
     interval, depending on REG_NO_BK_BRACES.
   If not set, \{, \}, {, and } are literals.  */
#define REG_INTERVALS (1ul << 9)

/* If this bit is set, +, ? and | aren't recognized as operators.
   If not set, they are.  */
#define REG_LIMITED_OPS (1ul << 10)

/* If this bit is set, newline is an alternation operator.
   If not set, newline is literal.  */
#define REG_NEWLINE_ALT (1ul << 11)

/* If this bit is set, then `{...}' defines an interval, and \{ and \}
     are literals.
  If not set, then `\{...\}' defines an interval.  */
#define REG_NO_BK_BRACES (1ul << 12)

/* If this bit is set, (...) defines a group, and \( and \) are literals.
   If not set, \(...\) defines a group, and ( and ) are literals.  */
#define REG_NO_BK_PARENS (1ul << 13)

/* If this bit is set, then \<digit> matches <digit>.
   If not set, then \<digit> is a back-reference.  */
#define REG_NO_BK_REFS (1ul << 14)

/* If this bit is set, then | is an alternation operator, and \| is literal.
   If not set, then \| is an alternation operator, and | is literal.  */
#define REG_NO_BK_VBAR (1ul << 15)

/* If this bit is set, then an ending range point collating higher
     than the starting range point, as in [z-a], is invalid.
   If not set, the containing range is empty and does not match any string.  */
#define REG_NO_EMPTY_RANGES (1ul << 16)

/* If this bit is set, then an unmatched ) is ordinary.
   If not set, then an unmatched ) is invalid.  */
#define REG_UNMATCHED_RIGHT_PAREN_ORD (1ul << 17)

/* If this bit is set, succeed as soon as we match the whole pattern,
   without further backtracking.  */
#define REG_NO_POSIX_BACKTRACKING (1ul << 18)

/* If this bit is set, do not process the GNU regex operators.
   If not set, then the GNU regex operators are recognized. */
#define REG_NO_GNU_OPS (1ul << 19)

/* If this bit is set, turn on internal regex debugging.
   If not set, and debugging was on, turn it off.
   This only works if regex.c is compiled -DDEBUG.
   We define this bit always, so that all that's needed to turn on
   debugging is to recompile regex.c; the calling code can always have
   this bit set, and it won't affect anything in the normal case. */
#define REG_DEBUG (1ul << 20)

/* If this bit is set, a syntactically invalid interval is treated as
   a string of ordinary characters.  For example, the ERE 'a{1' is
   treated as 'a\{1'.  */
#define REG_INVALID_INTERVAL_ORD (1ul << 21)

/* If this bit is set, then ignore case when matching.
   If not set, then case is significant.  */
#define REG_IGNORE_CASE (1ul << 22)

/* This bit is used internally like REG_CONTEXT_INDEP_ANCHORS but only
   for ^, because it is difficult to scan the regex backwards to find
   whether ^ should be special.  */
#define REG_CARET_ANCHORS_HERE (1ul << 23)

/* If this bit is set, then \{ cannot be first in an bre or
   immediately after an alternation or begin-group operator.  */
#define REG_CONTEXT_INVALID_DUP (1ul << 24)

/* If this bit is set, then no_sub will be set to 1 during
   re_compile_pattern.  */
#define REG_NO_SUB (1ul << 25)

/* This global variable defines the particular regexp syntax to use (for
   some interfaces).  When a regexp is compiled, the syntax used is
   stored in the pattern buffer, so changing this does not affect
   already-compiled regexps.  */
extern reg_syntax_t re_syntax_options;

/* Define combinations of the above bits for the standard possibilities.
   (The [[[ comments delimit what gets put into the Texinfo file, so
   don't delete them!)  */
/* [[[begin syntaxes]]] */
#define REG_SYNTAX_EMACS 0

#define REG_SYNTAX_AWK							\
  (REG_BACKSLASH_ESCAPE_IN_LISTS   | REG_DOT_NOT_NULL			\
   | REG_NO_BK_PARENS		   | REG_NO_BK_REFS			\
   | REG_NO_BK_VBAR		   | REG_NO_EMPTY_RANGES		\
   | REG_DOT_NEWLINE		   | REG_CONTEXT_INDEP_ANCHORS		\
   | REG_UNMATCHED_RIGHT_PAREN_ORD | REG_NO_GNU_OPS)

#define REG_SYNTAX_GNU_AWK						\
  ((REG_SYNTAX_POSIX_EXTENDED | REG_BACKSLASH_ESCAPE_IN_LISTS		\
    | REG_DEBUG)							\
   & ~(REG_DOT_NOT_NULL | REG_INTERVALS | REG_CONTEXT_INDEP_OPS		\
       | REG_CONTEXT_INVALID_OPS ))

#define REG_SYNTAX_POSIX_AWK						\
  (REG_SYNTAX_POSIX_EXTENDED | REG_BACKSLASH_ESCAPE_IN_LISTS		\
   | REG_INTERVALS	     | REG_NO_GNU_OPS)

#define REG_SYNTAX_GREP							\
  (REG_BK_PLUS_QM	       | REG_CHAR_CLASSES			\
   | REG_HAT_LISTS_NOT_NEWLINE | REG_INTERVALS				\
   | REG_NEWLINE_ALT)

#define REG_SYNTAX_EGREP						\
  (REG_CHAR_CLASSES	   | REG_CONTEXT_INDEP_ANCHORS			\
   | REG_CONTEXT_INDEP_OPS | REG_HAT_LISTS_NOT_NEWLINE			\
   | REG_NEWLINE_ALT	   | REG_NO_BK_PARENS				\
   | REG_NO_BK_VBAR)

#define REG_SYNTAX_POSIX_EGREP						\
  (REG_SYNTAX_EGREP | REG_INTERVALS | REG_NO_BK_BRACES			\
   | REG_INVALID_INTERVAL_ORD)

/* P1003.2/D11.2, section 4.20.7.1, lines 5078ff.  */
#define REG_SYNTAX_ED REG_SYNTAX_POSIX_BASIC

#define REG_SYNTAX_SED REG_SYNTAX_POSIX_BASIC

/* Syntax bits common to both basic and extended POSIX regex syntax.  */
#define _REG_SYNTAX_POSIX_COMMON					\
  (REG_CHAR_CLASSES | REG_DOT_NEWLINE	 | REG_DOT_NOT_NULL		\
   | REG_INTERVALS  | REG_NO_EMPTY_RANGES)

#define REG_SYNTAX_POSIX_BASIC						\
  (_REG_SYNTAX_POSIX_COMMON | REG_BK_PLUS_QM | REG_CONTEXT_INVALID_DUP)

/* Differs from ..._POSIX_BASIC only in that REG_BK_PLUS_QM becomes
   REG_LIMITED_OPS, i.e., \? \+ \| are not recognized.  Actually, this
   isn't minimal, since other operators, such as \`, aren't disabled.  */
#define REG_SYNTAX_POSIX_MINIMAL_BASIC					\
  (_REG_SYNTAX_POSIX_COMMON | REG_LIMITED_OPS)

#define REG_SYNTAX_POSIX_EXTENDED					\
  (_REG_SYNTAX_POSIX_COMMON  | REG_CONTEXT_INDEP_ANCHORS		\
   | REG_CONTEXT_INDEP_OPS   | REG_NO_BK_BRACES				\
   | REG_NO_BK_PARENS	     | REG_NO_BK_VBAR				\
   | REG_CONTEXT_INVALID_OPS | REG_UNMATCHED_RIGHT_PAREN_ORD)

/* Differs from ..._POSIX_EXTENDED in that REG_CONTEXT_INDEP_OPS is
   removed and REG_NO_BK_REFS is added.  */
#define REG_SYNTAX_POSIX_MINIMAL_EXTENDED				\
  (_REG_SYNTAX_POSIX_COMMON  | REG_CONTEXT_INDEP_ANCHORS		\
   | REG_CONTEXT_INVALID_OPS | REG_NO_BK_BRACES				\
   | REG_NO_BK_PARENS	     | REG_NO_BK_REFS				\
   | REG_NO_BK_VBAR	     | REG_UNMATCHED_RIGHT_PAREN_ORD)
/* [[[end syntaxes]]] */

/* Maximum number of duplicates an interval can allow.  This is
   distinct from RE_DUP_MAX, to conform to POSIX name space rules and
   to avoid collisions with <limits.h>.  */
#define REG_DUP_MAX 32767


/* POSIX `cflags' bits (i.e., information for `regcomp').  */

/* If this bit is set, then use extended regular expression syntax.
   If not set, then use basic regular expression syntax.  */
#define REG_EXTENDED 1

/* If this bit is set, then ignore case when matching.
   If not set, then case is significant.  */
#define REG_ICASE (1 << 1)

/* If this bit is set, then anchors do not match at newline
     characters in the string.
   If not set, then anchors do match at newlines.  */
#define REG_NEWLINE (1 << 2)

/* If this bit is set, then report only success or fail in regexec.
   If not set, then returns differ between not matching and errors.  */
#define REG_NOSUB (1 << 3)


/* POSIX `eflags' bits (i.e., information for regexec).  */

/* If this bit is set, then the beginning-of-line operator doesn't match
     the beginning of the string (presumably because it's not the
     beginning of a line).
   If not set, then the beginning-of-line operator does match the
     beginning of the string.  */
#define REG_NOTBOL 1

/* Like REG_NOTBOL, except for the end-of-line.  */
#define REG_NOTEOL (1 << 1)

/* Use PMATCH[0] to delimit the start and end of the search in the
   buffer.  */
#define REG_STARTEND (1 << 2)


/* If any error codes are removed, changed, or added, update the
   `__re_error_msgid' table in regcomp.c.  */

typedef enum
{
  _REG_ENOSYS = -1,	/* This will never happen for this implementation.  */
#define REG_ENOSYS	_REG_ENOSYS

  _REG_NOERROR,		/* Success.  */
#define REG_NOERROR	_REG_NOERROR

  _REG_NOMATCH,		/* Didn't find a match (for regexec).  */
#define REG_NOMATCH	_REG_NOMATCH

  /* POSIX regcomp return error codes.  (In the order listed in the
     standard.)  */

  _REG_BADPAT,		/* Invalid pattern.  */
#define REG_BADPAT	_REG_BADPAT

  _REG_ECOLLATE,	/* Inalid collating element.  */
#define REG_ECOLLATE	_REG_ECOLLATE

  _REG_ECTYPE,		/* Invalid character class name.  */
#define REG_ECTYPE	_REG_ECTYPE

  _REG_EESCAPE,		/* Trailing backslash.  */
#define REG_EESCAPE	_REG_EESCAPE

  _REG_ESUBREG,		/* Invalid back reference.  */
#define REG_ESUBREG	_REG_ESUBREG

  _REG_EBRACK,		/* Unmatched left bracket.  */
#define REG_EBRACK	_REG_EBRACK

  _REG_EPAREN,		/* Parenthesis imbalance.  */
#define REG_EPAREN	_REG_EPAREN

  _REG_EBRACE,		/* Unmatched \{.  */
#define REG_EBRACE	_REG_EBRACE

  _REG_BADBR,		/* Invalid contents of \{\}.  */
#define REG_BADBR	_REG_BADBR

  _REG_ERANGE,		/* Invalid range end.  */
#define REG_ERANGE	_REG_ERANGE

  _REG_ESPACE,		/* Ran out of memory.  */
#define REG_ESPACE	_REG_ESPACE

  _REG_BADRPT,		/* No preceding re for repetition op.  */
#define REG_BADRPT	_REG_BADRPT

  /* Error codes we've added.  */

  _REG_EEND,		/* Premature end.  */
#define REG_EEND	_REG_EEND

  _REG_ESIZE,		/* Compiled pattern bigger than 2^16 bytes.  */
#define REG_ESIZE	_REG_ESIZE

  _REG_ERPAREN		/* Unmatched ) or \); not returned from regcomp.  */
#define REG_ERPAREN	_REG_ERPAREN

} reg_errcode_t;

/* In the traditional GNU implementation, regex.h defined member names
   like `buffer' that POSIX does not allow.  These members now have
   names with leading `re_' (e.g., `re_buffer').  Support the old
   names only if _REGEX_SOURCE is defined.  New programs should use
   the new names.  */
#ifdef _REGEX_SOURCE
# define _REG_RE_NAME(id) id
# define _REG_RM_NAME(id) id
#else
# define _REG_RE_NAME(id) re_##id
# define _REG_RM_NAME(id) rm_##id
#endif

/* The user can specify the type of the re_translate member by
   defining the macro REG_TRANSLATE_TYPE.  In the traditional GNU
   implementation, this macro was named RE_TRANSLATE_TYPE, but POSIX
   does not allow this.  Support the old name only if _REGEX_SOURCE
   and if the new name is not defined. New programs should use the new
   name.  */
#ifndef REG_TRANSLATE_TYPE
# if defined _REGEX_SOURCE && defined RE_TRANSLATE_TYPE
#  define REG_TRANSLATE_TYPE RE_TRANSLATE_TYPE
# else
#  define REG_TRANSLATE_TYPE char *
# endif
#endif

/* This data structure represents a compiled pattern.  Before calling
   the pattern compiler), the fields `re_buffer', `re_allocated', `re_fastmap',
   `re_translate', and `re_no_sub' can be set.  After the pattern has been
   compiled, the `re_nsub' field is available.  All other fields are
   private to the regex routines.  */

struct re_pattern_buffer
{
/* [[[begin pattern_buffer]]] */
	/* Space that holds the compiled pattern.  It is declared as
          `unsigned char *' because its elements are
           sometimes used as array indexes.  */
  unsigned char *_REG_RE_NAME (buffer);

	/* Number of bytes to which `re_buffer' points.  */
  __re_long_size_t _REG_RE_NAME (allocated);

	/* Number of bytes actually used in `re_buffer'.  */
  __re_long_size_t _REG_RE_NAME (used);

        /* Syntax setting with which the pattern was compiled.  */
  reg_syntax_t _REG_RE_NAME (syntax);

        /* Pointer to a fastmap, if any, otherwise zero.  re_search uses
           the fastmap, if there is one, to skip over impossible
           starting points for matches.  */
  char *_REG_RE_NAME (fastmap);

        /* Either a translate table to apply to all characters before
           comparing them, or zero for no translation.  The translation
           is applied to a pattern when it is compiled and to a string
           when it is matched.  */
  REG_TRANSLATE_TYPE _REG_RE_NAME (translate);

	/* Number of subexpressions found by the compiler.  */
  size_t re_nsub;

        /* Zero if this pattern cannot match the empty string, one else.
           Well, in truth it's used only in `re_search_2', to see
           whether or not we should use the fastmap, so we don't set
           this absolutely perfectly; see `re_compile_fastmap' (the
           `duplicate' case).  */
  unsigned int _REG_RE_NAME (can_be_null) : 1;

        /* If REG_UNALLOCATED, allocate space in the `regs' structure
             for `max (REG_NREGS, re_nsub + 1)' groups.
           If REG_REALLOCATE, reallocate space if necessary.
           If REG_FIXED, use what's there.  */
#define REG_UNALLOCATED 0
#define REG_REALLOCATE 1
#define REG_FIXED 2
  unsigned int _REG_RE_NAME (regs_allocated) : 2;

        /* Set to zero when `regex_compile' compiles a pattern; set to one
           by `re_compile_fastmap' if it updates the fastmap.  */
  unsigned int _REG_RE_NAME (fastmap_accurate) : 1;

        /* If set, `re_match_2' does not return information about
           subexpressions.  */
  unsigned int _REG_RE_NAME (no_sub) : 1;

        /* If set, a beginning-of-line anchor doesn't match at the
           beginning of the string.  */
  unsigned int _REG_RE_NAME (not_bol) : 1;

        /* Similarly for an end-of-line anchor.  */
  unsigned int _REG_RE_NAME (not_eol) : 1;

        /* If true, an anchor at a newline matches.  */
  unsigned int _REG_RE_NAME (newline_anchor) : 1;

/* [[[end pattern_buffer]]] */
};

typedef struct re_pattern_buffer regex_t;

/* This is the structure we store register match data in.  See
   regex.texinfo for a full description of what registers match.  */
struct re_registers
{
  __re_size_t _REG_RM_NAME (num_regs);
  regoff_t *_REG_RM_NAME (start);
  regoff_t *_REG_RM_NAME (end);
};


/* If `regs_allocated' is REG_UNALLOCATED in the pattern buffer,
   `re_match_2' returns information about at least this many registers
   the first time a `regs' structure is passed.  */
#ifndef REG_NREGS
# define REG_NREGS 30
#endif


/* POSIX specification for registers.  Aside from the different names than
   `re_registers', POSIX uses an array of structures, instead of a
   structure of arrays.  */
typedef struct
{
  regoff_t rm_so;  /* Byte offset from string's start to substring's start.  */
  regoff_t rm_eo;  /* Byte offset from string's start to substring's end.  */
} regmatch_t;

/* Declarations for routines.  */

/* Sets the current default syntax to SYNTAX, and return the old syntax.
   You can also simply assign to the `re_syntax_options' variable.  */
extern reg_syntax_t re_set_syntax (reg_syntax_t __syntax);

/* Compile the regular expression PATTERN, with length LENGTH
   and syntax given by the global `re_syntax_options', into the buffer
   BUFFER.  Return NULL if successful, and an error string if not.  */
extern const char *re_compile_pattern (const char *__pattern, size_t __length,
				       struct re_pattern_buffer *__buffer);


/* Compile a fastmap for the compiled pattern in BUFFER; used to
   accelerate searches.  Return 0 if successful and -2 if was an
   internal error.  */
extern int re_compile_fastmap (struct re_pattern_buffer *__buffer);


/* Search in the string STRING (with length LENGTH) for the pattern
   compiled into BUFFER.  Start searching at position START, for RANGE
   characters.  Return the starting position of the match, -1 for no
   match, or -2 for an internal error.  Also return register
   information in REGS (if REGS and BUFFER->re_no_sub are nonzero).  */
extern regoff_t re_search (struct re_pattern_buffer *__buffer,
			   const char *__string, __re_idx_t __length,
			   __re_idx_t __start, regoff_t __range,
			   struct re_registers *__regs);


/* Like `re_search', but search in the concatenation of STRING1 and
   STRING2.  Also, stop searching at index START + STOP.  */
extern regoff_t re_search_2 (struct re_pattern_buffer *__buffer,
			     const char *__string1, __re_idx_t __length1,
			     const char *__string2, __re_idx_t __length2,
			     __re_idx_t __start, regoff_t __range,
			     struct re_registers *__regs,
			     __re_idx_t __stop);


/* Like `re_search', but return how many characters in STRING the regexp
   in BUFFER matched, starting at position START.  */
extern regoff_t re_match (struct re_pattern_buffer *__buffer,
			  const char *__string, __re_idx_t __length,
			  __re_idx_t __start, struct re_registers *__regs);


/* Relates to `re_match' as `re_search_2' relates to `re_search'.  */
extern regoff_t re_match_2 (struct re_pattern_buffer *__buffer,
			    const char *__string1, __re_idx_t __length1,
			    const char *__string2, __re_idx_t __length2,
			    __re_idx_t __start, struct re_registers *__regs,
			    __re_idx_t __stop);


/* Set REGS to hold NUM_REGS registers, storing them in STARTS and
   ENDS.  Subsequent matches using BUFFER and REGS will use this memory
   for recording register information.  STARTS and ENDS must be
   allocated with malloc, and must each be at least `NUM_REGS * sizeof
   (regoff_t)' bytes long.

   If NUM_REGS == 0, then subsequent matches should allocate their own
   register data.

   Unless this function is called, the first search or match using
   PATTERN_BUFFER will allocate its own register data, without
   freeing the old data.  */
extern void re_set_registers (struct re_pattern_buffer *__buffer,
			      struct re_registers *__regs,
			      __re_size_t __num_regs,
			      regoff_t *__starts, regoff_t *__ends);

#if defined _REGEX_RE_COMP || defined _LIBC
# ifndef _CRAY
/* 4.2 bsd compatibility.  */
extern char *re_comp (const char *);
extern int re_exec (const char *);
# endif
#endif

/* GCC 2.95 and later have "__restrict"; C99 compilers have
   "restrict", and "configure" may have defined "restrict".  */
#ifndef __restrict
# if ! (2 < __GNUC__ || (2 == __GNUC__ && 95 <= __GNUC_MINOR__))
#  if defined restrict || 199901L <= __STDC_VERSION__
#   define __restrict restrict
#  else
#   define __restrict
#  endif
# endif
#endif
/* gcc 3.1 and up support the [restrict] syntax, but g++ doesn't.  */
#ifndef __restrict_arr
# if (__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 1)) && !defined __cplusplus
#  define __restrict_arr __restrict
# else
#  define __restrict_arr
# endif
#endif

/* POSIX compatibility.  */
extern int regcomp (regex_t *__restrict __preg,
		    const char *__restrict __pattern,
		    int __cflags);

extern int regexec (const regex_t *__restrict __preg,
		    const char *__restrict __string, size_t __nmatch,
		    regmatch_t __pmatch[__restrict_arr],
		    int __eflags);

extern size_t regerror (int __errcode, const regex_t *__restrict __preg,
			char *__restrict __errbuf, size_t __errbuf_size);

extern void regfree (regex_t *__preg);


#ifdef _REGEX_SOURCE

/* Define the POSIX-compatible member names in terms of the
   incompatible (and deprecated) names established by _REG_RE_NAME.
   New programs should use the re_* names.  */

# define re_allocated allocated
# define re_buffer buffer
# define re_can_be_null can_be_null
# define re_fastmap fastmap
# define re_fastmap_accurate fastmap_accurate
# define re_newline_anchor newline_anchor
# define re_no_sub no_sub
# define re_not_bol not_bol
# define re_not_eol not_eol
# define re_regs_allocated regs_allocated
# define re_syntax syntax
# define re_translate translate
# define re_used used

/* Similarly for _REG_RM_NAME.  */

# define rm_end end
# define rm_num_regs num_regs
# define rm_start start

/* Undef RE_DUP_MAX first, in case the user has already included a
   <limits.h> with an incompatible definition.

   On GNU systems, the most common spelling for RE_DUP_MAX's value in
   <limits.h> is (0x7ffff), so define RE_DUP_MAX to that, not to
   REG_DUP_MAX.  This avoid some duplicate-macro-definition warnings
   with programs that include <limits.h> after this file.

   New programs should not assume that regex.h defines RE_DUP_MAX; to
   get the value of RE_DUP_MAX, they should instead include <limits.h>
   and possibly invoke the sysconf function.  */

# undef RE_DUP_MAX
# define RE_DUP_MAX (0x7fff)

/* Define the following symbols for backward source compatibility.
   These symbols violate the POSIX name space rules, and new programs
   should avoid them.  */

# define REGS_FIXED REG_FIXED
# define REGS_REALLOCATE REG_REALLOCATE
# define REGS_UNALLOCATED REG_UNALLOCATED
# define RE_BACKSLASH_ESCAPE_IN_LISTS REG_BACKSLASH_ESCAPE_IN_LISTS
# define RE_BK_PLUS_QM REG_BK_PLUS_QM
# define RE_CARET_ANCHORS_HERE REG_CARET_ANCHORS_HERE
# define RE_CHAR_CLASSES REG_CHAR_CLASSES
# define RE_CONTEXT_INDEP_ANCHORS REG_CONTEXT_INDEP_ANCHORS
# define RE_CONTEXT_INDEP_OPS REG_CONTEXT_INDEP_OPS
# define RE_CONTEXT_INVALID_DUP REG_CONTEXT_INVALID_DUP
# define RE_CONTEXT_INVALID_OPS REG_CONTEXT_INVALID_OPS
# define RE_DEBUG REG_DEBUG
# define RE_DOT_NEWLINE REG_DOT_NEWLINE
# define RE_DOT_NOT_NULL REG_DOT_NOT_NULL
# define RE_HAT_LISTS_NOT_NEWLINE REG_HAT_LISTS_NOT_NEWLINE
# define RE_ICASE REG_IGNORE_CASE /* avoid collision with REG_ICASE */
# define RE_INTERVALS REG_INTERVALS
# define RE_INVALID_INTERVAL_ORD REG_INVALID_INTERVAL_ORD
# define RE_LIMITED_OPS REG_LIMITED_OPS
# define RE_NEWLINE_ALT REG_NEWLINE_ALT
# define RE_NO_BK_BRACES REG_NO_BK_BRACES
# define RE_NO_BK_PARENS REG_NO_BK_PARENS
# define RE_NO_BK_REFS REG_NO_BK_REFS
# define RE_NO_BK_VBAR REG_NO_BK_VBAR
# define RE_NO_EMPTY_RANGES REG_NO_EMPTY_RANGES
# define RE_NO_GNU_OPS REG_NO_GNU_OPS
# define RE_NO_POSIX_BACKTRACKING REG_NO_POSIX_BACKTRACKING
# define RE_NO_SUB REG_NO_SUB
# define RE_NREGS REG_NREGS
# define RE_SYNTAX_AWK REG_SYNTAX_AWK
# define RE_SYNTAX_ED REG_SYNTAX_ED
# define RE_SYNTAX_EGREP REG_SYNTAX_EGREP
# define RE_SYNTAX_EMACS REG_SYNTAX_EMACS
# define RE_SYNTAX_GNU_AWK REG_SYNTAX_GNU_AWK
# define RE_SYNTAX_GREP REG_SYNTAX_GREP
# define RE_SYNTAX_POSIX_AWK REG_SYNTAX_POSIX_AWK
# define RE_SYNTAX_POSIX_BASIC REG_SYNTAX_POSIX_BASIC
# define RE_SYNTAX_POSIX_EGREP REG_SYNTAX_POSIX_EGREP
# define RE_SYNTAX_POSIX_EXTENDED REG_SYNTAX_POSIX_EXTENDED
# define RE_SYNTAX_POSIX_MINIMAL_BASIC REG_SYNTAX_POSIX_MINIMAL_BASIC
# define RE_SYNTAX_POSIX_MINIMAL_EXTENDED REG_SYNTAX_POSIX_MINIMAL_EXTENDED
# define RE_SYNTAX_SED REG_SYNTAX_SED
# define RE_UNMATCHED_RIGHT_PAREN_ORD REG_UNMATCHED_RIGHT_PAREN_ORD
# ifndef RE_TRANSLATE_TYPE
#  define RE_TRANSLATE_TYPE REG_TRANSLATE_TYPE
# endif

#endif /* defined _REGEX_SOURCE */

#ifdef __cplusplus
}
#endif	/* C++ */

#endif /* regex.h */

/*
Local variables:
make-backup-files: t
version-control: t
trim-versions-without-asking: nil
End:
*/
