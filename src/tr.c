/* tr -- a filter to translate characters
   Copyright (C) 91, 1995-2002 Free Software Foundation, Inc.

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
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

/* Written by Jim Meyering */

#include <config.h>

#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <sys/types.h>
#include <getopt.h>

#include "system.h"
#include "closeout.h"
#include "error.h"
#include "safe-read.h"
#include "xstrtol.h"

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "tr"

#define AUTHORS "Jim Meyering"

#define N_CHARS (UCHAR_MAX + 1)

/* A pointer to a filtering function.  */
typedef size_t (*Filter) (/* unsigned char *, size_t, Filter */);

/* Convert from character C to its index in the collating
   sequence array.  Just cast to an unsigned int to avoid
   problems with sign-extension.  */
#define ORD(c) (unsigned int)(c)

/* The inverse of ORD.  */
#define CHR(i) (unsigned char)(i)

/* The value for Spec_list->state that indicates to
   get_next that it should initialize the tail pointer.
   Its value should be as large as possible to avoid conflict
   a valid value for the state field -- and that may be as
   large as any valid repeat_count.  */
#define BEGIN_STATE (INT_MAX - 1)

/* The value for Spec_list->state that indicates to
   get_next that the element pointed to by Spec_list->tail is
   being considered for the first time on this pass through the
   list -- it indicates that get_next should make any necessary
   initializations.  */
#define NEW_ELEMENT (BEGIN_STATE + 1)

/* A value distinct from any character that may have been stored in a
   buffer as the result of a block-read in the function squeeze_filter.  */
#define NOT_A_CHAR (unsigned int)(-1)

/* The following (but not CC_NO_CLASS) are indices into the array of
   valid character class strings.  */
enum Char_class
  {
    CC_ALNUM = 0, CC_ALPHA = 1, CC_BLANK = 2, CC_CNTRL = 3,
    CC_DIGIT = 4, CC_GRAPH = 5, CC_LOWER = 6, CC_PRINT = 7,
    CC_PUNCT = 8, CC_SPACE = 9, CC_UPPER = 10, CC_XDIGIT = 11,
    CC_NO_CLASS = 9999
  };

/* Character class to which a character (returned by get_next) belonged;
   but it is set only if the construct from which the character was obtained
   was one of the character classes [:upper:] or [:lower:].  The value
   is used only when translating and then, only to make sure that upper
   and lower class constructs have the same relative positions in string1
   and string2.  */
enum Upper_Lower_class
  {
    UL_LOWER = 0,
    UL_UPPER = 1,
    UL_NONE = 2
  };

/* A shortcut to ensure that when constructing the translation array,
   one of the values returned by paired calls to get_next (from s1 and s2)
   is from [:upper:] and the other is from [:lower:], or neither is from
   upper or lower.  By default, GNU tr permits the identity mappings: from
   [:upper:] to [:upper:] and [:lower:] to [:lower:].  But when
   POSIXLY_CORRECT is set, those evoke diagnostics. This array is indexed
   by values of type enum Upper_Lower_class.  */
static int const class_ok[3][3] =
{
  {1, 1, 0},
  {1, 1, 0},
  {0, 0, 1}
};

/* The type of a List_element.  See build_spec_list for more details.  */
enum Range_element_type
  {
    RE_NO_TYPE = 0,
    RE_NORMAL_CHAR,
    RE_RANGE,
    RE_CHAR_CLASS,
    RE_EQUIV_CLASS,
    RE_REPEATED_CHAR
  };

/* One construct in one of tr's argument strings.
   For example, consider the POSIX version of the classic tr command:
       tr -cs 'a-zA-Z_' '[\n*]'
   String1 has 3 constructs, two of which are ranges (a-z and A-Z),
   and a single normal character, `_'.  String2 has one construct.  */
struct List_element
  {
    enum Range_element_type type;
    struct List_element *next;
    union
      {
	int normal_char;
	struct			/* unnamed */
	  {
	    unsigned int first_char;
	    unsigned int last_char;
	  }
	range;
	enum Char_class char_class;
	int equiv_code;
	struct			/* unnamed */
	  {
	    unsigned int the_repeated_char;
	    size_t repeat_count;
	  }
	repeated_char;
      }
    u;
  };

/* Each of tr's argument strings is parsed into a form that is easier
   to work with: a linked list of constructs (struct List_element).
   Each Spec_list structure also encapsulates various attributes of
   the corresponding argument string.  The attributes are used mainly
   to verify that the strings are valid in the context of any options
   specified (like -s, -d, or -c).  The main exception is the member
   `tail', which is first used to construct the list.  After construction,
   it is used by get_next to save its state when traversing the list.
   The member `state' serves a similar function.  */
struct Spec_list
  {
    /* Points to the head of the list of range elements.
       The first struct is a dummy; its members are never used.  */
    struct List_element *head;

    /* When appending, points to the last element.  When traversing via
       get_next(), points to the element to process next.  Setting
       Spec_list.state to the value BEGIN_STATE before calling get_next
       signals get_next to initialize tail to point to head->next.  */
    struct List_element *tail;

    /* Used to save state between calls to get_next().  */
    unsigned int state;

    /* Length, in the sense that length ('a-z[:digit:]123abc')
       is 42 ( = 26 + 10 + 6).  */
    size_t length;

    /* The number of [c*] and [c*0] constructs that appear in this spec.  */
    int n_indefinite_repeats;

    /* If n_indefinite_repeats is nonzero, this points to the List_element
       corresponding to the last [c*] or [c*0] construct encountered in
       this spec.  Otherwise it is undefined.  */
    struct List_element *indefinite_repeat_element;

    /* Non-zero if this spec contains at least one equivalence
       class construct e.g. [=c=].  */
    int has_equiv_class;

    /* Non-zero if this spec contains at least one character class
       construct.  E.g. [:digit:].  */
    int has_char_class;

    /* Non-zero if this spec contains at least one of the character class
       constructs (all but upper and lower) that aren't allowed in s2.  */
    int has_restricted_char_class;
  };

/* A representation for escaped string1 or string2.  As a string is parsed,
   any backslash-escaped characters (other than octal or \a, \b, \f, \n,
   etc.) are marked as such in this structure by setting the corresponding
   entry in the ESCAPED vector.  */
struct E_string
{
  unsigned char *s;
  int *escaped;
  size_t len;
};

/* Return nonzero if the Ith character of escaped string ES matches C
   and is not escaped itself.  */
#define ES_MATCH(ES, I, C) ((ES)->s[(I)] == (C) && !(ES)->escaped[(I)])

/* The name by which this program was run.  */
char *program_name;

/* When nonzero, each sequence in the input of a repeated character
   (call it c) is replaced (in the output) by a single occurrence of c
   for every c in the squeeze set.  */
static int squeeze_repeats = 0;

/* When nonzero, removes characters in the delete set from input.  */
static int delete = 0;

/* Use the complement of set1 in place of set1.  */
static int complement = 0;

/* When nonzero, this flag causes GNU tr to provide strict
   compliance with POSIX draft 1003.2.11.2.  The POSIX spec
   says that when -d is used without -s, string2 (if present)
   must be ignored.  Silently ignoring arguments is a bad idea.
   The default GNU behavior is to give a usage message and exit.
   Additionally, when this flag is nonzero, tr prints warnings
   on stderr if it is being used in a manner that is not portable.
   Applicable warnings are given by default, but are suppressed
   if the environment variable `POSIXLY_CORRECT' is set, since
   being POSIX conformant means we can't issue such messages.
   Warnings on the following topics are suppressed when this
   variable is nonzero:
   1. Ambiguous octal escapes.  */
static int posix_pedantic;

/* When tr is performing translation and string1 is longer than string2,
   POSIX says that the result is undefined.  That gives the implementor
   of a POSIX conforming version of tr two reasonable choices for the
   semantics of this case.

   * The BSD tr pads string2 to the length of string1 by
   repeating the last character in string2.

   * System V tr ignores characters in string1 that have no
   corresponding character in string2.  That is, string1 is effectively
   truncated to the length of string2.

   When nonzero, this flag causes GNU tr to imitate the behavior
   of System V tr when translating with string1 longer than string2.
   The default is to emulate BSD tr.  This flag is ignored in modes where
   no translation is performed.  Emulating the System V tr
   in this exceptional case causes the relatively common BSD idiom:

       tr -cs A-Za-z0-9 '\012'

   to break (it would convert only zero bytes, rather than all
   non-alphanumerics, to newlines).

   WARNING: This switch does not provide general BSD or System V
   compatibility.  For example, it doesn't disable the interpretation
   of the POSIX constructs [:alpha:], [=c=], and [c*10], so if by
   some unfortunate coincidence you use such constructs in scripts
   expecting to use some other version of tr, the scripts will break.  */
static int truncate_set1 = 0;

/* An alias for (!delete && non_option_args == 2).
   It is set in main and used there and in validate().  */
static int translating;

#ifndef BUFSIZ
# define BUFSIZ 8192
#endif

#define IO_BUF_SIZE BUFSIZ
static unsigned char io_buf[IO_BUF_SIZE];

static char const *const char_class_name[] =
{
  "alnum", "alpha", "blank", "cntrl", "digit", "graph",
  "lower", "print", "punct", "space", "upper", "xdigit"
};
#define N_CHAR_CLASSES (sizeof(char_class_name) / sizeof(char_class_name[0]))

typedef char SET_TYPE;

/* Array of boolean values.  A character `c' is a member of the
   squeeze set if and only if in_squeeze_set[c] is true.  The squeeze
   set is defined by the last (possibly, the only) string argument
   on the command line when the squeeze option is given.  */
static SET_TYPE in_squeeze_set[N_CHARS];

/* Array of boolean values.  A character `c' is a member of the
   delete set if and only if in_delete_set[c] is true.  The delete
   set is defined by the first (or only) string argument on the
   command line when the delete option is given.  */
static SET_TYPE in_delete_set[N_CHARS];

/* Array of character values defining the translation (if any) that
   tr is to perform.  Translation is performed only when there are
   two specification strings and the delete switch is not given.  */
static char xlate[N_CHARS];

static struct option const long_options[] =
{
  {"complement", no_argument, NULL, 'c'},
  {"delete", no_argument, NULL, 'd'},
  {"squeeze-repeats", no_argument, NULL, 's'},
  {"truncate-set1", no_argument, NULL, 't'},
  {GETOPT_HELP_OPTION_DECL},
  {GETOPT_VERSION_OPTION_DECL},
  {NULL, 0, NULL, 0}
};

void
usage (int status)
{
  if (status != 0)
    fprintf (stderr, _("Try `%s --help' for more information.\n"),
	     program_name);
  else
    {
      printf (_("\
Usage: %s [OPTION]... SET1 [SET2]\n\
"),
	      program_name);
      fputs (_("\
Translate, squeeze, and/or delete characters from standard input,\n\
writing to standard output.\n\
\n\
  -c, --complement        first complement SET1\n\
  -d, --delete            delete characters in SET1, do not translate\n\
  -s, --squeeze-repeats   replace each input sequence of a repeated character\n\
                            that is listed in SET1 with a single occurrence\n\
                            of that character\n\
  -t, --truncate-set1     first truncate SET1 to length of SET2\n\
"), stdout);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      fputs (_("\
\n\
SETs are specified as strings of characters.  Most represent themselves.\n\
Interpreted sequences are:\n\
\n\
  \\NNN            character with octal value NNN (1 to 3 octal digits)\n\
  \\\\              backslash\n\
  \\a              audible BEL\n\
  \\b              backspace\n\
  \\f              form feed\n\
  \\n              new line\n\
  \\r              return\n\
  \\t              horizontal tab\n\
"), stdout);
     fputs (_("\
  \\v              vertical tab\n\
  CHAR1-CHAR2     all characters from CHAR1 to CHAR2 in ascending order\n\
  [CHAR*]         in SET2, copies of CHAR until length of SET1\n\
  [CHAR*REPEAT]   REPEAT copies of CHAR, REPEAT octal if starting with 0\n\
  [:alnum:]       all letters and digits\n\
  [:alpha:]       all letters\n\
  [:blank:]       all horizontal whitespace\n\
  [:cntrl:]       all control characters\n\
  [:digit:]       all digits\n\
"), stdout);
     fputs (_("\
  [:graph:]       all printable characters, not including space\n\
  [:lower:]       all lower case letters\n\
  [:print:]       all printable characters, including space\n\
  [:punct:]       all punctuation characters\n\
  [:space:]       all horizontal or vertical whitespace\n\
  [:upper:]       all upper case letters\n\
  [:xdigit:]      all hexadecimal digits\n\
  [=CHAR=]        all characters which are equivalent to CHAR\n\
"), stdout);
     fputs (_("\
\n\
Translation occurs if -d is not given and both SET1 and SET2 appear.\n\
-t may be used only when translating.  SET2 is extended to length of\n\
SET1 by repeating its last character as necessary.  \
"), stdout);
     fputs (_("\
Excess characters\n\
of SET2 are ignored.  Only [:lower:] and [:upper:] are guaranteed to\n\
expand in ascending order; used in SET2 while translating, they may\n\
only be used in pairs to specify case conversion.  \
"), stdout);
     fputs (_("\
-s uses SET1 if not\n\
translating nor deleting; else squeezing uses SET2 and occurs after\n\
translation or deletion.\n\
"), stdout);
      printf (_("\nReport bugs to <%s>.\n"), PACKAGE_BUGREPORT);
    }
  exit (status == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}

/* Return nonzero if the character C is a member of the
   equivalence class containing the character EQUIV_CLASS.  */

static int
is_equiv_class_member (unsigned int equiv_class, unsigned int c)
{
  return (equiv_class == c);
}

/* Return nonzero if the character C is a member of the
   character class CHAR_CLASS.  */

static int
is_char_class_member (enum Char_class char_class, unsigned int c)
{
  int result;

  switch (char_class)
    {
    case CC_ALNUM:
      result = ISALNUM (c);
      break;
    case CC_ALPHA:
      result = ISALPHA (c);
      break;
    case CC_BLANK:
      result = ISBLANK (c);
      break;
    case CC_CNTRL:
      result = ISCNTRL (c);
      break;
    case CC_DIGIT:
      result = ISDIGIT_LOCALE (c);
      break;
    case CC_GRAPH:
      result = ISGRAPH (c);
      break;
    case CC_LOWER:
      result = ISLOWER (c);
      break;
    case CC_PRINT:
      result = ISPRINT (c);
      break;
    case CC_PUNCT:
      result = ISPUNCT (c);
      break;
    case CC_SPACE:
      result = ISSPACE (c);
      break;
    case CC_UPPER:
      result = ISUPPER (c);
      break;
    case CC_XDIGIT:
      result = ISXDIGIT (c);
      break;
    default:
      abort ();
      break;
    }
  return result;
}

static void
es_free (struct E_string *es)
{
  free (es->s);
  free (es->escaped);
}

/* Perform the first pass over each range-spec argument S, converting all
   \c and \ddd escapes to their one-byte representations.  The conversion
   is done in-place, so S must point to writable storage.  If an invalid
   quote sequence is found print an error message and return nonzero.
   Otherwise set *LEN to the length of the resulting string and return
   zero.  The resulting array of characters may contain zero-bytes;
   however, on input, S is assumed to be null-terminated, and hence
   cannot contain actual (non-escaped) zero bytes.  */

static int
unquote (const unsigned char *s, struct E_string *es)
{
  size_t i, j;
  size_t len;

  len = strlen ((char *) s);

  es->s = (unsigned char *) xmalloc (len);
  es->escaped = (int *) xmalloc (len * sizeof (es->escaped[0]));
  for (i = 0; i < len; i++)
    es->escaped[i] = 0;

  j = 0;
  for (i = 0; s[i]; i++)
    {
      switch (s[i])
	{
	  int c;
	case '\\':
	  switch (s[i + 1])
	    {
	      int oct_digit;
	    case '\\':
	      c = '\\';
	      break;
	    case 'a':
	      c = '\007';
	      break;
	    case 'b':
	      c = '\b';
	      break;
	    case 'f':
	      c = '\f';
	      break;
	    case 'n':
	      c = '\n';
	      break;
	    case 'r':
	      c = '\r';
	      break;
	    case 't':
	      c = '\t';
	      break;
	    case 'v':
	      c = '\v';
	      break;
	    case '0':
	    case '1':
	    case '2':
	    case '3':
	    case '4':
	    case '5':
	    case '6':
	    case '7':
	      c = s[i + 1] - '0';
	      oct_digit = s[i + 2] - '0';
	      if (0 <= oct_digit && oct_digit <= 7)
		{
		  c = 8 * c + oct_digit;
		  ++i;
		  oct_digit = s[i + 2] - '0';
		  if (0 <= oct_digit && oct_digit <= 7)
		    {
		      if (8 * c + oct_digit < N_CHARS)
			{
			  c = 8 * c + oct_digit;
			  ++i;
			}
		      else if (!posix_pedantic)
			{
			  /* A 3-digit octal number larger than \377 won't
			     fit in 8 bits.  So we stop when adding the
			     next digit would put us over the limit and
			     give a warning about the ambiguity.  POSIX
			     isn't clear on this, but one person has said
			     that in his interpretation, POSIX says tr
			     can't even give a warning.  */
			  error (0, 0, _("warning: the ambiguous octal escape \
\\%c%c%c is being\n\tinterpreted as the 2-byte sequence \\0%c%c, `%c'"),
				 s[i], s[i + 1], s[i + 2],
				 s[i], s[i + 1], s[i + 2]);
			}
		    }
		}
	      break;
	    case '\0':
	      error (0, 0, _("invalid backslash escape at end of string"));
	      return 1;

	    default:
	      if (posix_pedantic)
		{
		  error (0, 0, _("invalid backslash escape `\\%c'"), s[i + 1]);
		  return 1;
		}
	      else
	        {
		  c = s[i + 1];
		  es->escaped[j] = 1;
		}
	    }
	  ++i;
	  es->s[j++] = c;
	  break;
	default:
	  es->s[j++] = s[i];
	  break;
	}
    }
  es->len = j;
  return 0;
}

/* If CLASS_STR is a valid character class string, return its index
   in the global char_class_name array.  Otherwise, return CC_NO_CLASS.  */

static enum Char_class
look_up_char_class (const unsigned char *class_str, size_t len)
{
  unsigned int i;

  for (i = 0; i < N_CHAR_CLASSES; i++)
    if (strncmp ((const char *) class_str, char_class_name[i], len) == 0
	&& strlen (char_class_name[i]) == len)
      return (enum Char_class) i;
  return CC_NO_CLASS;
}

/* Return a newly allocated string with a printable version of C.
   This function is used solely for formatting error messages.  */

static char *
make_printable_char (unsigned int c)
{
  char *buf = xmalloc (5);

  assert (c < N_CHARS);
  if (ISPRINT (c))
    {
      buf[0] = c;
      buf[1] = '\0';
    }
  else
    {
      sprintf (buf, "\\%03o", c);
    }
  return buf;
}

/* Return a newly allocated copy of S which is suitable for printing.
   LEN is the number of characters in S.  Most non-printing
   (isprint) characters are represented by a backslash followed by
   3 octal digits.  However, the characters represented by \c escapes
   where c is one of [abfnrtv] are represented by their 2-character \c
   sequences.  This function is used solely for printing error messages.  */

static char *
make_printable_str (const unsigned char *s, size_t len)
{
  /* Worst case is that every character expands to a backslash
     followed by a 3-character octal escape sequence.  */
  char *printable_buf = xmalloc (4 * len + 1);
  char *p = printable_buf;
  size_t i;

  for (i = 0; i < len; i++)
    {
      char buf[5];
      char *tmp = NULL;

      switch (s[i])
	{
	case '\\':
	  tmp = "\\";
	  break;
	case '\007':
	  tmp = "\\a";
	  break;
	case '\b':
	  tmp = "\\b";
	  break;
	case '\f':
	  tmp = "\\f";
	  break;
	case '\n':
	  tmp = "\\n";
	  break;
	case '\r':
	  tmp = "\\r";
	  break;
	case '\t':
	  tmp = "\\t";
	  break;
	case '\v':
	  tmp = "\\v";
	  break;
	default:
	  if (ISPRINT (s[i]))
	    {
	      buf[0] = s[i];
	      buf[1] = '\0';
	    }
	  else
	    sprintf (buf, "\\%03o", s[i]);
	  tmp = buf;
	  break;
	}
      p = stpcpy (p, tmp);
    }
  return printable_buf;
}

/* Append a newly allocated structure representing a
   character C to the specification list LIST.  */

static void
append_normal_char (struct Spec_list *list, unsigned int c)
{
  struct List_element *new;

  new = (struct List_element *) xmalloc (sizeof (struct List_element));
  new->next = NULL;
  new->type = RE_NORMAL_CHAR;
  new->u.normal_char = c;
  assert (list->tail);
  list->tail->next = new;
  list->tail = new;
}

/* Append a newly allocated structure representing the range
   of characters from FIRST to LAST to the specification list LIST.
   Return nonzero if LAST precedes FIRST in the collating sequence,
   zero otherwise.  This means that '[c-c]' is acceptable.  */

static int
append_range (struct Spec_list *list, unsigned int first, unsigned int last)
{
  struct List_element *new;

  if (ORD (first) > ORD (last))
    {
      char *tmp1 = make_printable_char (first);
      char *tmp2 = make_printable_char (last);

      error (0, 0,
       _("range-endpoints of `%s-%s' are in reverse collating sequence order"),
	     tmp1, tmp2);
      free (tmp1);
      free (tmp2);
      return 1;
    }
  new = (struct List_element *) xmalloc (sizeof (struct List_element));
  new->next = NULL;
  new->type = RE_RANGE;
  new->u.range.first_char = first;
  new->u.range.last_char = last;
  assert (list->tail);
  list->tail->next = new;
  list->tail = new;
  return 0;
}

/* If CHAR_CLASS_STR is a valid character class string, append a
   newly allocated structure representing that character class to the end
   of the specification list LIST and return 0.  If CHAR_CLASS_STR is not
   a valid string return nonzero.  */

static int
append_char_class (struct Spec_list *list,
		   const unsigned char *char_class_str, size_t len)
{
  enum Char_class char_class;
  struct List_element *new;

  char_class = look_up_char_class (char_class_str, len);
  if (char_class == CC_NO_CLASS)
    return 1;
  new = (struct List_element *) xmalloc (sizeof (struct List_element));
  new->next = NULL;
  new->type = RE_CHAR_CLASS;
  new->u.char_class = char_class;
  assert (list->tail);
  list->tail->next = new;
  list->tail = new;
  return 0;
}

/* Append a newly allocated structure representing a [c*n]
   repeated character construct to the specification list LIST.
   THE_CHAR is the single character to be repeated, and REPEAT_COUNT
   is a non-negative repeat count.  */

static void
append_repeated_char (struct Spec_list *list, unsigned int the_char,
		      size_t repeat_count)
{
  struct List_element *new;

  new = (struct List_element *) xmalloc (sizeof (struct List_element));
  new->next = NULL;
  new->type = RE_REPEATED_CHAR;
  new->u.repeated_char.the_repeated_char = the_char;
  new->u.repeated_char.repeat_count = repeat_count;
  assert (list->tail);
  list->tail->next = new;
  list->tail = new;
}

/* Given a string, EQUIV_CLASS_STR, from a [=str=] context and
   the length of that string, LEN, if LEN is exactly one, append
   a newly allocated structure representing the specified
   equivalence class to the specification list, LIST and return zero.
   If LEN is not 1, return nonzero.  */

static int
append_equiv_class (struct Spec_list *list,
		    const unsigned char *equiv_class_str, size_t len)
{
  struct List_element *new;

  if (len != 1)
    return 1;
  new = (struct List_element *) xmalloc (sizeof (struct List_element));
  new->next = NULL;
  new->type = RE_EQUIV_CLASS;
  new->u.equiv_code = *equiv_class_str;
  assert (list->tail);
  list->tail->next = new;
  list->tail = new;
  return 0;
}

/* Return a newly allocated copy of the LEN-byte prefix of P.
   The returned string may contain NUL bytes and is *not* NUL-terminated.  */

static unsigned char *
xmemdup (const unsigned char *p, size_t len)
{
  unsigned char *tmp = (unsigned char *) xmalloc (len);

  /* Use memcpy rather than strncpy because `p' may contain zero-bytes.  */
  memcpy (tmp, p, len);
  return tmp;
}

/* Search forward starting at START_IDX for the 2-char sequence
   (PRE_BRACKET_CHAR,']') in the string P of length P_LEN.  If such
   a sequence is found, set *RESULT_IDX to the index of the first
   character and return nonzero. Otherwise return zero.  P may contain
   zero bytes.  */

static int
find_closing_delim (const struct E_string *es, size_t start_idx,
		    unsigned int pre_bracket_char, size_t *result_idx)
{
  size_t i;

  for (i = start_idx; i < es->len - 1; i++)
    if (es->s[i] == pre_bracket_char && es->s[i + 1] == ']'
	&& !es->escaped[i] && !es->escaped[i + 1])
      {
	*result_idx = i;
	return 1;
      }
  return 0;
}

/* Parse the bracketed repeat-char syntax.  If the P_LEN characters
   beginning with P[ START_IDX ] comprise a valid [c*n] construct,
   then set *CHAR_TO_REPEAT, *REPEAT_COUNT, and *CLOSING_BRACKET_IDX
   and return zero. If the second character following
   the opening bracket is not `*' or if no closing bracket can be
   found, return -1.  If a closing bracket is found and the
   second char is `*', but the string between the `*' and `]' isn't
   empty, an octal number, or a decimal number, print an error message
   and return -2.  */

static int
find_bracketed_repeat (const struct E_string *es, size_t start_idx,
		       unsigned int *char_to_repeat, size_t *repeat_count,
		       size_t *closing_bracket_idx)
{
  size_t i;

  assert (start_idx + 1 < es->len);
  if (!ES_MATCH (es, start_idx + 1, '*'))
    return -1;

  for (i = start_idx + 2; i < es->len; i++)
    {
      if (ES_MATCH (es, i, ']'))
	{
	  size_t digit_str_len = i - start_idx - 2;

	  *char_to_repeat = es->s[start_idx];
	  if (digit_str_len == 0)
	    {
	      /* We've matched [c*] -- no explicit repeat count.  */
	      *repeat_count = 0;
	      *closing_bracket_idx = i;
	      return 0;
	    }

	  /* Here, we have found [c*s] where s should be a string
	     of octal (if it starts with `0') or decimal digits.  */
	  {
	    const char *digit_str = (const char *) &es->s[start_idx + 2];
	    unsigned long int tmp_ulong;
	    char *d_end;
	    int base = 10;
	    /* Select the base manually so we can be sure it's either 8 or 10.
	       If the spec allowed it to be interpreted as hexadecimal, we
	       could have used `0' and let xstrtoul decide.  */
	    if (*digit_str == '0')
	      {
		base = 8;
		++digit_str;
		--digit_str_len;
	      }
	    if (xstrtoul (digit_str, &d_end, base, &tmp_ulong, NULL)
		  != LONGINT_OK
		|| BEGIN_STATE < tmp_ulong
		|| digit_str + digit_str_len != d_end)
	      {
		char *tmp = make_printable_str (es->s + start_idx + 2,
						i - start_idx - 2);
		error (0, 0, _("invalid repeat count `%s' in [c*n] construct"),
		       tmp);
		free (tmp);
		return -2;
	      }
	    *repeat_count = tmp_ulong;
	  }
	  *closing_bracket_idx = i;
	  return 0;
	}
    }
  return -1;			/* No bracket found.  */
}

/* Return nonzero if the string at ES->s[IDX] matches the regular
   expression `\*[0-9]*\]', zero otherwise.  To match, the `*' and
   the `]' must not be escaped.  */

static int
star_digits_closebracket (const struct E_string *es, size_t idx)
{
  size_t i;

  if (!ES_MATCH (es, idx, '*'))
    return 0;

  for (i = idx + 1; i < es->len; i++)
    {
      if (!ISDIGIT (es->s[i]))
	{
	  if (ES_MATCH (es, i, ']'))
	    return 1;
	  return 0;
	}
    }
  return 0;
}

/* Convert string UNESCAPED_STRING (which has been preprocessed to
   convert backslash-escape sequences) of length LEN characters into
   a linked list of the following 5 types of constructs:
      - [:str:] Character class where `str' is one of the 12 valid strings.
      - [=c=] Equivalence class where `c' is any single character.
      - [c*n] Repeat the single character `c' `n' times. n may be omitted.
	  However, if `n' is present, it must be a non-negative octal or
	  decimal integer.
      - r-s Range of characters from `r' to `s'.  The second endpoint must
	  not precede the first in the current collating sequence.
      - c Any other character is interpreted as itself.  */

static int
build_spec_list (const struct E_string *es, struct Spec_list *result)
{
  const unsigned char *p;
  size_t i;

  p = es->s;

  /* The main for-loop below recognizes the 4 multi-character constructs.
     A character that matches (in its context) none of the multi-character
     constructs is classified as `normal'.  Since all multi-character
     constructs have at least 3 characters, any strings of length 2 or
     less are composed solely of normal characters.  Hence, the index of
     the outer for-loop runs only as far as LEN-2.  */

  for (i = 0; i + 2 < es->len; /* empty */)
    {
      if (ES_MATCH (es, i, '['))
	{
	  int matched_multi_char_construct;
	  size_t closing_bracket_idx;
	  unsigned int char_to_repeat;
	  size_t repeat_count;
	  int err;

	  matched_multi_char_construct = 1;
	  if (ES_MATCH (es, i + 1, ':')
	      || ES_MATCH (es, i + 1, '='))
	    {
	      size_t closing_delim_idx;
	      int found;

	      found = find_closing_delim (es, i + 2, p[i + 1],
					  &closing_delim_idx);
	      if (found)
		{
		  int parse_failed;
		  size_t opnd_str_len = closing_delim_idx - 1 - (i + 2) + 1;
		  unsigned char *opnd_str;

		  if (opnd_str_len == 0)
		    {
		      if (p[i + 1] == ':')
			error (0, 0, _("missing character class name `[::]'"));
		      else
			error (0, 0,
			       _("missing equivalence class character `[==]'"));
		      return 1;
		    }

		  opnd_str = xmemdup (p + i + 2, opnd_str_len);

		  if (p[i + 1] == ':')
		    {
		      parse_failed = append_char_class (result, opnd_str,
							opnd_str_len);

		      /* FIXME: big comment.  */
		      if (parse_failed)
			{
			  if (star_digits_closebracket (es, i + 2))
			    {
			      free (opnd_str);
			      goto try_bracketed_repeat;
			    }
			  else
			    {
			      char *tmp = make_printable_str (opnd_str,
							      opnd_str_len);
			      error (0, 0, _("invalid character class `%s'"),
				     tmp);
			      free (tmp);
			      return 1;
			    }
			}
		    }
		  else
		    {
		      parse_failed = append_equiv_class (result, opnd_str,
							 opnd_str_len);

		      /* FIXME: big comment.  */
		      if (parse_failed)
			{
			  if (star_digits_closebracket (es, i + 2))
			    {
			      free (opnd_str);
			      goto try_bracketed_repeat;
			    }
			  else
			    {
			      char *tmp = make_printable_str (opnd_str,
							      opnd_str_len);
			      error (0, 0,
	       _("%s: equivalence class operand must be a single character"),
				     tmp);
			      free (tmp);
			      return 1;
			    }
			}
		    }
		  free (opnd_str);

		  /* Return nonzero if append_*_class reports a problem.  */
		  if (parse_failed)
		    return 1;
		  else
		    i = closing_delim_idx + 2;
		  continue;
		}
	      /* Else fall through.  This could be [:*] or [=*].  */
	    }

	try_bracketed_repeat:

	  /* Determine whether this is a bracketed repeat range
	     matching the RE \[.\*(dec_or_oct_number)?\].  */
	  err = find_bracketed_repeat (es, i + 1, &char_to_repeat,
				       &repeat_count,
				       &closing_bracket_idx);
	  if (err == 0)
	    {
	      append_repeated_char (result, char_to_repeat, repeat_count);
	      i = closing_bracket_idx + 1;
	    }
	  else if (err == -1)
	    {
	      matched_multi_char_construct = 0;
	    }
	  else
	    {
	      /* Found a string that looked like [c*n] but the
		 numeric part was invalid.  */
	      return 1;
	    }

	  if (matched_multi_char_construct)
	    continue;

	  /* We reach this point if P does not match [:str:], [=c=],
	     [c*n], or [c*].  Now, see if P looks like a range `[-c'
	     (from `[' to `c').  */
	}

      /* Look ahead one char for ranges like a-z.  */
      if (ES_MATCH (es, i + 1, '-'))
	{
	  if (append_range (result, p[i], p[i + 2]))
	    return 1;
	  i += 3;
	}
      else
	{
	  append_normal_char (result, p[i]);
	  ++i;
	}
    }

  /* Now handle the (2 or fewer) remaining characters p[i]..p[es->len - 1].  */
  for (; i < es->len; i++)
    append_normal_char (result, p[i]);

  return 0;
}

/* Given a Spec_list S (with its saved state implicit in the values
   of its members `tail' and `state'), return the next single character
   in the expansion of S's constructs.  If the last character of S was
   returned on the previous call or if S was empty, this function
   returns -1.  For example, successive calls to get_next where S
   represents the spec-string 'a-d[y*3]' will return the sequence
   of values a, b, c, d, y, y, y, -1.  Finally, if the construct from
   which the returned character comes is [:upper:] or [:lower:], the
   parameter CLASS is given a value to indicate which it was.  Otherwise
   CLASS is set to UL_NONE.  This value is used only when constructing
   the translation table to verify that any occurrences of upper and
   lower class constructs in the spec-strings appear in the same relative
   positions.  */

static int
get_next (struct Spec_list *s, enum Upper_Lower_class *class)
{
  struct List_element *p;
  int return_val;
  int i;

  if (class)
    *class = UL_NONE;

  if (s->state == BEGIN_STATE)
    {
      s->tail = s->head->next;
      s->state = NEW_ELEMENT;
    }

  p = s->tail;
  if (p == NULL)
    return -1;

  switch (p->type)
    {
    case RE_NORMAL_CHAR:
      return_val = p->u.normal_char;
      s->state = NEW_ELEMENT;
      s->tail = p->next;
      break;

    case RE_RANGE:
      if (s->state == NEW_ELEMENT)
	s->state = ORD (p->u.range.first_char);
      else
	++(s->state);
      return_val = CHR (s->state);
      if (s->state == ORD (p->u.range.last_char))
	{
	  s->tail = p->next;
	  s->state = NEW_ELEMENT;
	}
      break;

    case RE_CHAR_CLASS:
      if (class)
	{
	  int upper_or_lower;
	  switch (p->u.char_class)
	    {
	    case CC_LOWER:
	      *class = UL_LOWER;
	      upper_or_lower = 1;
	      break;
	    case CC_UPPER:
	      *class = UL_UPPER;
	      upper_or_lower = 1;
	      break;
	    default:
	      upper_or_lower = 0;
	      break;
	    }

	  if (upper_or_lower)
	    {
	      s->tail = p->next;
	      s->state = NEW_ELEMENT;
	      return_val = 0;
	      break;
	    }
	}

      if (s->state == NEW_ELEMENT)
	{
	  for (i = 0; i < N_CHARS; i++)
	    if (is_char_class_member (p->u.char_class, i))
	      break;
	  assert (i < N_CHARS);
	  s->state = i;
	}
      assert (is_char_class_member (p->u.char_class, s->state));
      return_val = CHR (s->state);
      for (i = s->state + 1; i < N_CHARS; i++)
	if (is_char_class_member (p->u.char_class, i))
	  break;
      if (i < N_CHARS)
	s->state = i;
      else
	{
	  s->tail = p->next;
	  s->state = NEW_ELEMENT;
	}
      break;

    case RE_EQUIV_CLASS:
      /* FIXME: this assumes that each character is alone in its own
         equivalence class (which appears to be correct for my
         LC_COLLATE.  But I don't know of any function that allows
         one to determine a character's equivalence class.  */

      return_val = p->u.equiv_code;
      s->state = NEW_ELEMENT;
      s->tail = p->next;
      break;

    case RE_REPEATED_CHAR:
      /* Here, a repeat count of n == 0 means don't repeat at all.  */
      if (p->u.repeated_char.repeat_count == 0)
	{
	  s->tail = p->next;
	  s->state = NEW_ELEMENT;
	  return_val = get_next (s, class);
	}
      else
	{
	  if (s->state == NEW_ELEMENT)
	    {
	      s->state = 0;
	    }
	  ++(s->state);
	  return_val = p->u.repeated_char.the_repeated_char;
	  if (p->u.repeated_char.repeat_count > 0
	      && s->state == p->u.repeated_char.repeat_count)
	    {
	      s->tail = p->next;
	      s->state = NEW_ELEMENT;
	    }
	}
      break;

    case RE_NO_TYPE:
      abort ();
      break;

    default:
      abort ();
      break;
    }

  return return_val;
}

/* This is a minor kludge.  This function is called from
   get_spec_stats to determine the cardinality of a set derived
   from a complemented string.  It's a kludge in that some of the
   same operations are (duplicated) performed in set_initialize.  */

static int
card_of_complement (struct Spec_list *s)
{
  int c;
  int cardinality = N_CHARS;
  SET_TYPE in_set[N_CHARS];

  memset (in_set, 0, N_CHARS * sizeof (in_set[0]));
  s->state = BEGIN_STATE;
  while ((c = get_next (s, NULL)) != -1)
    if (!in_set[c]++)
      --cardinality;
  return cardinality;
}

/* Gather statistics about the spec-list S in preparation for the tests
   in validate that determine the consistency of the specs.  This function
   is called at most twice; once for string1, and again for any string2.
   LEN_S1 < 0 indicates that this is the first call and that S represents
   string1.  When LEN_S1 >= 0, it is the length of the expansion of the
   constructs in string1, and we can use its value to resolve any
   indefinite repeat construct in S (which represents string2).  Hence,
   this function has the side-effect that it converts a valid [c*]
   construct in string2 to [c*n] where n is large enough (or 0) to give
   string2 the same length as string1.  For example, with the command
   tr a-z 'A[\n*]Z' on the second call to get_spec_stats, LEN_S1 would
   be 26 and S (representing string2) would be converted to 'A[\n*24]Z'.  */

static void
get_spec_stats (struct Spec_list *s)
{
  struct List_element *p;
  int len = 0;

  s->n_indefinite_repeats = 0;
  s->has_equiv_class = 0;
  s->has_restricted_char_class = 0;
  s->has_char_class = 0;
  for (p = s->head->next; p; p = p->next)
    {
      switch (p->type)
	{
	  int i;
	case RE_NORMAL_CHAR:
	  ++len;
	  break;

	case RE_RANGE:
	  assert (p->u.range.last_char >= p->u.range.first_char);
	  len += p->u.range.last_char - p->u.range.first_char + 1;
	  break;

	case RE_CHAR_CLASS:
	  s->has_char_class = 1;
	  for (i = 0; i < N_CHARS; i++)
	    if (is_char_class_member (p->u.char_class, i))
	      ++len;
	  switch (p->u.char_class)
	    {
	    case CC_UPPER:
	    case CC_LOWER:
	      break;
	    default:
	      s->has_restricted_char_class = 1;
	      break;
	    }
	  break;

	case RE_EQUIV_CLASS:
	  for (i = 0; i < N_CHARS; i++)
	    if (is_equiv_class_member (p->u.equiv_code, i))
	      ++len;
	  s->has_equiv_class = 1;
	  break;

	case RE_REPEATED_CHAR:
	  if (p->u.repeated_char.repeat_count > 0)
	    len += p->u.repeated_char.repeat_count;
	  else if (p->u.repeated_char.repeat_count == 0)
	    {
	      s->indefinite_repeat_element = p;
	      ++(s->n_indefinite_repeats);
	    }
	  break;

	case RE_NO_TYPE:
	  assert (0);
	  break;
	}
    }

  s->length = len;
}

static void
get_s1_spec_stats (struct Spec_list *s1)
{
  get_spec_stats (s1);
  if (complement)
    s1->length = card_of_complement (s1);
}

static void
get_s2_spec_stats (struct Spec_list *s2, size_t len_s1)
{
  get_spec_stats (s2);
  if (len_s1 >= s2->length && s2->n_indefinite_repeats == 1)
    {
      s2->indefinite_repeat_element->u.repeated_char.repeat_count =
	len_s1 - s2->length;
      s2->length = len_s1;
    }
}

static void
spec_init (struct Spec_list *spec_list)
{
  spec_list->head = spec_list->tail =
    (struct List_element *) xmalloc (sizeof (struct List_element));
  spec_list->head->next = NULL;
}

/* This function makes two passes over the argument string S.  The first
   one converts all \c and \ddd escapes to their one-byte representations.
   The second constructs a linked specification list, SPEC_LIST, of the
   characters and constructs that comprise the argument string.  If either
   of these passes detects an error, this function returns nonzero.  */

static int
parse_str (const unsigned char *s, struct Spec_list *spec_list)
{
  struct E_string es;
  int fail;

  fail = unquote (s, &es);
  if (!fail)
    fail = build_spec_list (&es, spec_list);
  es_free (&es);
  return fail;
}

/* Given two specification lists, S1 and S2, and assuming that
   S1->length > S2->length, append a single [c*n] element to S2 where c
   is the last character in the expansion of S2 and n is the difference
   between the two lengths.
   Upon successful completion, S2->length is set to S1->length.  The only
   way this function can fail to make S2 as long as S1 is when S2 has
   zero-length, since in that case, there is no last character to repeat.
   So S2->length is required to be at least 1.

   Providing this functionality allows the user to do some pretty
   non-BSD (and non-portable) things:  For example, the command
       tr -cs '[:upper:]0-9' '[:lower:]'
   is almost guaranteed to give results that depend on your collating
   sequence.  */

static void
string2_extend (const struct Spec_list *s1, struct Spec_list *s2)
{
  struct List_element *p;
  int char_to_repeat;
  int i;

  assert (translating);
  assert (s1->length > s2->length);
  assert (s2->length > 0);

  p = s2->tail;
  switch (p->type)
    {
    case RE_NORMAL_CHAR:
      char_to_repeat = p->u.normal_char;
      break;
    case RE_RANGE:
      char_to_repeat = p->u.range.last_char;
      break;
    case RE_CHAR_CLASS:
      for (i = N_CHARS; i >= 0; i--)
	if (is_char_class_member (p->u.char_class, i))
	  break;
      assert (i >= 0);
      char_to_repeat = CHR (i);
      break;

    case RE_REPEATED_CHAR:
      char_to_repeat = p->u.repeated_char.the_repeated_char;
      break;

    case RE_EQUIV_CLASS:
      /* This shouldn't happen, because validate exits with an error
         if it finds an equiv class in string2 when translating.  */
      abort ();
      break;

    case RE_NO_TYPE:
      abort ();
      break;

    default:
      abort ();
      break;
    }

  append_repeated_char (s2, char_to_repeat, s1->length - s2->length);
  s2->length = s1->length;
}

/* Return non-zero if S is a non-empty list in which exactly one
   character (but potentially, many instances of it) appears.
   E.g.  [X*] or xxxxxxxx.  */

static int
homogeneous_spec_list (struct Spec_list *s)
{
  int b, c;

  s->state = BEGIN_STATE;

  if ((b = get_next (s, NULL)) == -1)
    return 0;

  while ((c = get_next (s, NULL)) != -1)
    if (c != b)
      return 0;

  return 1;
}

/* Die with an error message if S1 and S2 describe strings that
   are not valid with the given command line switches.
   A side effect of this function is that if a valid [c*] or
   [c*0] construct appears in string2, it is converted to [c*n]
   with a value for n that makes s2->length == s1->length.  By
   the same token, if the --truncate-set1 option is not
   given, S2 may be extended.  */

static void
validate (struct Spec_list *s1, struct Spec_list *s2)
{
  get_s1_spec_stats (s1);
  if (s1->n_indefinite_repeats > 0)
    {
      error (EXIT_FAILURE, 0,
	     _("the [c*] repeat construct may not appear in string1"));
    }

  if (s2)
    {
      get_s2_spec_stats (s2, s1->length);

      if (s2->n_indefinite_repeats > 1)
	{
	  error (EXIT_FAILURE, 0,
		 _("only one [c*] repeat construct may appear in string2"));
	}

      if (translating)
	{
	  if (s2->has_equiv_class)
	    {
	      error (EXIT_FAILURE, 0,
		     _("[=c=] expressions may not appear in string2 \
when translating"));
	    }

	  if (s1->length > s2->length)
	    {
	      if (!truncate_set1)
		{
		  /* string2 must be non-empty unless --truncate-set1 is
		     given or string1 is empty.  */

		  if (s2->length == 0)
		    error (EXIT_FAILURE, 0,
		     _("when not truncating set1, string2 must be non-empty"));
		  string2_extend (s1, s2);
		}
	    }

	  if (complement && s1->has_char_class
	      && ! (s2->length == s1->length && homogeneous_spec_list (s2)))
	    {
	      error (EXIT_FAILURE, 0,
		     _("when translating with complemented character classes,\
\nstring2 must map all characters in the domain to one"));
	    }

	  if (s2->has_restricted_char_class)
	    {
	      error (EXIT_FAILURE, 0,
		     _("when translating, the only character classes that may \
appear in\nstring2 are `upper' and `lower'"));
	    }
	}
      else
	/* Not translating.  */
	{
	  if (s2->n_indefinite_repeats > 0)
	    error (EXIT_FAILURE, 0,
		   _("the [c*] construct may appear in string2 only \
when translating"));
	}
    }
}

/* Read buffers of SIZE bytes via the function READER (if READER is
   NULL, read from stdin) until EOF.  When non-NULL, READER is either
   read_and_delete or read_and_xlate.  After each buffer is read, it is
   processed and written to stdout.  The buffers are processed so that
   multiple consecutive occurrences of the same character in the input
   stream are replaced by a single occurrence of that character if the
   character is in the squeeze set.  */

static void
squeeze_filter (unsigned char *buf, size_t size, Filter reader)
{
  unsigned int char_to_squeeze = NOT_A_CHAR;
  size_t i = 0;
  size_t nr = 0;

  for (;;)
    {
      size_t begin;

      if (i >= nr)
	{
	  if (reader == NULL)
	    {
	      nr = safe_read (0, (char *) buf, size);
	      if (nr == SAFE_READ_ERROR)
		error (EXIT_FAILURE, errno, _("read error"));
	    }
	  else
	    {
	      nr = (*reader) (buf, size, NULL);
	    }

	  if (nr == 0)
	    break;
	  i = 0;
	}

      begin = i;

      if (char_to_squeeze == NOT_A_CHAR)
	{
	  size_t out_len;
	  /* Here, by being a little tricky, we can get a significant
	     performance increase in most cases when the input is
	     reasonably large.  Since tr will modify the input only
	     if two consecutive (and identical) input characters are
	     in the squeeze set, we can step by two through the data
	     when searching for a character in the squeeze set.  This
	     means there may be a little more work in a few cases and
	     perhaps twice as much work in the worst cases where most
	     of the input is removed by squeezing repeats.  But most
	     uses of this functionality seem to remove less than 20-30%
	     of the input.  */
	  for (; i < nr && !in_squeeze_set[buf[i]]; i += 2)
	    ;			/* empty */

	  /* There is a special case when i == nr and we've just
	     skipped a character (the last one in buf) that is in
	     the squeeze set.  */
	  if (i == nr && in_squeeze_set[buf[i - 1]])
	    --i;

	  if (i >= nr)
	    out_len = nr - begin;
	  else
	    {
	      char_to_squeeze = buf[i];
	      /* We're about to output buf[begin..i].  */
	      out_len = i - begin + 1;

	      /* But since we stepped by 2 in the loop above,
	         out_len may be one too large.  */
	      if (i > 0 && buf[i - 1] == char_to_squeeze)
		--out_len;

	      /* Advance i to the index of first character to be
	         considered when looking for a char different from
	         char_to_squeeze.  */
	      ++i;
	    }
	  if (out_len > 0
	      && fwrite ((char *) &buf[begin], 1, out_len, stdout) == 0)
	    error (EXIT_FAILURE, errno, _("write error"));
	}

      if (char_to_squeeze != NOT_A_CHAR)
	{
	  /* Advance i to index of first char != char_to_squeeze
	     (or to nr if all the rest of the characters in this
	     buffer are the same as char_to_squeeze).  */
	  for (; i < nr && buf[i] == char_to_squeeze; i++)
	    ;			/* empty */
	  if (i < nr)
	    char_to_squeeze = NOT_A_CHAR;
	  /* If (i >= nr) we've squeezed the last character in this buffer.
	     So now we have to read a new buffer and continue comparing
	     characters against char_to_squeeze.  */
	}
    }
}

/* Read buffers of SIZE bytes from stdin until one is found that
   contains at least one character not in the delete set.  Store
   in the array BUF, all characters from that buffer that are not
   in the delete set, and return the number of characters saved
   or 0 upon EOF.  */

static size_t
read_and_delete (unsigned char *buf, size_t size, Filter not_used)
{
  size_t n_saved;
  static int hit_eof = 0;

  assert (not_used == NULL);

  if (hit_eof)
    return 0;

  /* This enclosing do-while loop is to make sure that
     we don't return zero (indicating EOF) when we've
     just deleted all the characters in a buffer.  */
  do
    {
      size_t i;
      size_t nr = safe_read (0, (char *) buf, size);

      if (nr == SAFE_READ_ERROR)
	error (EXIT_FAILURE, errno, _("read error"));
      if (nr == 0)
	{
	  hit_eof = 1;
	  return 0;
	}

      /* This first loop may be a waste of code, but gives much
         better performance when no characters are deleted in
         the beginning of a buffer.  It just avoids the copying
         of buf[i] into buf[n_saved] when it would be a NOP.  */

      for (i = 0; i < nr && !in_delete_set[buf[i]]; i++)
	/* empty */ ;
      n_saved = i;

      for (++i; i < nr; i++)
	if (!in_delete_set[buf[i]])
	  buf[n_saved++] = buf[i];
    }
  while (n_saved == 0);

  return n_saved;
}

/* Read at most SIZE bytes from stdin into the array BUF.  Then
   perform the in-place and one-to-one mapping specified by the global
   array `xlate'.  Return the number of characters read, or 0 upon EOF.  */

static size_t
read_and_xlate (unsigned char *buf, size_t size, Filter not_used)
{
  size_t bytes_read = 0;
  static int hit_eof = 0;
  size_t i;

  assert (not_used == NULL);

  if (hit_eof)
    return 0;

  bytes_read = safe_read (0, (char *) buf, size);
  if (bytes_read == SAFE_READ_ERROR)
    error (EXIT_FAILURE, errno, _("read error"));
  if (bytes_read == 0)
    {
      hit_eof = 1;
      return 0;
    }

  for (i = 0; i < bytes_read; i++)
    buf[i] = xlate[buf[i]];

  return bytes_read;
}

/* Initialize a boolean membership set IN_SET with the character
   values obtained by traversing the linked list of constructs S
   using the function `get_next'.  If COMPLEMENT_THIS_SET is
   nonzero the resulting set is complemented.  */

static void
set_initialize (struct Spec_list *s, int complement_this_set, SET_TYPE *in_set)
{
  int c;
  size_t i;

  memset (in_set, 0, N_CHARS * sizeof (in_set[0]));
  s->state = BEGIN_STATE;
  while ((c = get_next (s, NULL)) != -1)
    in_set[c] = 1;
  if (complement_this_set)
    for (i = 0; i < N_CHARS; i++)
      in_set[i] = (!in_set[i]);
}

int
main (int argc, char **argv)
{
  int c;
  int non_option_args;
  struct Spec_list buf1, buf2;
  struct Spec_list *s1 = &buf1;
  struct Spec_list *s2 = &buf2;

  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  while ((c = getopt_long (argc, argv, "cdst", long_options, NULL)) != -1)
    {
      switch (c)
	{
	case 0:
	  break;

	case 'c':
	  complement = 1;
	  break;

	case 'd':
	  delete = 1;
	  break;

	case 's':
	  squeeze_repeats = 1;
	  break;

	case 't':
	  truncate_set1 = 1;
	  break;

	case_GETOPT_HELP_CHAR;

	case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);

	default:
	  usage (2);
	  break;
	}
    }

  posix_pedantic = (getenv ("POSIXLY_CORRECT") != NULL);

  non_option_args = argc - optind;
  translating = (non_option_args == 2 && !delete);

  /* Change this test if it is valid to give tr no options and
     no args at all.  POSIX doesn't specifically say anything
     either way, but it looks like they implied it's invalid
     by omission.  If you want to make tr do a slow imitation
     of `cat' use `tr a a'.  */
  if (non_option_args > 2)
    {
      error (0, 0, _("too many arguments"));
      usage (2);
    }

  if (!delete && !squeeze_repeats && non_option_args != 2)
    error (EXIT_FAILURE, 0, _("two strings must be given when translating"));

  if (delete && squeeze_repeats && non_option_args != 2)
    error (EXIT_FAILURE, 0, _("two strings must be given when both \
deleting and squeezing repeats"));

  /* If --delete is given without --squeeze-repeats, then
     only one string argument may be specified.  But POSIX
     says to ignore any string2 in this case, so if POSIXLY_CORRECT
     is set, pretend we never saw string2.  But I think
     this deserves a fatal error, so that's the default.  */
  if ((delete && !squeeze_repeats) && non_option_args != 1)
    {
      if (posix_pedantic && non_option_args == 2)
	--non_option_args;
      else
	error (EXIT_FAILURE, 0,
	       _("only one string may be given when deleting \
without squeezing repeats"));
    }

  if (squeeze_repeats && non_option_args == 0)
    error (EXIT_FAILURE, 0,
	   _("at least one string must be given when squeezing repeats"));

  spec_init (s1);
  if (parse_str ((unsigned char *) argv[optind], s1))
    exit (EXIT_FAILURE);

  if (non_option_args == 2)
    {
      spec_init (s2);
      if (parse_str ((unsigned char *) argv[optind + 1], s2))
	exit (EXIT_FAILURE);
    }
  else
    s2 = NULL;

  validate (s1, s2);

  /* Use binary I/O, since `tr' is sometimes used to transliterate
     non-printable characters, or characters which are stripped away
     by text-mode reads (like CR and ^Z).  */
  SET_BINARY2 (STDIN_FILENO, STDOUT_FILENO);

  if (squeeze_repeats && non_option_args == 1)
    {
      set_initialize (s1, complement, in_squeeze_set);
      squeeze_filter (io_buf, IO_BUF_SIZE, NULL);
    }
  else if (delete && non_option_args == 1)
    {
      size_t nr;

      set_initialize (s1, complement, in_delete_set);
      do
	{
	  nr = read_and_delete (io_buf, IO_BUF_SIZE, NULL);
	  if (nr > 0 && fwrite ((char *) io_buf, 1, nr, stdout) == 0)
	    error (EXIT_FAILURE, errno, _("write error"));
	}
      while (nr > 0);
    }
  else if (squeeze_repeats && delete && non_option_args == 2)
    {
      set_initialize (s1, complement, in_delete_set);
      set_initialize (s2, 0, in_squeeze_set);
      squeeze_filter (io_buf, IO_BUF_SIZE, read_and_delete);
    }
  else if (translating)
    {
      if (complement)
	{
	  int i;
	  SET_TYPE *in_s1 = in_delete_set;

	  set_initialize (s1, 0, in_s1);
	  s2->state = BEGIN_STATE;
	  for (i = 0; i < N_CHARS; i++)
	    xlate[i] = i;
	  for (i = 0; i < N_CHARS; i++)
	    {
	      if (!in_s1[i])
		{
		  int ch = get_next (s2, NULL);
		  assert (ch != -1 || truncate_set1);
		  if (ch == -1)
		    {
		      /* This will happen when tr is invoked like e.g.
		         tr -cs A-Za-z0-9 '\012'.  */
		      break;
		    }
		  xlate[i] = ch;
		}
	    }
	  assert (get_next (s2, NULL) == -1 || truncate_set1);
	}
      else
	{
	  int c1, c2;
	  int i;
	  enum Upper_Lower_class class_s1;
	  enum Upper_Lower_class class_s2;

	  for (i = 0; i < N_CHARS; i++)
	    xlate[i] = i;
	  s1->state = BEGIN_STATE;
	  s2->state = BEGIN_STATE;
	  for (;;)
	    {
	      c1 = get_next (s1, &class_s1);
	      c2 = get_next (s2, &class_s2);
	      if (!class_ok[(int) class_s1][(int) class_s2])
		error (EXIT_FAILURE, 0,
		       _("misaligned [:upper:] and/or [:lower:] construct"));

	      if (class_s1 == UL_LOWER && class_s2 == UL_UPPER)
		{
		  for (i = 0; i < N_CHARS; i++)
		    if (ISLOWER (i))
		      xlate[i] = toupper (i);
		}
	      else if (class_s1 == UL_UPPER && class_s2 == UL_LOWER)
		{
		  for (i = 0; i < N_CHARS; i++)
		    if (ISUPPER (i))
		      xlate[i] = tolower (i);
		}
	      else if ((class_s1 == UL_LOWER && class_s2 == UL_LOWER)
		       || (class_s1 == UL_UPPER && class_s2 == UL_UPPER))
		{
		  /* By default, GNU tr permits the identity mappings: from
		     [:upper:] to [:upper:] and [:lower:] to [:lower:].  But
		     when POSIXLY_CORRECT is set, those evoke diagnostics.  */
		  if (posix_pedantic)
		    {
		      error (EXIT_FAILURE, 0,
			     _("\
invalid identity mapping;  when translating, any [:lower:] or [:upper:]\n\
construct in string1 must be aligned with a corresponding construct\n\
([:upper:] or [:lower:], respectively) in string2"));
		    }
		}
	      else
		{
		  /* The following should have been checked by validate...  */
		  if (c1 == -1 || c2 == -1)
		    break;
		  xlate[c1] = c2;
		}
	    }
	  assert (c1 == -1 || truncate_set1);
	}
      if (squeeze_repeats)
	{
	  set_initialize (s2, 0, in_squeeze_set);
	  squeeze_filter (io_buf, IO_BUF_SIZE, read_and_xlate);
	}
      else
	{
	  size_t bytes_read;

	  do
	    {
	      bytes_read = read_and_xlate (io_buf, IO_BUF_SIZE, NULL);
	      if (bytes_read > 0
		  && fwrite ((char *) io_buf, 1, bytes_read, stdout) == 0)
		error (EXIT_FAILURE, errno, _("write error"));
	    }
	  while (bytes_read > 0);
	}
    }

  if (close (STDIN_FILENO) != 0)
    error (EXIT_FAILURE, errno, _("standard input"));

  exit (EXIT_SUCCESS);
}
