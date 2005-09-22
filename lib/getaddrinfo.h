/* Get address information.
   Copyright (C) 1996-2002, 2003, 2004, 2005 Free Software Foundation, Inc.
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

#ifndef GETADDRINFO_H
# define GETADDRINFO_H

/* Get getaddrinfo declarations, if available.  Also get 'socklen_t',
   and 'struct sockaddr' via sys/types.h which are used below. */
# include <sys/types.h>
# include <sys/socket.h>
# include <netdb.h>

# if !HAVE_GETADDRINFO

/* Structure to contain information about address of a service provider.  */
struct addrinfo
{
  int ai_flags;			/* Input flags.  */
  int ai_family;		/* Protocol family for socket.  */
  int ai_socktype;		/* Socket type.  */
  int ai_protocol;		/* Protocol for socket.  */
  socklen_t ai_addrlen;		/* Length of socket address.  */
  struct sockaddr *ai_addr;	/* Socket address for socket.  */
  char *ai_canonname;		/* Canonical name for service location.  */
  struct addrinfo *ai_next;	/* Pointer to next in list.  */
};

/* Possible values for `ai_flags' field in `addrinfo' structure.  */
# define AI_PASSIVE	0x0001	/* Socket address is intended for `bind'.  */
# define AI_CANONNAME	0x0002	/* Request for canonical name.  */
# define AI_NUMERICHOST	0x0004	/* Don't use name resolution.  */
# define AI_V4MAPPED	0x0008	/* IPv4 mapped addresses are acceptable.  */
# define AI_ALL		0x0010	/* Return IPv4 mapped and IPv6 addresses.  */
# define AI_ADDRCONFIG	0x0020	/* Use configuration of this host to choose
				   returned address type..  */

/* Error values for `getaddrinfo' function.  */
# define EAI_BADFLAGS	  -1	/* Invalid value for `ai_flags' field.  */
# define EAI_NONAME	  -2	/* NAME or SERVICE is unknown.  */
# define EAI_AGAIN	  -3	/* Temporary failure in name resolution.  */
# define EAI_FAIL	  -4	/* Non-recoverable failure in name res.  */
# define EAI_NODATA	  -5	/* No address associated with NAME.  */
# define EAI_FAMILY	  -6	/* `ai_family' not supported.  */
# define EAI_SOCKTYPE	  -7	/* `ai_socktype' not supported.  */
# define EAI_SERVICE	  -8	/* SERVICE not supported for `ai_socktype'.  */
# define EAI_ADDRFAMILY	  -9	/* Address family for NAME not supported.  */
# define EAI_MEMORY	  -10	/* Memory allocation failure.  */
# define EAI_SYSTEM	  -11	/* System error returned in `errno'.  */
# define EAI_OVERFLOW	  -12	/* Argument buffer overflow.  */
# ifdef __USE_GNU
#  define EAI_INPROGRESS  -100	/* Processing request in progress.  */
#  define EAI_CANCELED	  -101	/* Request canceled.  */
#  define EAI_NOTCANCELED -102	/* Request not canceled.  */
#  define EAI_ALLDONE	  -103	/* All requests done.  */
#  define EAI_INTR	  -104	/* Interrupted by a signal.  */
#  define EAI_IDN_ENCODE  -105	/* IDN encoding failed.  */
# endif

/* Translate name of a service location and/or a service name to set of
   socket addresses.
   For more details, see the POSIX:2001 specification
   <http://www.opengroup.org/susv3xsh/getaddrinfo.html>.  */
extern int getaddrinfo (const char *restrict nodename,
			const char *restrict servname,
			const struct addrinfo *restrict hints,
			struct addrinfo **restrict res);

/* Free `addrinfo' structure AI including associated storage.
   For more details, see the POSIX:2001 specification
   <http://www.opengroup.org/susv3xsh/getaddrinfo.html>.  */
extern void freeaddrinfo (struct addrinfo *ai);

/* Convert error return from getaddrinfo() to a string.
   For more details, see the POSIX:2001 specification
   <http://www.opengroup.org/susv3xsh/gai_strerror.html>.  */
extern const char *gai_strerror (int ecode);

# endif /* !HAVE_GETADDRINFO */

#endif /* GETADDRINFO_H */
