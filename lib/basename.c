/* basename.c -- return the last element in a path */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#ifndef FILESYSTEM_PREFIX_LEN
# define FILESYSTEM_PREFIX_LEN(f) 0
#endif

#ifndef ISSLASH
# define ISSLASH(c) ((c) == '/')
#endif

/* In general, we can't use the builtin `basename' function if available,
   since it has different meanings in different environments.
   In some environments the builtin `basename' modifies its argument.  */

char *
base_name (name)
     char const *name;
{
  char const *base = name += FILESYSTEM_PREFIX_LEN (name);

  for (; *name; name++)
    if (ISSLASH (*name))
      base = name + 1;

  return (char *) base;
}

#ifdef STDC_HEADERS
# include <stdlib.h>
#else
char *malloc ();
#endif

char *
base_name_strip_trailing_slashes (name)
     char const *name;
{
  char const *end_p = name += FILESYSTEM_PREFIX_LEN (name);
  char const *first, *p;
  char *base;
  int length;

  /* Make END_P point to the byte after the last non-slash character
     in NAME if one exists.  */
  for (p = name; *p; p++)
    if (!ISSLASH (*p))
      end_p = p + 1;

  if (end_p == name)
    {
      first = end_p;
    }
  else
    {
      first = end_p - 1;
      while (first > name && !ISSLASH (*(first - 1)))
	--first;
    }

  length = end_p - first;
  base = (char *) malloc (length + 1);
  if (base == 0)
    return 0;

  memcpy (base, first, length);
  base[length] = '\0';

  return base;
}

#ifdef TEST
# include <assert.h>
# include <stdlib.h>

# define CHECK(a,b) assert (strcmp (base_name_strip_trailing_slashes(a), b) \
			    == 0)

int
main ()
{
  CHECK ("a", "a");
  CHECK ("ab", "ab");
  CHECK ("ab/c", "c");
  CHECK ("/ab/c", "c");
  CHECK ("/ab/c/", "c");
  CHECK ("/ab/c////", "c");
  CHECK ("/", "");
  CHECK ("////", "");
  CHECK ("////a", "a");
  CHECK ("//a//", "a");
  CHECK ("/a", "a");
  exit (0);
}
#endif
