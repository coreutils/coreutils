/* Calculate the size of physical memory.
   Copyright 2000, 2001, 2003 Free Software Foundation, Inc.

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

/* Written by Paul Eggert and Jim Meyering.  */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include "physmem.h"

#if HAVE_UNISTD_H
# include <unistd.h>
#endif

#if HAVE_SYS_PSTAT_H
# include <sys/pstat.h>
#endif

#if HAVE_SYSGET && HAVE_SYS_SYSGET_H && HAVE_SYS_SYSINFO_H
# include <sys/types.h>
# include <sys/sysget.h>
# include <sys/sysinfo.h>

# if defined SGT_COOKIE_INIT && defined SGT_NODE_INFO && defined SGT_READ
#  define IRIX_SYSGET_TOTAL \
    do { double t; if (irix_sysget (&t, NULL) == 0) return t; } while (0)
#  define IRIX_SYSGET_AVAILABLE \
    do { double f; if (irix_sysget (NULL, &f) == 0) return f; } while (0)

/* If TOTAL is non-NULL, set *TOTAL to the number of bytes of physical memory.
   If AVAIL is non-NULL, set *AVAIL to the number of bytes of available memory.
   Return nonzero immediately if sysget fails.
   Otherwise, set the requested value(s) and return zero.  */
static int
irix_sysget (double *total, double *avail)
{
  nodeinfo_t buf;
  sgt_cookie_t cookie;

  SGT_COOKIE_INIT (&cookie);
  if (sysget (SGT_NODE_INFO, (char *) &buf, sizeof buf, SGT_READ, &cookie)
      != sizeof buf)
    return 1;

  if (total)
    *total = buf.totalmem;
  if (avail)
    *avail = buf.freemem;
  return 0;
}
# endif
#endif

#ifndef IRIX_SYSGET_TOTAL
# define IRIX_SYSGET_TOTAL
# define IRIX_SYSGET_AVAILABLE
#endif

/* Return the total amount of physical memory.  */
double
physmem_total (void)
{
#if defined _SC_PHYS_PAGES && defined _SC_PAGESIZE
  {
    double pages = sysconf (_SC_PHYS_PAGES);
    double pagesize = sysconf (_SC_PAGESIZE);
    if (0 <= pages && 0 <= pagesize)
      return pages * pagesize;
  }
#endif

#if HAVE_PSTAT_GETSTATIC
  {
    struct pst_static pss;
    if (0 <= pstat_getstatic (&pss, sizeof pss, 1, 0))
      {
	double pages = pss.physical_memory;
	double pagesize = pss.page_size;
	if (0 <= pages && 0 <= pagesize)
	  return pages * pagesize;
      }
  }
#endif

  IRIX_SYSGET_TOTAL;

  /* Guess 64 MB.  It's probably an older host, so guess small.  */
  return 64 * 1024 * 1024;
}

/* Return the amount of physical memory available.  */
double
physmem_available (void)
{
#if defined _SC_AVPHYS_PAGES && defined _SC_PAGESIZE
  {
    double pages = sysconf (_SC_AVPHYS_PAGES);
    double pagesize = sysconf (_SC_PAGESIZE);
    if (0 <= pages && 0 <= pagesize)
      return pages * pagesize;
  }
#endif

#if HAVE_PSTAT_GETSTATIC && HAVE_PSTAT_GETDYNAMIC
  {
    struct pst_static pss;
    struct pst_dynamic psd;
    if (0 <= pstat_getstatic (&pss, sizeof pss, 1, 0)
	&& 0 <= pstat_getdynamic (&psd, sizeof psd, 1, 0))
      {
	double pages = psd.psd_free;
	double pagesize = pss.page_size;
	if (0 <= pages && 0 <= pagesize)
	  return pages * pagesize;
      }
  }
#endif

  IRIX_SYSGET_AVAILABLE;

  /* Guess 25% of physical memory.  */
  return physmem_total () / 4;
}


#if DEBUG

# include <stdio.h>
# include <stdlib.h>

int
main ()
{
  printf ("%12.f %12.f\n", physmem_total (), physmem_available ());
  exit (0);
}

#endif /* DEBUG */

/*
Local Variables:
compile-command: "gcc -DDEBUG -DHAVE_CONFIG_H -I.. -g -O -Wall -W physmem.c"
End:
*/
