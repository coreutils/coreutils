/* Host name canonicalization

   Copyright (C) 1995, 1999, 2000 Free Software Foundation, Inc.

   Written by Miles Bader <miles@gnu.ai.mit.edu>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2, or (at
   your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <sys/types.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
# include <string.h>
#endif
#ifdef HAVE_NETDB_H
# include <netdb.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif

#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif
#ifdef HAVE_ARPA_INET_H
# include <arpa/inet.h>
#endif

/* Returns the canonical hostname associated with HOST (allocated in a static
   buffer), or 0 if it can't be determined.  */
char *
canon_host (const char *host)
{
#ifdef HAVE_GETHOSTBYNAME
  struct hostent *he = gethostbyname (host);

  if (he)
    {
# ifdef HAVE_GETHOSTBYADDR
      char *addr = 0;

      /* Try and get an ascii version of the numeric host address.  */
      switch (he->h_addrtype)
	{
#  ifdef HAVE_INET_NTOA
	case AF_INET:
	  addr = inet_ntoa (*(struct in_addr *) he->h_addr);
	  break;
#  endif /* HAVE_INET_NTOA */
	}

      if (addr && strcmp (he->h_name, addr) == 0)
	{
	  /* gethostbyname has returned a string representation of the IP
	     address, for example, "127.0.0.1".  So now, look up the host
	     name via the address.  Although it may seem reasonable to look
	     up the host name via the address, we must not pass `he->h_addr'
	     directly to gethostbyaddr because on some systems he->h_addr
	     is located in a static library buffer that is reused in the
	     gethostbyaddr call.  Make a copy and use that instead.  */
	  char *h_addr_copy = (char *) malloc (he->h_length);
	  if (h_addr_copy == NULL)
	    he = NULL;
	  else
	    {
	      memcpy (h_addr_copy, he->h_addr, he->h_length);
	      he = gethostbyaddr (h_addr_copy, he->h_length, he->h_addrtype);
	      free (h_addr_copy);
	    }
	}
# endif /* HAVE_GETHOSTBYADDR */

      if (he)
	return (char *) (he->h_name);
    }
#endif /* HAVE_GETHOSTBYNAME */
  return 0;
}

#ifdef TEST_CANON_HOST
int
main (int argc, char **argv)
{
  int i;
  for (i = 1; i < argc; i++)
    {
      char *s = canon_host (argv[i]);
      printf ("%s: %s\n", argv[i], (s ? s : "<undef>"));
    }
  exit (0);
}
#endif /* TEST_CANON_HOST */
