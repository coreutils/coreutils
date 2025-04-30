/* wc.c - print newline, word, and byte counts for each file.
   Modified to include --max-line-length-with-content option.
   Original Author: Free Software Foundation, Inc.
   Modified by: [Your Name]
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <getopt.h>

#define CHAR_MAX 255

static bool print_lines = false;
static bool print_words = false;
static bool print_bytes = false;
static bool print_chars = false;
static bool print_max_line_length = false;
static bool print_max_line_and_content = false;

static struct option const long_options[] =
{
  {"lines", no_argument, NULL, 'l'},
  {"words", no_argument, NULL, 'w'},
  {"bytes", no_argument, NULL, 'c'},
  {"chars", no_argument, NULL, 'm'},
  {"max-line-length", no_argument, NULL, 'L'},
  {"max-line-length-with-content", no_argument, NULL, CHAR_MAX + 1},
  {NULL, 0, NULL, 0}
};

void print_usage(const char *prog_name)
{
  fprintf(stderr, "Usage: %s [OPTION]... [FILE]...\n", prog_name);
  fprintf(stderr, "Print newline, word, and byte counts for each FILE.\n");
  fprintf(stderr, "  -l, --lines            print the newline counts\n");
  fprintf(stderr, "  -w, --words            print the word counts\n");
  fprintf(stderr, "  -c, --bytes            print the byte counts\n");
  fprintf(stderr, "  -m, --chars            print the character counts\n");
  fprintf(stderr, "  -L, --max-line-length  print the length of the longest line\n");
  fprintf(stderr, "      --max-line-length-with-content  print length, line number and content of the longest line\n");
}

void process_file(FILE *fp, const char *filename)
{
  size_t lines = 0, words = 0, bytes = 0, chars = 0;
  size_t max_line_length = 0;
  size_t max_line_number = 0;
  char *longest_line = NULL;
  size_t longest_line_size = 0;

  char *line = NULL;
  size_t len = 0;
  ssize_t read;

  size_t current_line_number = 0;
  while ((read = getline(&line, &len, fp)) != -1)
  {
    bytes += read;
    chars += read; // Simplified; for multibyte support, use mbrtowc
    lines++;
    current_line_number++;

    // Word count
    bool in_word = false;
    for (ssize_t i = 0; i < read; i++)
    {
      if (line[i] == ' ' || line[i] == '\t' || line[i] == '\n')
      {
        if (in_word)
        {
          words++;
          in_word = false;
        }
      }
      else
      {
        in_word = true;
      }
    }
    if (in_word)
      words++;

    // Max line length and content
    if ((size_t)read > max_line_length)
    {
      max_line_length = read;
      max_line_number = current_line_number;
      free(longest_line);
      longest_line = strdup(line);
      longest_line_size = read;
    }
  }

  free(line);

  // Output
  if (print_lines)
    printf("%7zu ", lines);
  if (print_words)
    printf("%7zu ", words);
  if (print_bytes)
    printf("%7zu ", bytes);
  if (print_chars)
    printf("%7zu ", chars);
  if (print_max_line_length)
  {
    printf("%7zu", max_line_length);
    if (print_max_line_and_content && longest_line)
    {
      printf(" %zu %s", max_line_number, longest_line);
    }
    else
    {
      printf(" ");
    }
  }
  printf("%s\n", filename);

  free(longest_line);
}

int main(int argc, char **argv)
{
  int c;
  while ((c = getopt_long(argc, argv, "lwcLm", long_options, NULL)) != -1)
  {
    switch (c)
    {
      case 'l':
        print_lines = true;
        break;
      case 'w':
        print_words = true;
        break;
      case 'c':
        print_bytes = true;
        break;
      case 'm':
        print_chars = true;
        break;
      case 'L':
        print_max_line_length = true;
        break;
      case CHAR_MAX + 1:
        print_max_line_length = true;
        print_max_line_and_content = true;
        break;
      default:
        print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }
  }

  if (optind == argc)
  {
    // No files provided; read from stdin
    process_file(stdin, "");
  }
  else
  {
    for (int i = optind; i < argc; i++)
    {
      FILE *fp = fopen(argv[i], "r");
      if (!fp)
      {
        perror(argv[i]);
        continue;
      }
      process_file(fp, argv[i]);
      fclose(fp);
    }
  }

  return 0;
}
