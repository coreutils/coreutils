/* Permuted index for GNU, with keywords in their context.
   Copyright (C) 1990-2016 Free Software Foundation, Inc.
   François Pinard <pinard@iro.umontreal.ca>, 1988.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   François Pinard <pinard@iro.umontreal.ca> */

#include <config.h>

#include <getopt.h>
#include <sys/types.h>
#include "system.h"
#include <regex.h>
#include "argmatch.h"
#include "diacrit.h"
#include "error.h"
#include "fadvise.h"
#include "quote.h"
#include "read-file.h"
#include "stdio--.h"
#include "xstrtol.h"

/* The official name of this program (e.g., no 'g' prefix).  */
#define PROGRAM_NAME "ptx"

/* TRANSLATORS: Please translate "F. Pinard" to "François Pinard"
   if "ç" (c-with-cedilla) is available in the translation's character
   set and encoding.  */
#define AUTHORS proper_name_utf8 ("F. Pinard", "Fran\xc3\xa7ois Pinard")

/* Number of possible characters in a byte.  */
#define CHAR_SET_SIZE 256

#define ISODIGIT(C) ((C) >= '0' && (C) <= '7')
#define HEXTOBIN(C) ((C) >= 'a' && (C) <= 'f' ? (C)-'a'+10 \
                     : (C) >= 'A' && (C) <= 'F' ? (C)-'A'+10 : (C)-'0')
#define OCTTOBIN(C) ((C) - '0')

/* Debugging the memory allocator.  */

#if WITH_DMALLOC
# define MALLOC_FUNC_CHECK 1
# include <dmalloc.h>
#endif

/* Global definitions.  */

/* FIXME: There are many unchecked integer overflows in this file,
   that will cause this command to misbehave given large inputs or
   options.  Many of the "int" values below should be "size_t" or
   something else like that.  */

/* Program options.  */

enum Format
{
  UNKNOWN_FORMAT,		/* output format still unknown */
  DUMB_FORMAT,			/* output for a dumb terminal */
  ROFF_FORMAT,			/* output for 'troff' or 'nroff' */
  TEX_FORMAT			/* output for 'TeX' or 'LaTeX' */
};

static bool gnu_extensions = true;	/* trigger all GNU extensions */
static bool auto_reference = false;	/* refs are 'file_name:line_number:' */
static bool input_reference = false;	/* refs at beginning of input lines */
static bool right_reference = false;	/* output refs after right context  */
static int line_width = 72;	/* output line width in characters */
static int gap_size = 3;	/* number of spaces between output fields */
static const char *truncation_string = "/";
                                /* string used to mark line truncations */
static const char *macro_name = "xx";	/* macro name for roff or TeX output */
static enum Format output_format = UNKNOWN_FORMAT;
                                /* output format */

static bool ignore_case = false;	/* fold lower to upper for sorting */
static const char *break_file = NULL;	/* name of the 'Break chars' file */
static const char *only_file = NULL;	/* name of the 'Only words' file */
static const char *ignore_file = NULL;	/* name of the 'Ignore words' file */

/* Options that use regular expressions.  */
struct regex_data
{
  /* The original regular expression, as a string.  */
  char const *string;

  /* The compiled regular expression, and its fastmap.  */
  struct re_pattern_buffer pattern;
  char fastmap[UCHAR_MAX + 1];
};

static struct regex_data context_regex;	/* end of context */
static struct regex_data word_regex;	/* keyword */

/* A BLOCK delimit a region in memory of arbitrary size, like the copy of a
   whole file.  A WORD is something smaller, its length should fit in a
   short integer.  A WORD_TABLE may contain several WORDs.  */

typedef struct
  {
    char *start;		/* pointer to beginning of region */
    char *end;			/* pointer to end + 1 of region */
  }
BLOCK;

typedef struct
  {
    char *start;		/* pointer to beginning of region */
    short int size;		/* length of the region */
  }
WORD;

typedef struct
  {
    WORD *start;		/* array of WORDs */
    size_t alloc;		/* allocated length */
    size_t length;		/* number of used entries */
  }
WORD_TABLE;

/* Pattern description tables.  */

/* For each character, provide its folded equivalent.  */
static unsigned char folded_chars[CHAR_SET_SIZE];

/* End of context pattern register indices.  */
static struct re_registers context_regs;

/* Keyword pattern register indices.  */
static struct re_registers word_regs;

/* A word characters fastmap is used only when no word regexp has been
   provided.  A word is then made up of a sequence of one or more characters
   allowed by the fastmap.  Contains !0 if character allowed in word.  Not
   only this is faster in most cases, but it simplifies the implementation
   of the Break files.  */
static char word_fastmap[CHAR_SET_SIZE];

/* Maximum length of any word read.  */
static int maximum_word_length;

/* Maximum width of any reference used.  */
static int reference_max_width;

/* Ignore and Only word tables.  */

static WORD_TABLE ignore_table;	/* table of words to ignore */
static WORD_TABLE only_table;		/* table of words to select */

/* Source text table, and scanning macros.  */

static int number_input_files;	/* number of text input files */
static int total_line_count;	/* total number of lines seen so far */
static const char **input_file_name;	/* array of text input file names */
static int *file_line_count;	/* array of 'total_line_count' values at end */

static BLOCK *text_buffers;	/* files to study */

/* SKIP_NON_WHITE used only for getting or skipping the reference.  */

#define SKIP_NON_WHITE(cursor, limit) \
  while (cursor < limit && ! isspace (to_uchar (*cursor)))		\
    cursor++

#define SKIP_WHITE(cursor, limit) \
  while (cursor < limit && isspace (to_uchar (*cursor)))		\
    cursor++

#define SKIP_WHITE_BACKWARDS(cursor, start) \
  while (cursor > start && isspace (to_uchar (cursor[-1])))		\
    cursor--

#define SKIP_SOMETHING(cursor, limit) \
  if (word_regex.string)						\
    {									\
      regoff_t count;							\
      count = re_match (&word_regex.pattern, cursor, limit - cursor, 0, NULL); \
      if (count == -2)							\
        matcher_error ();						\
      cursor += count == -1 ? 1 : count;				\
    }									\
  else if (word_fastmap[to_uchar (*cursor)])				\
    while (cursor < limit && word_fastmap[to_uchar (*cursor)])		\
      cursor++;								\
  else									\
    cursor++

/* Occurrences table.

   The 'keyword' pointer provides the central word, which is surrounded
   by a left context and a right context.  The 'keyword' and 'length'
   field allow full 8-bit characters keys, even including NULs.  At other
   places in this program, the name 'keyafter' refers to the keyword
   followed by its right context.

   The left context does not extend, towards the beginning of the file,
   further than a distance given by the 'left' value.  This value is
   relative to the keyword beginning, it is usually negative.  This
   insures that, except for white space, we will never have to backward
   scan the source text, when it is time to generate the final output
   lines.

   The right context, indirectly attainable through the keyword end, does
   not extend, towards the end of the file, further than a distance given
   by the 'right' value.  This value is relative to the keyword
   beginning, it is usually positive.

   When automatic references are used, the 'reference' value is the
   overall line number in all input files read so far, in this case, it
   is of type (int).  When input references are used, the 'reference'
   value indicates the distance between the keyword beginning and the
   start of the reference field, it is of type (DELTA) and usually
   negative.  */

typedef short int DELTA;	/* to hold displacement within one context */

typedef struct
  {
    WORD key;			/* description of the keyword */
    DELTA left;			/* distance to left context start */
    DELTA right;		/* distance to right context end */
    int reference;		/* reference descriptor */
    size_t file_index;		/* corresponding file  */
  }
OCCURS;

/* The various OCCURS tables are indexed by the language.  But the time
   being, there is no such multiple language support.  */

static OCCURS *occurs_table[1];	/* all words retained from the read text */
static size_t occurs_alloc[1];	/* allocated size of occurs_table */
static size_t number_of_occurs[1]; /* number of used slots in occurs_table */


/* Communication among output routines.  */

/* Indicate if special output processing is requested for each character.  */
static char edited_flag[CHAR_SET_SIZE];

static int half_line_width;	/* half of line width, reference excluded */
static int before_max_width;	/* maximum width of before field */
static int keyafter_max_width;	/* maximum width of keyword-and-after field */
static int truncation_string_length;/* length of string that flags truncation */

/* When context is limited by lines, wraparound may happen on final output:
   the 'head' pointer gives access to some supplementary left context which
   will be seen at the end of the output line, the 'tail' pointer gives
   access to some supplementary right context which will be seen at the
   beginning of the output line. */

static BLOCK tail;		/* tail field */
static int tail_truncation;	/* flag truncation after the tail field */

static BLOCK before;		/* before field */
static int before_truncation;	/* flag truncation before the before field */

static BLOCK keyafter;		/* keyword-and-after field */
static int keyafter_truncation;	/* flag truncation after the keyafter field */

static BLOCK head;		/* head field */
static int head_truncation;	/* flag truncation before the head field */

static BLOCK reference;		/* reference field for input reference mode */

/* Miscellaneous routines.  */

/* Diagnose an error in the regular expression matcher.  Then exit.  */

static void ATTRIBUTE_NORETURN
matcher_error (void)
{
  error (0, errno, _("error in regular expression matcher"));
  exit (EXIT_FAILURE);
}

/*------------------------------------------------------.
| Duplicate string STRING, while evaluating \-escapes.  |
`------------------------------------------------------*/

/* Loosely adapted from GNU sh-utils printf.c code.  */

static char *
copy_unescaped_string (const char *string)
{
  char *result;			/* allocated result */
  char *cursor;			/* cursor in result */
  int value;			/* value of \nnn escape */
  int length;			/* length of \nnn escape */

  result = xmalloc (strlen (string) + 1);
  cursor = result;

  while (*string)
    {
      if (*string == '\\')
        {
          string++;
          switch (*string)
            {
            case 'x':		/* \xhhh escape, 3 chars maximum */
              value = 0;
              for (length = 0, string++;
                   length < 3 && isxdigit (to_uchar (*string));
                   length++, string++)
                value = value * 16 + HEXTOBIN (*string);
              if (length == 0)
                {
                  *cursor++ = '\\';
                  *cursor++ = 'x';
                }
              else
                *cursor++ = value;
              break;

            case '0':		/* \0ooo escape, 3 chars maximum */
              value = 0;
              for (length = 0, string++;
                   length < 3 && ISODIGIT (*string);
                   length++, string++)
                value = value * 8 + OCTTOBIN (*string);
              *cursor++ = value;
              break;

            case 'a':		/* alert */
#if __STDC__
              *cursor++ = '\a';
#else
              *cursor++ = 7;
#endif
              string++;
              break;

            case 'b':		/* backspace */
              *cursor++ = '\b';
              string++;
              break;

            case 'c':		/* cancel the rest of the output */
              while (*string)
                string++;
              break;

            case 'f':		/* form feed */
              *cursor++ = '\f';
              string++;
              break;

            case 'n':		/* new line */
              *cursor++ = '\n';
              string++;
              break;

            case 'r':		/* carriage return */
              *cursor++ = '\r';
              string++;
              break;

            case 't':		/* horizontal tab */
              *cursor++ = '\t';
              string++;
              break;

            case 'v':		/* vertical tab */
#if __STDC__
              *cursor++ = '\v';
#else
              *cursor++ = 11;
#endif
              string++;
              break;

            case '\0':		/* lone backslash at end of string */
              /* ignore it */
              break;

            default:
              *cursor++ = '\\';
              *cursor++ = *string++;
              break;
            }
        }
      else
        *cursor++ = *string++;
    }

  *cursor = '\0';
  return result;
}

/*--------------------------------------------------------------------------.
| Compile the regex represented by REGEX, diagnose and abort if any error.  |
`--------------------------------------------------------------------------*/

static void
compile_regex (struct regex_data *regex)
{
  struct re_pattern_buffer *pattern = &regex->pattern;
  char const *string = regex->string;
  char const *message;

  pattern->buffer = NULL;
  pattern->allocated = 0;
  pattern->fastmap = regex->fastmap;
  pattern->translate = ignore_case ? folded_chars : NULL;

  message = re_compile_pattern (string, strlen (string), pattern);
  if (message)
    error (EXIT_FAILURE, 0, _("%s (for regexp %s)"), message, quote (string));

  /* The fastmap should be compiled before 're_match'.  The following
     call is not mandatory, because 're_search' is always called sooner,
     and it compiles the fastmap if this has not been done yet.  */

  re_compile_fastmap (pattern);
}

/*------------------------------------------------------------------------.
| This will initialize various tables for pattern match and compiles some |
| regexps.								  |
`------------------------------------------------------------------------*/

static void
initialize_regex (void)
{
  int character;		/* character value */

  /* Initialize the case folding table.  */

  if (ignore_case)
    for (character = 0; character < CHAR_SET_SIZE; character++)
      folded_chars[character] = toupper (character);

  /* Unless the user already provided a description of the end of line or
     end of sentence sequence, select an end of line sequence to compile.
     If the user provided an empty definition, thus disabling end of line
     or sentence feature, make it NULL to speed up tests.  If GNU
     extensions are enabled, use end of sentence like in GNU emacs.  If
     disabled, use end of lines.  */

  if (context_regex.string)
    {
      if (!*context_regex.string)
        context_regex.string = NULL;
    }
  else if (gnu_extensions && !input_reference)
    context_regex.string = "[.?!][]\"')}]*\\($\\|\t\\|  \\)[ \t\n]*";
  else
    context_regex.string = "\n";

  if (context_regex.string)
    compile_regex (&context_regex);

  /* If the user has already provided a non-empty regexp to describe
     words, compile it.  Else, unless this has already been done through
     a user provided Break character file, construct a fastmap of
     characters that may appear in a word.  If GNU extensions enabled,
     include only letters of the underlying character set.  If disabled,
     include almost everything, even punctuations; stop only on white
     space.  */

  if (word_regex.string)
    compile_regex (&word_regex);
  else if (!break_file)
    {
      if (gnu_extensions)
        {

          /* Simulate \w+.  */

          for (character = 0; character < CHAR_SET_SIZE; character++)
            word_fastmap[character] = !! isalpha (character);
        }
      else
        {

          /* Simulate [^ \t\n]+.  */

          memset (word_fastmap, 1, CHAR_SET_SIZE);
          word_fastmap[' '] = 0;
          word_fastmap['\t'] = 0;
          word_fastmap['\n'] = 0;
        }
    }
}

/*------------------------------------------------------------------------.
| This routine will attempt to swallow a whole file name FILE_NAME into a |
| contiguous region of memory and return a description of it into BLOCK.  |
| Standard input is assumed whenever FILE_NAME is NULL, empty or "-".	  |
|									  |
| Previously, in some cases, white space compression was attempted while  |
| inputting text.  This was defeating some regexps like default end of	  |
| sentence, which checks for two consecutive spaces.  If white space	  |
| compression is ever reinstated, it should be in output routines.	  |
`------------------------------------------------------------------------*/

static void
swallow_file_in_memory (const char *file_name, BLOCK *block)
{
  size_t used_length;		/* used length in memory buffer */

  /* As special cases, a file name which is NULL or "-" indicates standard
     input, which is already opened.  In all other cases, open the file from
     its name.  */
  bool using_stdin = !file_name || !*file_name || STREQ (file_name, "-");
  if (using_stdin)
    block->start = fread_file (stdin, &used_length);
  else
    block->start = read_file (file_name, &used_length);

  if (!block->start)
    error (EXIT_FAILURE, errno, "%s", quotef (using_stdin ? "-" : file_name));

  block->end = block->start + used_length;
}

/* Sort and search routines.  */

/*--------------------------------------------------------------------------.
| Compare two words, FIRST and SECOND, and return 0 if they are identical.  |
| Return less than 0 if the first word goes before the second; return	    |
| greater than 0 if the first word goes after the second.		    |
|									    |
| If a word is indeed a prefix of the other, the shorter should go first.   |
`--------------------------------------------------------------------------*/

static int
compare_words (const void *void_first, const void *void_second)
{
#define first ((const WORD *) void_first)
#define second ((const WORD *) void_second)
  int length;			/* minimum of two lengths */
  int counter;			/* cursor in words */
  int value;			/* value of comparison */

  length = first->size < second->size ? first->size : second->size;

  if (ignore_case)
    {
      for (counter = 0; counter < length; counter++)
        {
          value = (folded_chars [to_uchar (first->start[counter])]
                   - folded_chars [to_uchar (second->start[counter])]);
          if (value != 0)
            return value;
        }
    }
  else
    {
      for (counter = 0; counter < length; counter++)
        {
          value = (to_uchar (first->start[counter])
                   - to_uchar (second->start[counter]));
          if (value != 0)
            return value;
        }
    }

  return first->size - second->size;
#undef first
#undef second
}

/*-----------------------------------------------------------------------.
| Decides which of two OCCURS, FIRST or SECOND, should lexicographically |
| go first.  In case of a tie, preserve the original order through a	 |
| pointer comparison.							 |
`-----------------------------------------------------------------------*/

static int
compare_occurs (const void *void_first, const void *void_second)
{
#define first ((const OCCURS *) void_first)
#define second ((const OCCURS *) void_second)
  int value;

  value = compare_words (&first->key, &second->key);
  return value == 0 ? first->key.start - second->key.start : value;
#undef first
#undef second
}

/*------------------------------------------------------------.
| Return !0 if WORD appears in TABLE.  Uses a binary search.  |
`------------------------------------------------------------*/

static int _GL_ATTRIBUTE_PURE
search_table (WORD *word, WORD_TABLE *table)
{
  int lowest;			/* current lowest possible index */
  int highest;			/* current highest possible index */
  int middle;			/* current middle index */
  int value;			/* value from last comparison */

  lowest = 0;
  highest = table->length - 1;
  while (lowest <= highest)
    {
      middle = (lowest + highest) / 2;
      value = compare_words (word, table->start + middle);
      if (value < 0)
        highest = middle - 1;
      else if (value > 0)
        lowest = middle + 1;
      else
        return 1;
    }
  return 0;
}

/*---------------------------------------------------------------------.
| Sort the whole occurs table in memory.  Presumably, 'qsort' does not |
| take intermediate copies or table elements, so the sort will be      |
| stabilized throughout the comparison routine.			       |
`---------------------------------------------------------------------*/

static void
sort_found_occurs (void)
{

  /* Only one language for the time being.  */
  if (number_of_occurs[0])
    qsort (occurs_table[0], number_of_occurs[0], sizeof **occurs_table,
           compare_occurs);
}

/* Parameter files reading routines.  */

/*----------------------------------------------------------------------.
| Read a file named FILE_NAME, containing a set of break characters.    |
| Build a content to the array word_fastmap in which all characters are |
| allowed except those found in the file.  Characters may be repeated.  |
`----------------------------------------------------------------------*/

static void
digest_break_file (const char *file_name)
{
  BLOCK file_contents;		/* to receive a copy of the file */
  char *cursor;			/* cursor in file copy */

  swallow_file_in_memory (file_name, &file_contents);

  /* Make the fastmap and record the file contents in it.  */

  memset (word_fastmap, 1, CHAR_SET_SIZE);
  for (cursor = file_contents.start; cursor < file_contents.end; cursor++)
    word_fastmap[to_uchar (*cursor)] = 0;

  if (!gnu_extensions)
    {

      /* If GNU extensions are enabled, the only way to avoid newline as
         a break character is to write all the break characters in the
         file with no newline at all, not even at the end of the file.
         If disabled, spaces, tabs and newlines are always considered as
         break characters even if not included in the break file.  */

      word_fastmap[' '] = 0;
      word_fastmap['\t'] = 0;
      word_fastmap['\n'] = 0;
    }

  /* Return the space of the file, which is no more required.  */

  free (file_contents.start);
}

/*-----------------------------------------------------------------------.
| Read a file named FILE_NAME, containing one word per line, then	 |
| construct in TABLE a table of WORD descriptors for them.  The routine	 |
| swallows the whole file in memory; this is at the expense of space	 |
| needed for newlines, which are useless; however, the reading is fast.	 |
`-----------------------------------------------------------------------*/

static void
digest_word_file (const char *file_name, WORD_TABLE *table)
{
  BLOCK file_contents;		/* to receive a copy of the file */
  char *cursor;			/* cursor in file copy */
  char *word_start;		/* start of the current word */

  swallow_file_in_memory (file_name, &file_contents);

  table->start = NULL;
  table->alloc = 0;
  table->length = 0;

  /* Read the whole file.  */

  cursor = file_contents.start;
  while (cursor < file_contents.end)
    {

      /* Read one line, and save the word in contains.  */

      word_start = cursor;
      while (cursor < file_contents.end && *cursor != '\n')
        cursor++;

      /* Record the word in table if it is not empty.  */

      if (cursor > word_start)
        {
          if (table->length == table->alloc)
            {
              if ((SIZE_MAX / sizeof *table->start - 1) / 2 < table->alloc)
                xalloc_die ();
              table->alloc = table->alloc * 2 + 1;
              table->start = xrealloc (table->start,
                                       table->alloc * sizeof *table->start);
            }

          table->start[table->length].start = word_start;
          table->start[table->length].size = cursor - word_start;
          table->length++;
        }

      /* This test allows for an incomplete line at end of file.  */

      if (cursor < file_contents.end)
        cursor++;
    }

  /* Finally, sort all the words read.  */

  qsort (table->start, table->length, sizeof table->start[0], compare_words);
}

/* Keyword recognition and selection.  */

/*----------------------------------------------------------------------.
| For each keyword in the source text, constructs an OCCURS structure.  |
`----------------------------------------------------------------------*/

static void
find_occurs_in_text (size_t file_index)
{
  char *cursor;			/* for scanning the source text */
  char *scan;			/* for scanning the source text also */
  char *line_start;		/* start of the current input line */
  char *line_scan;		/* newlines scanned until this point */
  int reference_length;		/* length of reference in input mode */
  WORD possible_key;		/* possible key, to ease searches */
  OCCURS *occurs_cursor;	/* current OCCURS under construction */

  char *context_start;		/* start of left context */
  char *context_end;		/* end of right context */
  char *word_start;		/* start of word */
  char *word_end;		/* end of word */
  char *next_context_start;	/* next start of left context */

  const BLOCK *text_buffer = &text_buffers[file_index];

  /* reference_length is always used within 'if (input_reference)'.
     However, GNU C diagnoses that it may be used uninitialized.  The
     following assignment is merely to shut it up.  */

  reference_length = 0;

  /* Tracking where lines start is helpful for reference processing.  In
     auto reference mode, this allows counting lines.  In input reference
     mode, this permits finding the beginning of the references.

     The first line begins with the file, skip immediately this very first
     reference in input reference mode, to help further rejection any word
     found inside it.  Also, unconditionally assigning these variable has
     the happy effect of shutting up lint.  */

  line_start = text_buffer->start;
  line_scan = line_start;
  if (input_reference)
    {
      SKIP_NON_WHITE (line_scan, text_buffer->end);
      reference_length = line_scan - line_start;
      SKIP_WHITE (line_scan, text_buffer->end);
    }

  /* Process the whole buffer, one line or one sentence at a time.  */

  for (cursor = text_buffer->start;
       cursor < text_buffer->end;
       cursor = next_context_start)
    {

      /* 'context_start' gets initialized before the processing of each
         line, or once for the whole buffer if no end of line or sentence
         sequence separator.  */

      context_start = cursor;

      /* If an end of line or end of sentence sequence is defined and
         non-empty, 'next_context_start' will be recomputed to be the end of
         each line or sentence, before each one is processed.  If no such
         sequence, then 'next_context_start' is set at the end of the whole
         buffer, which is then considered to be a single line or sentence.
         This test also accounts for the case of an incomplete line or
         sentence at the end of the buffer.  */

      next_context_start = text_buffer->end;
      if (context_regex.string)
        switch (re_search (&context_regex.pattern, cursor,
                           text_buffer->end - cursor,
                           0, text_buffer->end - cursor, &context_regs))
          {
          case -2:
            matcher_error ();

          case -1:
            break;

          default:
            next_context_start = cursor + context_regs.end[0];
            break;
          }

      /* Include the separator into the right context, but not any suffix
         white space in this separator; this insures it will be seen in
         output and will not take more space than necessary.  */

      context_end = next_context_start;
      SKIP_WHITE_BACKWARDS (context_end, context_start);

      /* Read and process a single input line or sentence, one word at a
         time.  */

      while (1)
        {
          if (word_regex.string)

            /* If a word regexp has been compiled, use it to skip at the
               beginning of the next word.  If there is no such word, exit
               the loop.  */

            {
              regoff_t r = re_search (&word_regex.pattern, cursor,
                                      context_end - cursor,
                                      0, context_end - cursor, &word_regs);
              if (r == -2)
                matcher_error ();
              if (r == -1)
                break;
              word_start = cursor + word_regs.start[0];
              word_end = cursor + word_regs.end[0];
            }
          else

            /* Avoid re_search and use the fastmap to skip to the
               beginning of the next word.  If there is no more word in
               the buffer, exit the loop.  */

            {
              scan = cursor;
              while (scan < context_end
                     && !word_fastmap[to_uchar (*scan)])
                scan++;

              if (scan == context_end)
                break;

              word_start = scan;

              while (scan < context_end
                     && word_fastmap[to_uchar (*scan)])
                scan++;

              word_end = scan;
            }

          /* Skip right to the beginning of the found word.  */

          cursor = word_start;

          /* Skip any zero length word.  Just advance a single position,
             then go fetch the next word.  */

          if (word_end == word_start)
            {
              cursor++;
              continue;
            }

          /* This is a genuine, non empty word, so save it as a possible
             key.  Then skip over it.  Also, maintain the maximum length of
             all words read so far.  It is mandatory to take the maximum
             length of all words in the file, without considering if they
             are actually kept or rejected, because backward jumps at output
             generation time may fall in *any* word.  */

          possible_key.start = cursor;
          possible_key.size = word_end - word_start;
          cursor += possible_key.size;

          if (possible_key.size > maximum_word_length)
            maximum_word_length = possible_key.size;

          /* In input reference mode, update 'line_start' from its previous
             value.  Count the lines just in case auto reference mode is
             also selected. If it happens that the word just matched is
             indeed part of a reference; just ignore it.  */

          if (input_reference)
            {
              while (line_scan < possible_key.start)
                if (*line_scan == '\n')
                  {
                    total_line_count++;
                    line_scan++;
                    line_start = line_scan;
                    SKIP_NON_WHITE (line_scan, text_buffer->end);
                    reference_length = line_scan - line_start;
                  }
                else
                  line_scan++;
              if (line_scan > possible_key.start)
                continue;
            }

          /* Ignore the word if an 'Ignore words' table exists and if it is
             part of it.  Also ignore the word if an 'Only words' table and
             if it is *not* part of it.

             It is allowed that both tables be used at once, even if this
             may look strange for now.  Just ignore a word that would appear
             in both.  If regexps are eventually implemented for these
             tables, the Ignore table could then reject words that would
             have been previously accepted by the Only table.  */

          if (ignore_file && search_table (&possible_key, &ignore_table))
            continue;
          if (only_file && !search_table (&possible_key, &only_table))
            continue;

          /* A non-empty word has been found.  First of all, insure
             proper allocation of the next OCCURS, and make a pointer to
             where it will be constructed.  */

          if (number_of_occurs[0] == occurs_alloc[0])
            {
              if ((SIZE_MAX / sizeof *occurs_table[0] - 1) / 2
                  < occurs_alloc[0])
                xalloc_die ();
              occurs_alloc[0] = occurs_alloc[0] * 2 + 1;
              occurs_table[0] =
                xrealloc (occurs_table[0],
                          occurs_alloc[0] * sizeof *occurs_table[0]);
            }

          occurs_cursor = occurs_table[0] + number_of_occurs[0];

          /* Define the reference field, if any.  */

          if (auto_reference)
            {

              /* While auto referencing, update 'line_start' from its
                 previous value, counting lines as we go.  If input
                 referencing at the same time, 'line_start' has been
                 advanced earlier, and the following loop is never really
                 executed.  */

              while (line_scan < possible_key.start)
                if (*line_scan == '\n')
                  {
                    total_line_count++;
                    line_scan++;
                    line_start = line_scan;
                    SKIP_NON_WHITE (line_scan, text_buffer->end);
                  }
                else
                  line_scan++;

              occurs_cursor->reference = total_line_count;
            }
          else if (input_reference)
            {

              /* If only input referencing, 'line_start' has been computed
                 earlier to detect the case the word matched would be part
                 of the reference.  The reference position is simply the
                 value of 'line_start'.  */

              occurs_cursor->reference
                = (DELTA) (line_start - possible_key.start);
              if (reference_length > reference_max_width)
                reference_max_width = reference_length;
            }

          /* Exclude the reference from the context in simple cases.  */

          if (input_reference && line_start == context_start)
            {
              SKIP_NON_WHITE (context_start, context_end);
              SKIP_WHITE (context_start, context_end);
            }

          /* Completes the OCCURS structure.  */

          occurs_cursor->key = possible_key;
          occurs_cursor->left = context_start - possible_key.start;
          occurs_cursor->right = context_end - possible_key.start;
          occurs_cursor->file_index = file_index;

          number_of_occurs[0]++;
        }
    }
}

/* Formatting and actual output - service routines.  */

/*-----------------------------------------.
| Prints some NUMBER of spaces on stdout.  |
`-----------------------------------------*/

static void
print_spaces (int number)
{
  int counter;

  for (counter = number; counter > 0; counter--)
    putchar (' ');
}

/*-------------------------------------.
| Prints the field provided by FIELD.  |
`-------------------------------------*/

static void
print_field (BLOCK field)
{
  char *cursor;			/* Cursor in field to print */
  int base;			/* Base character, without diacritic */
  int diacritic;		/* Diacritic code for the character */

  /* Whitespace is not really compressed.  Instead, each white space
     character (tab, vt, ht etc.) is printed as one single space.  */

  for (cursor = field.start; cursor < field.end; cursor++)
    {
      unsigned char character = *cursor;
      if (edited_flag[character])
        {

          /* First check if this is a diacriticized character.

             This works only for TeX.  I do not know how diacriticized
             letters work with 'roff'.  Please someone explain it to me!  */

          diacritic = todiac (character);
          if (diacritic != 0 && output_format == TEX_FORMAT)
            {
              base = tobase (character);
              switch (diacritic)
                {

                case 1:		/* Latin diphthongs */
                  switch (base)
                    {
                    case 'o':
                      fputs ("\\oe{}", stdout);
                      break;

                    case 'O':
                      fputs ("\\OE{}", stdout);
                      break;

                    case 'a':
                      fputs ("\\ae{}", stdout);
                      break;

                    case 'A':
                      fputs ("\\AE{}", stdout);
                      break;

                    default:
                      putchar (' ');
                    }
                  break;

                case 2:		/* Acute accent */
                  printf ("\\'%s%c", (base == 'i' ? "\\" : ""), base);
                  break;

                case 3:		/* Grave accent */
                  printf ("\\'%s%c", (base == 'i' ? "\\" : ""), base);
                  break;

                case 4:		/* Circumflex accent */
                  printf ("\\^%s%c", (base == 'i' ? "\\" : ""), base);
                  break;

                case 5:		/* Diaeresis */
                  printf ("\\\"%s%c", (base == 'i' ? "\\" : ""), base);
                  break;

                case 6:		/* Tilde accent */
                  printf ("\\~%s%c", (base == 'i' ? "\\" : ""), base);
                  break;

                case 7:		/* Cedilla */
                  printf ("\\c{%c}", base);
                  break;

                case 8:		/* Small circle beneath */
                  switch (base)
                    {
                    case 'a':
                      fputs ("\\aa{}", stdout);
                      break;

                    case 'A':
                      fputs ("\\AA{}", stdout);
                      break;

                    default:
                      putchar (' ');
                    }
                  break;

                case 9:		/* Strike through */
                  switch (base)
                    {
                    case 'o':
                      fputs ("\\o{}", stdout);
                      break;

                    case 'O':
                      fputs ("\\O{}", stdout);
                      break;

                    default:
                      putchar (' ');
                    }
                  break;
                }
            }
          else

            /* This is not a diacritic character, so handle cases which are
               really specific to 'roff' or TeX.  All white space processing
               is done as the default case of this switch.  */

            switch (character)
              {
              case '"':
                /* In roff output format, double any quote.  */
                putchar ('"');
                putchar ('"');
                break;

              case '$':
              case '%':
              case '&':
              case '#':
              case '_':
                /* In TeX output format, precede these with a backslash.  */
                putchar ('\\');
                putchar (character);
                break;

              case '{':
              case '}':
                /* In TeX output format, precede these with a backslash and
                   force mathematical mode.  */
                printf ("$\\%c$", character);
                break;

              case '\\':
                /* In TeX output mode, request production of a backslash.  */
                fputs ("\\backslash{}", stdout);
                break;

              default:
                /* Any other flagged character produces a single space.  */
                putchar (' ');
              }
        }
      else
        putchar (*cursor);
    }
}

/* Formatting and actual output - planning routines.  */

/*--------------------------------------------------------------------.
| From information collected from command line options and input file |
| readings, compute and fix some output parameter values.	      |
`--------------------------------------------------------------------*/

static void
fix_output_parameters (void)
{
  int file_index;		/* index in text input file arrays */
  int line_ordinal;		/* line ordinal value for reference */
  char ordinal_string[12];	/* edited line ordinal for reference */
  int reference_width;		/* width for the whole reference */
  int character;		/* character ordinal */
  const char *cursor;		/* cursor in some constant strings */

  /* In auto reference mode, the maximum width of this field is
     precomputed and subtracted from the overall line width.  Add one for
     the column which separate the file name from the line number.  */

  if (auto_reference)
    {
      reference_max_width = 0;
      for (file_index = 0; file_index < number_input_files; file_index++)
        {
          line_ordinal = file_line_count[file_index] + 1;
          if (file_index > 0)
            line_ordinal -= file_line_count[file_index - 1];
          sprintf (ordinal_string, "%d", line_ordinal);
          reference_width = strlen (ordinal_string);
          if (input_file_name[file_index])
            reference_width += strlen (input_file_name[file_index]);
          if (reference_width > reference_max_width)
            reference_max_width = reference_width;
        }
      reference_max_width++;
      reference.start = xmalloc ((size_t) reference_max_width + 1);
    }

  /* If the reference appears to the left of the output line, reserve some
     space for it right away, including one gap size.  */

  if ((auto_reference || input_reference) && !right_reference)
    line_width -= reference_max_width + gap_size;

  /* The output lines, minimally, will contain from left to right a left
     context, a gap, and a keyword followed by the right context with no
     special intervening gap.  Half of the line width is dedicated to the
     left context and the gap, the other half is dedicated to the keyword
     and the right context; these values are computed once and for all here.
     There also are tail and head wrap around fields, used when the keyword
     is near the beginning or the end of the line, or when some long word
     cannot fit in, but leave place from wrapped around shorter words.  The
     maximum width of these fields are recomputed separately for each line,
     on a case by case basis.  It is worth noting that it cannot happen that
     both the tail and head fields are used at once.  */

  half_line_width = line_width / 2;
  before_max_width = half_line_width - gap_size;
  keyafter_max_width = half_line_width;

  /* If truncation_string is the empty string, make it NULL to speed up
     tests.  In this case, truncation_string_length will never get used, so
     there is no need to set it.  */

  if (truncation_string && *truncation_string)
    truncation_string_length = strlen (truncation_string);
  else
    truncation_string = NULL;

  if (gnu_extensions)
    {

      /* When flagging truncation at the left of the keyword, the
         truncation mark goes at the beginning of the before field,
         unless there is a head field, in which case the mark goes at the
         left of the head field.  When flagging truncation at the right
         of the keyword, the mark goes at the end of the keyafter field,
         unless there is a tail field, in which case the mark goes at the
         end of the tail field.  Only eight combination cases could arise
         for truncation marks:

         . None.
         . One beginning the before field.
         . One beginning the head field.
         . One ending the keyafter field.
         . One ending the tail field.
         . One beginning the before field, another ending the keyafter field.
         . One ending the tail field, another beginning the before field.
         . One ending the keyafter field, another beginning the head field.

         So, there is at most two truncation marks, which could appear both
         on the left side of the center of the output line, both on the
         right side, or one on either side.  */

      before_max_width -= 2 * truncation_string_length;
      if (before_max_width < 0)
        before_max_width = 0;
      keyafter_max_width -= 2 * truncation_string_length;
    }
  else
    {

      /* I never figured out exactly how UNIX' ptx plans the output width
         of its various fields.  If GNU extensions are disabled, do not
         try computing the field widths correctly; instead, use the
         following formula, which does not completely imitate UNIX' ptx,
         but almost.  */

      keyafter_max_width -= 2 * truncation_string_length + 1;
    }

  /* Compute which characters need special output processing.  Initialize
     by flagging any white space character.  Some systems do not consider
     form feed as a space character, but we do.  */

  for (character = 0; character < CHAR_SET_SIZE; character++)
    edited_flag[character] = !! isspace (character);
  edited_flag['\f'] = 1;

  /* Complete the special character flagging according to selected output
     format.  */

  switch (output_format)
    {
    case UNKNOWN_FORMAT:
      /* Should never happen.  */

    case DUMB_FORMAT:
      break;

    case ROFF_FORMAT:

      /* 'Quote' characters should be doubled.  */

      edited_flag['"'] = 1;
      break;

    case TEX_FORMAT:

      /* Various characters need special processing.  */

      for (cursor = "$%&#_{}\\"; *cursor; cursor++)
        edited_flag[to_uchar (*cursor)] = 1;

      /* Any character with 8th bit set will print to a single space, unless
         it is diacriticized.  */

      for (character = 0200; character < CHAR_SET_SIZE; character++)
        edited_flag[character] = todiac (character) != 0;
      break;
    }
}

/*------------------------------------------------------------------.
| Compute the position and length of all the output fields, given a |
| pointer to some OCCURS.					    |
`------------------------------------------------------------------*/

static void
define_all_fields (OCCURS *occurs)
{
  int tail_max_width;		/* allowable width of tail field */
  int head_max_width;		/* allowable width of head field */
  char *cursor;			/* running cursor in source text */
  char *left_context_start;	/* start of left context */
  char *right_context_end;	/* end of right context */
  char *left_field_start;	/* conservative start for 'head'/'before' */
  const char *file_name;	/* file name for reference */
  int line_ordinal;		/* line ordinal for reference */
  const char *buffer_start;	/* start of buffered file for this occurs */
  const char *buffer_end;	/* end of buffered file for this occurs */

  /* Define 'keyafter', start of left context and end of right context.
     'keyafter' starts at the saved position for keyword and extend to the
     right from the end of the keyword, eating separators or full words, but
     not beyond maximum allowed width for 'keyafter' field or limit for the
     right context.  Suffix spaces will be removed afterwards.  */

  keyafter.start = occurs->key.start;
  keyafter.end = keyafter.start + occurs->key.size;
  left_context_start = keyafter.start + occurs->left;
  right_context_end = keyafter.start + occurs->right;

  buffer_start = text_buffers[occurs->file_index].start;
  buffer_end = text_buffers[occurs->file_index].end;

  cursor = keyafter.end;
  while (cursor < right_context_end
         && cursor <= keyafter.start + keyafter_max_width)
    {
      keyafter.end = cursor;
      SKIP_SOMETHING (cursor, right_context_end);
    }
  if (cursor <= keyafter.start + keyafter_max_width)
    keyafter.end = cursor;

  keyafter_truncation = truncation_string && keyafter.end < right_context_end;

  SKIP_WHITE_BACKWARDS (keyafter.end, keyafter.start);

  /* When the left context is wide, it might take some time to catch up from
     the left context boundary to the beginning of the 'head' or 'before'
     fields.  So, in this case, to speed the catchup, we jump back from the
     keyword, using some secure distance, possibly falling in the middle of
     a word.  A secure backward jump would be at least half the maximum
     width of a line, plus the size of the longest word met in the whole
     input.  We conclude this backward jump by a skip forward of at least
     one word.  In this manner, we should not inadvertently accept only part
     of a word.  From the reached point, when it will be time to fix the
     beginning of 'head' or 'before' fields, we will skip forward words or
     delimiters until we get sufficiently near.  */

  if (-occurs->left > half_line_width + maximum_word_length)
    {
      left_field_start
        = keyafter.start - (half_line_width + maximum_word_length);
      SKIP_SOMETHING (left_field_start, keyafter.start);
    }
  else
    left_field_start = keyafter.start + occurs->left;

  /* 'before' certainly ends at the keyword, but not including separating
     spaces.  It starts after than the saved value for the left context, by
     advancing it until it falls inside the maximum allowed width for the
     before field.  There will be no prefix spaces either.  'before' only
     advances by skipping single separators or whole words. */

  before.start = left_field_start;
  before.end = keyafter.start;
  SKIP_WHITE_BACKWARDS (before.end, before.start);

  while (before.start + before_max_width < before.end)
    SKIP_SOMETHING (before.start, before.end);

  if (truncation_string)
    {
      cursor = before.start;
      SKIP_WHITE_BACKWARDS (cursor, buffer_start);
      before_truncation = cursor > left_context_start;
    }
  else
    before_truncation = 0;

  SKIP_WHITE (before.start, buffer_end);

  /* The tail could not take more columns than what has been left in the
     left context field, and a gap is mandatory.  It starts after the
     right context, and does not contain prefixed spaces.  It ends at
     the end of line, the end of buffer or when the tail field is full,
     whichever comes first.  It cannot contain only part of a word, and
     has no suffixed spaces.  */

  tail_max_width
    = before_max_width - (before.end - before.start) - gap_size;

  if (tail_max_width > 0)
    {
      tail.start = keyafter.end;
      SKIP_WHITE (tail.start, buffer_end);

      tail.end = tail.start;
      cursor = tail.end;
      while (cursor < right_context_end
             && cursor < tail.start + tail_max_width)
        {
          tail.end = cursor;
          SKIP_SOMETHING (cursor, right_context_end);
        }

      if (cursor < tail.start + tail_max_width)
        tail.end = cursor;

      if (tail.end > tail.start)
        {
          keyafter_truncation = 0;
          tail_truncation = truncation_string && tail.end < right_context_end;
        }
      else
        tail_truncation = 0;

      SKIP_WHITE_BACKWARDS (tail.end, tail.start);
    }
  else
    {

      /* No place left for a tail field.  */

      tail.start = NULL;
      tail.end = NULL;
      tail_truncation = 0;
    }

  /* 'head' could not take more columns than what has been left in the right
     context field, and a gap is mandatory.  It ends before the left
     context, and does not contain suffixed spaces.  Its pointer is advanced
     until the head field has shrunk to its allowed width.  It cannot
     contain only part of a word, and has no suffixed spaces.  */

  head_max_width
    = keyafter_max_width - (keyafter.end - keyafter.start) - gap_size;

  if (head_max_width > 0)
    {
      head.end = before.start;
      SKIP_WHITE_BACKWARDS (head.end, buffer_start);

      head.start = left_field_start;
      while (head.start + head_max_width < head.end)
        SKIP_SOMETHING (head.start, head.end);

      if (head.end > head.start)
        {
          before_truncation = 0;
          head_truncation = (truncation_string
                             && head.start > left_context_start);
        }
      else
        head_truncation = 0;

      SKIP_WHITE (head.start, head.end);
    }
  else
    {

      /* No place left for a head field.  */

      head.start = NULL;
      head.end = NULL;
      head_truncation = 0;
    }

  if (auto_reference)
    {

      /* Construct the reference text in preallocated space from the file
         name and the line number.  Standard input yields an empty file name.
         Ensure line numbers are 1 based, even if they are computed 0 based.  */

      file_name = input_file_name[occurs->file_index];
      if (!file_name)
        file_name = "";

      line_ordinal = occurs->reference + 1;
      if (occurs->file_index > 0)
        line_ordinal -= file_line_count[occurs->file_index - 1];

      sprintf (reference.start, "%s:%d", file_name, line_ordinal);
      reference.end = reference.start + strlen (reference.start);
    }
  else if (input_reference)
    {

      /* Reference starts at saved position for reference and extends right
         until some white space is met.  */

      reference.start = keyafter.start + (DELTA) occurs->reference;
      reference.end = reference.start;
      SKIP_NON_WHITE (reference.end, right_context_end);
    }
}

/* Formatting and actual output - control routines.  */

/*----------------------------------------------------------------------.
| Output the current output fields as one line for 'troff' or 'nroff'.  |
`----------------------------------------------------------------------*/

static void
output_one_roff_line (void)
{
  /* Output the 'tail' field.  */

  printf (".%s \"", macro_name);
  print_field (tail);
  if (tail_truncation)
    fputs (truncation_string, stdout);
  putchar ('"');

  /* Output the 'before' field.  */

  fputs (" \"", stdout);
  if (before_truncation)
    fputs (truncation_string, stdout);
  print_field (before);
  putchar ('"');

  /* Output the 'keyafter' field.  */

  fputs (" \"", stdout);
  print_field (keyafter);
  if (keyafter_truncation)
    fputs (truncation_string, stdout);
  putchar ('"');

  /* Output the 'head' field.  */

  fputs (" \"", stdout);
  if (head_truncation)
    fputs (truncation_string, stdout);
  print_field (head);
  putchar ('"');

  /* Conditionally output the 'reference' field.  */

  if (auto_reference || input_reference)
    {
      fputs (" \"", stdout);
      print_field (reference);
      putchar ('"');
    }

  putchar ('\n');
}

/*---------------------------------------------------------.
| Output the current output fields as one line for 'TeX'.  |
`---------------------------------------------------------*/

static void
output_one_tex_line (void)
{
  BLOCK key;			/* key field, isolated */
  BLOCK after;			/* after field, isolated */
  char *cursor;			/* running cursor in source text */

  printf ("\\%s ", macro_name);
  putchar ('{');
  print_field (tail);
  fputs ("}{", stdout);
  print_field (before);
  fputs ("}{", stdout);
  key.start = keyafter.start;
  after.end = keyafter.end;
  cursor = keyafter.start;
  SKIP_SOMETHING (cursor, keyafter.end);
  key.end = cursor;
  after.start = cursor;
  print_field (key);
  fputs ("}{", stdout);
  print_field (after);
  fputs ("}{", stdout);
  print_field (head);
  putchar ('}');
  if (auto_reference || input_reference)
    {
      putchar ('{');
      print_field (reference);
      putchar ('}');
    }
  putchar ('\n');
}

/*-------------------------------------------------------------------.
| Output the current output fields as one line for a dumb terminal.  |
`-------------------------------------------------------------------*/

static void
output_one_dumb_line (void)
{
  if (!right_reference)
    {
      if (auto_reference)
        {

          /* Output the 'reference' field, in such a way that GNU emacs
             next-error will handle it.  The ending colon is taken from the
             gap which follows.  */

          print_field (reference);
          putchar (':');
          print_spaces (reference_max_width
                        + gap_size
                        - (reference.end - reference.start)
                        - 1);
        }
      else
        {

          /* Output the 'reference' field and its following gap.  */

          print_field (reference);
          print_spaces (reference_max_width
                        + gap_size
                        - (reference.end - reference.start));
        }
    }

  if (tail.start < tail.end)
    {
      /* Output the 'tail' field.  */

      print_field (tail);
      if (tail_truncation)
        fputs (truncation_string, stdout);

      print_spaces (half_line_width - gap_size
                    - (before.end - before.start)
                    - (before_truncation ? truncation_string_length : 0)
                    - (tail.end - tail.start)
                    - (tail_truncation ? truncation_string_length : 0));
    }
  else
    print_spaces (half_line_width - gap_size
                  - (before.end - before.start)
                  - (before_truncation ? truncation_string_length : 0));

  /* Output the 'before' field.  */

  if (before_truncation)
    fputs (truncation_string, stdout);
  print_field (before);

  print_spaces (gap_size);

  /* Output the 'keyafter' field.  */

  print_field (keyafter);
  if (keyafter_truncation)
    fputs (truncation_string, stdout);

  if (head.start < head.end)
    {
      /* Output the 'head' field.  */

      print_spaces (half_line_width
                    - (keyafter.end - keyafter.start)
                    - (keyafter_truncation ? truncation_string_length : 0)
                    - (head.end - head.start)
                    - (head_truncation ? truncation_string_length : 0));
      if (head_truncation)
        fputs (truncation_string, stdout);
      print_field (head);
    }
  else

    if ((auto_reference || input_reference) && right_reference)
      print_spaces (half_line_width
                    - (keyafter.end - keyafter.start)
                    - (keyafter_truncation ? truncation_string_length : 0));

  if ((auto_reference || input_reference) && right_reference)
    {
      /* Output the 'reference' field.  */

      print_spaces (gap_size);
      print_field (reference);
    }

  putchar ('\n');
}

/*------------------------------------------------------------------------.
| Scan the whole occurs table and, for each entry, output one line in the |
| appropriate format.							  |
`------------------------------------------------------------------------*/

static void
generate_all_output (void)
{
  size_t occurs_index;		/* index of keyword entry being processed */
  OCCURS *occurs_cursor;	/* current keyword entry being processed */

  /* The following assignments are useful to provide default values in case
     line contexts or references are not used, in which case these variables
     would never be computed.  */

  tail.start = NULL;
  tail.end = NULL;
  tail_truncation = 0;

  head.start = NULL;
  head.end = NULL;
  head_truncation = 0;

  /* Loop over all keyword occurrences.  */

  occurs_cursor = occurs_table[0];

  for (occurs_index = 0; occurs_index < number_of_occurs[0]; occurs_index++)
    {
      /* Compute the exact size of every field and whenever truncation flags
         are present or not.  */

      define_all_fields (occurs_cursor);

      /* Produce one output line according to selected format.  */

      switch (output_format)
        {
        case UNKNOWN_FORMAT:
          /* Should never happen.  */

        case DUMB_FORMAT:
          output_one_dumb_line ();
          break;

        case ROFF_FORMAT:
          output_one_roff_line ();
          break;

        case TEX_FORMAT:
          output_one_tex_line ();
          break;
        }

      /* Advance the cursor into the occurs table.  */

      occurs_cursor++;
    }
}

/* Option decoding and main program.  */

/*------------------------------------------------------.
| Print program identification and options, then exit.  |
`------------------------------------------------------*/

void
usage (int status)
{
  if (status != EXIT_SUCCESS)
    emit_try_help ();
  else
    {
      printf (_("\
Usage: %s [OPTION]... [INPUT]...   (without -G)\n\
  or:  %s -G [OPTION]... [INPUT [OUTPUT]]\n"),
              program_name, program_name);
      fputs (_("\
Output a permuted index, including context, of the words in the input files.\n\
"), stdout);

      emit_stdin_note ();
      emit_mandatory_arg_note ();

      fputs (_("\
  -A, --auto-reference           output automatically generated references\n\
  -G, --traditional              behave more like System V 'ptx'\n\
"), stdout);
      fputs (_("\
  -F, --flag-truncation=STRING   use STRING for flagging line truncations.\n\
                                 The default is '/'\n\
"), stdout);
      fputs (_("\
  -M, --macro-name=STRING        macro name to use instead of 'xx'\n\
  -O, --format=roff              generate output as roff directives\n\
  -R, --right-side-refs          put references at right, not counted in -w\n\
  -S, --sentence-regexp=REGEXP   for end of lines or end of sentences\n\
  -T, --format=tex               generate output as TeX directives\n\
"), stdout);
      fputs (_("\
  -W, --word-regexp=REGEXP       use REGEXP to match each keyword\n\
  -b, --break-file=FILE          word break characters in this FILE\n\
  -f, --ignore-case              fold lower case to upper case for sorting\n\
  -g, --gap-size=NUMBER          gap size in columns between output fields\n\
  -i, --ignore-file=FILE         read ignore word list from FILE\n\
  -o, --only-file=FILE           read only word list from this FILE\n\
"), stdout);
      fputs (_("\
  -r, --references               first field of each line is a reference\n\
  -t, --typeset-mode               - not implemented -\n\
  -w, --width=NUMBER             output width in columns, reference excluded\n\
"), stdout);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      emit_ancillary_info (PROGRAM_NAME);
    }
  exit (status);
}

/*----------------------------------------------------------------------.
| Main program.  Decode ARGC arguments passed through the ARGV array of |
| strings, then launch execution.				        |
`----------------------------------------------------------------------*/

/* Long options equivalences.  */
static struct option const long_options[] =
{
  {"auto-reference", no_argument, NULL, 'A'},
  {"break-file", required_argument, NULL, 'b'},
  {"flag-truncation", required_argument, NULL, 'F'},
  {"ignore-case", no_argument, NULL, 'f'},
  {"gap-size", required_argument, NULL, 'g'},
  {"ignore-file", required_argument, NULL, 'i'},
  {"macro-name", required_argument, NULL, 'M'},
  {"only-file", required_argument, NULL, 'o'},
  {"references", no_argument, NULL, 'r'},
  {"right-side-refs", no_argument, NULL, 'R'},
  {"format", required_argument, NULL, 10},
  {"sentence-regexp", required_argument, NULL, 'S'},
  {"traditional", no_argument, NULL, 'G'},
  {"typeset-mode", no_argument, NULL, 't'},
  {"width", required_argument, NULL, 'w'},
  {"word-regexp", required_argument, NULL, 'W'},
  {GETOPT_HELP_OPTION_DECL},
  {GETOPT_VERSION_OPTION_DECL},
  {NULL, 0, NULL, 0},
};

static char const* const format_args[] =
{
  "roff", "tex", NULL
};

static enum Format const format_vals[] =
{
  ROFF_FORMAT, TEX_FORMAT
};

int
main (int argc, char **argv)
{
  int optchar;			/* argument character */
  int file_index;		/* index in text input file arrays */

  /* Decode program options.  */

  initialize_main (&argc, &argv);
  set_program_name (argv[0]);
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

#if HAVE_SETCHRCLASS
  setchrclass (NULL);
#endif

  while (optchar = getopt_long (argc, argv, "AF:GM:ORS:TW:b:i:fg:o:trw:",
                                long_options, NULL),
         optchar != EOF)
    {
      switch (optchar)
        {
        default:
          usage (EXIT_FAILURE);

        case 'G':
          gnu_extensions = false;
          break;

        case 'b':
          break_file = optarg;
          break;

        case 'f':
          ignore_case = true;
          break;

        case 'g':
          {
            unsigned long int tmp_ulong;
            if (xstrtoul (optarg, NULL, 0, &tmp_ulong, NULL) != LONGINT_OK
                || ! (0 < tmp_ulong && tmp_ulong <= INT_MAX))
              error (EXIT_FAILURE, 0, _("invalid gap width: %s"),
                     quote (optarg));
            gap_size = tmp_ulong;
            break;
          }

        case 'i':
          ignore_file = optarg;
          break;

        case 'o':
          only_file = optarg;
          break;

        case 'r':
          input_reference = true;
          break;

        case 't':
          /* Yet to understand...  */
          break;

        case 'w':
          {
            unsigned long int tmp_ulong;
            if (xstrtoul (optarg, NULL, 0, &tmp_ulong, NULL) != LONGINT_OK
                || ! (0 < tmp_ulong && tmp_ulong <= INT_MAX))
              error (EXIT_FAILURE, 0, _("invalid line width: %s"),
                     quote (optarg));
            line_width = tmp_ulong;
            break;
          }

        case 'A':
          auto_reference = true;
          break;

        case 'F':
          truncation_string = copy_unescaped_string (optarg);
          break;

        case 'M':
          macro_name = optarg;
          break;

        case 'O':
          output_format = ROFF_FORMAT;
          break;

        case 'R':
          right_reference = true;
          break;

        case 'S':
          context_regex.string = copy_unescaped_string (optarg);
          break;

        case 'T':
          output_format = TEX_FORMAT;
          break;

        case 'W':
          word_regex.string = copy_unescaped_string (optarg);
          if (!*word_regex.string)
            word_regex.string = NULL;
          break;

        case 10:
          output_format = XARGMATCH ("--format", optarg,
                                     format_args, format_vals);
          break;

        case_GETOPT_HELP_CHAR;

        case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);
        }
    }

  /* Process remaining arguments.  If GNU extensions are enabled, process
     all arguments as input parameters.  If disabled, accept at most two
     arguments, the second of which is an output parameter.  */

  if (optind == argc)
    {

      /* No more argument simply means: read standard input.  */

      input_file_name = xmalloc (sizeof *input_file_name);
      file_line_count = xmalloc (sizeof *file_line_count);
      text_buffers =    xmalloc (sizeof *text_buffers);
      number_input_files = 1;
      input_file_name[0] = NULL;
    }
  else if (gnu_extensions)
    {
      number_input_files = argc - optind;
      input_file_name = xmalloc (number_input_files * sizeof *input_file_name);
      file_line_count = xmalloc (number_input_files * sizeof *file_line_count);
      text_buffers    = xmalloc (number_input_files * sizeof *text_buffers);

      for (file_index = 0; file_index < number_input_files; file_index++)
        {
          if (!*argv[optind] || STREQ (argv[optind], "-"))
            input_file_name[file_index] = NULL;
          else
            input_file_name[file_index] = argv[optind];
          optind++;
        }
    }
  else
    {

      /* There is one necessary input file.  */

      number_input_files = 1;
      input_file_name = xmalloc (sizeof *input_file_name);
      file_line_count = xmalloc (sizeof *file_line_count);
      text_buffers    = xmalloc (sizeof *text_buffers);
      if (!*argv[optind] || STREQ (argv[optind], "-"))
        input_file_name[0] = NULL;
      else
        input_file_name[0] = argv[optind];
      optind++;

      /* Redirect standard output, only if requested.  */

      if (optind < argc)
        {
          if (! freopen (argv[optind], "w", stdout))
            error (EXIT_FAILURE, errno, "%s", quotef (argv[optind]));
          optind++;
        }

      /* Diagnose any other argument as an error.  */

      if (optind < argc)
        {
          error (0, 0, _("extra operand %s"), quote (argv[optind]));
          usage (EXIT_FAILURE);
        }
    }

  /* If the output format has not been explicitly selected, choose dumb
     terminal format if GNU extensions are enabled, else 'roff' format.  */

  if (output_format == UNKNOWN_FORMAT)
    output_format = gnu_extensions ? DUMB_FORMAT : ROFF_FORMAT;

  /* Initialize the main tables.  */

  initialize_regex ();

  /* Read 'Break character' file, if any.  */

  if (break_file)
    digest_break_file (break_file);

  /* Read 'Ignore words' file and 'Only words' files, if any.  If any of
     these files is empty, reset the name of the file to NULL, to avoid
     unnecessary calls to search_table. */

  if (ignore_file)
    {
      digest_word_file (ignore_file, &ignore_table);
      if (ignore_table.length == 0)
        ignore_file = NULL;
    }

  if (only_file)
    {
      digest_word_file (only_file, &only_table);
      if (only_table.length == 0)
        only_file = NULL;
    }

  /* Prepare to study all the input files.  */

  number_of_occurs[0] = 0;
  total_line_count = 0;
  maximum_word_length = 0;
  reference_max_width = 0;

  for (file_index = 0; file_index < number_input_files; file_index++)
    {
      BLOCK *text_buffer = text_buffers + file_index;

      /* Read the file in core, then study it.  */

      swallow_file_in_memory (input_file_name[file_index], text_buffer);
      find_occurs_in_text (file_index);

      /* Maintain for each file how many lines has been read so far when its
         end is reached.  Incrementing the count first is a simple kludge to
         handle a possible incomplete line at end of file.  */

      total_line_count++;
      file_line_count[file_index] = total_line_count;
    }

  /* Do the output process phase.  */

  sort_found_occurs ();
  fix_output_parameters ();
  generate_all_output ();

  /* All done.  */

  return EXIT_SUCCESS;
}
