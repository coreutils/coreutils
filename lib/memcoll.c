/* Locale-specific memory comparison.
   Copyright 1999 Free Software Foundation, Inc.

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

/* Contributed by Paul Eggert <eggert@twinsun.com>.  */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#ifndef __GNUC__
# ifdef HAVE_ALLOCA_H
#  include <alloca.h>
# else
#  ifdef _AIX
 #  pragma alloca
#  else
#   ifdef _WIN32
#    include <malloc.h>
#    include <io.h>
#   else
#    ifndef alloca
char *alloca ();
#    endif
#   endif
#  endif
# endif
#endif

#if HAVE_STRING_H
# include <string.h>
#endif

/* Compare S1 (with length S1LEN) and S2 (with length S2LEN) according
   to the LC_COLLATE locale.  S1 and S2 do not overlap, but may be
   adjacent.  Temporarily modify the bytes after S1 and S2, but
   restore their original contents before returning.  */
int
memcoll (char *s1, size_t s1len, char *s2, size_t s2len)
{
  int diff;
  char n1;
  char n2;

  /* We will temporarily set the bytes after S1 and S2 to zero, so if
     S1 and S2 are adjacent, compare to a temporary copy of the
     earlier, to avoid temporarily stomping on the later.  */

  if (s1 + s1len == s2)
    {
      char *s2copy = alloca (s2len + 1);
      memcpy (s2copy, s2, s2len);
      s2 = s2copy;
    }

  if (s2 + s2len == s1)
    {
      char *s1copy = alloca (s1len + 1);
      memcpy (s1copy, s1, s1len);
      s1 = s1copy;
    }

  n1 = s1[s1len];  s1[s1len++] = '\0';
  n2 = s2[s2len];  s2[s2len++] = '\0';

  while (! (diff = strcoll (s1, s2)))
    {
      /* strcoll found no difference, but perhaps it was fooled by NUL
	 characters in the data.  Work around this problem by advancing
	 past the NUL chars.  */
      size_t size1 = strlen (s1) + 1;
      size_t size2 = strlen (s2) + 1;
      s1 += size1;
      s2 += size2;
      s1len -= size1;
      s2len -= size2;

      if (s1len == 0)
	{
	  if (s2len != 0)
	    diff = -1;
	  break;
	}
      else if (s2len == 0)
	{
	  diff = 1;
	  break;
	}
    }

  s1[s1len - 1] = n1;
  s2[s2len - 1] = n2;

  return diff;
}
