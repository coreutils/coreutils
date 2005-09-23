/* Get address information (partial implementation).
   Copyright (C) 1997, 2001, 2002, 2004, 2005 Free Software Foundation, Inc.
   Contributed by Simon Josefsson <simon@josefsson.org>.

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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "getaddrinfo.h"

#if HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif

/* Get calloc. */
#include <stdlib.h>

/* Get memcpy. */
#include <string.h>

#include <stdbool.h>

#include "gettext.h"
#define _(String) gettext (String)
#define N_(String) String

#include "strdup.h"

static inline bool
validate_family (int family)
{
  /* FIXME: Support more families. */
#if HAVE_IPV4
     if (family == PF_INET)
       return true;
#endif
#if HAVE_IPV6
     if (family == PF_INET6)
       return true;
#endif
     if (family == PF_UNSPEC)
       return true;
     return false;
}

/* Translate name of a service location and/or a service name to set of
   socket addresses. */
int
getaddrinfo (const char *restrict nodename,
	     const char *restrict servname,
	     const struct addrinfo *restrict hints,
	     struct addrinfo **restrict res)
{
  struct addrinfo *tmp;
  struct servent *se = NULL;
  struct hostent *he;
  void *storage;
  size_t size;
#if HAVE_IPV6
  struct v6_pair {
    struct addrinfo addrinfo;
    struct sockaddr_in6 sockaddr_in6;
  };
#endif
#if HAVE_IPV4
  struct v4_pair {
    struct addrinfo addrinfo;
    struct sockaddr_in sockaddr_in;
  };
#endif

  if (hints && (hints->ai_flags & ~AI_CANONNAME))
    /* FIXME: Support more flags. */
    return EAI_BADFLAGS;

  if (hints && !validate_family (hints->ai_family))
    return EAI_FAMILY;

  if (hints &&
      hints->ai_socktype != SOCK_STREAM && hints->ai_socktype != SOCK_DGRAM)
    /* FIXME: Support other socktype. */
    return EAI_SOCKTYPE; /* FIXME: Better return code? */

  if (!nodename)
    /* FIXME: Support server bind mode. */
    return EAI_NONAME;

  if (servname)
    {
      const char *proto =
	(hints && hints->ai_socktype == SOCK_DGRAM) ? "udp" : "tcp";

      /* FIXME: Use getservbyname_r if available. */
      se = getservbyname (servname, proto);

      if (!se)
	return EAI_SERVICE;
    }

  /* FIXME: Use gethostbyname_r if available. */
  he = gethostbyname (nodename);
  if (!he || he->h_addr_list[0] == NULL)
    return EAI_NONAME;

  switch (he->h_addrtype)
    {
#if HAVE_IPV6
    case PF_INET6:
      size = sizeof (struct v6_pair);
      break;
#endif

#if HAVE_IPV4
    case PF_INET:
      size = sizeof (struct v4_pair);
      break;
#endif

    default:
      return EAI_NODATA;
    }

  storage = calloc (1, size);
  if (!storage)
    return EAI_MEMORY;

  switch (he->h_addrtype)
    {
#if HAVE_IPV6
    case PF_INET6:
      {
	struct v6_pair *p = storage;
	struct sockaddr_in6 *sinp = &p->sockaddr_in6;
	tmp = &p->addrinfo;

	if (se)
	  sinp->sin6_port = se->s_port;

	if (he->h_length != sizeof (sinp->sin6_addr))
	  {
	    free (storage);
	    return EAI_SYSTEM; /* FIXME: Better return code?  Set errno? */
	  }

	memcpy (&sinp->sin6_addr, he->h_addr_list[0], sizeof sinp->sin6_addr);

	tmp->ai_addr = (struct sockaddr *) sinp;
	tmp->ai_addrlen = sizeof *sinp;
      }
      break;
#endif

#if HAVE_IPV4
    case PF_INET:
      {
	struct v4_pair *p = storage;
	struct sockaddr_in *sinp = &p->sockaddr_in;
	tmp = &p->addrinfo;

	if (se)
	  sinp->sin_port = se->s_port;

	if (he->h_length != sizeof (sinp->sin_addr))
	  {
	    free (storage);
	    return EAI_SYSTEM; /* FIXME: Better return code?  Set errno? */
	  }

	memcpy (&sinp->sin_addr, he->h_addr_list[0], sizeof sinp->sin_addr);

	tmp->ai_addr = (struct sockaddr *) sinp;
	tmp->ai_addrlen = sizeof *sinp;
      }
      break;
#endif

    default:
      free (storage);
      return EAI_NODATA;
    }

  if (hints && hints->ai_flags & AI_CANONNAME)
    {
      const char *cn;
      if (he->h_name)
	cn = he->h_name;
      else
	cn = nodename;

      tmp->ai_canonname = strdup (cn);
      if (!tmp->ai_canonname)
	{
	  free (storage);
	  return EAI_MEMORY;
	}
    }

  tmp->ai_protocol = (hints) ? hints->ai_protocol : 0;
  tmp->ai_socktype = (hints) ? hints->ai_socktype : 0;
  tmp->ai_addr->sa_family = he->h_addrtype;

  /* FIXME: If more than one address, create linked list of addrinfo's. */

  *res = tmp;

  return 0;
}

/* Free `addrinfo' structure AI including associated storage.  */
void
freeaddrinfo (struct addrinfo *ai)
{
  while (ai)
    {
      struct addrinfo *cur;

      cur = ai;
      ai = ai->ai_next;

      if (cur->ai_canonname) free (cur->ai_canonname);
      free (cur);
    }
}
