/* argmatch.h -- definitions and prototypes for argmatch.c
   Copyright (C) 1990, 1998 Free Software Foundation, Inc.

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

/* Written by David MacKenzie <djm@ai.mit.edu>
   Modified by Akim Demaille <demaille@inf.enst.fr> */

#ifndef _ARGMATCH_H_
# define _ARGMATCH_H_ 1

# if HAVE_CONFIG_H
#  include <config.h>
# endif

# include <sys/types.h>

# ifndef PARAMS
#  if PROTOTYPES || (defined (__STDC__) && __STDC__)
#   define PARAMS(args) args
#  else
#   define PARAMS(args) ()
#  endif  /* GCC.  */
# endif  /* Not PARAMS.  */

/* Return the index of the element of ARGLIST (NULL terminated) that
   matches with ARG.  If VALLIST is not NULL, then use it to resolve
   false ambiguities (i.e., different matches of ARG but corresponding
   to the same values in VALLIST).  */

int argmatch
	PARAMS ((const char *arg, const char *const *arglist,
		 const char *vallist, size_t valsize));
int argcasematch
	PARAMS ((const char *arg, const char *const *arglist,
		 const char *vallist, size_t valsize));

# define ARGMATCH(ARG,ARGLIST,VALLIST) \
  argmatch (ARG, ARGLIST, (const char *) VALLIST, sizeof (*VALLIST))

# define ARGCASEMATCH(ARG,ARGLIST,VALLIST) \
  argcasematch (ARG, ARGLIST, (const char *) VALLIST, sizeof (*VALLIST))



/* Report on stderr why argmatch failed.  Report correct values. */

void argmatch_invalid
	PARAMS ((const char *kind, const char *value, int problem));

/* Left for compatibility with the old name invalid_arg */

# define invalid_arg(KIND,VALUE,PROBLEM) \
	argmatch_invalid (KIND, VALUE, PROBLEM)



/* Report on stderr the list of possible arguments.  */

void argmatch_valid
	PARAMS ((const char *const *arglist,
		 const char *vallist, size_t valsize));

# define ARGMATCH_VALID(ARGLIST,VALLIST) \
  valid_args (ARGLIST, (const char *) VALLIST, sizeof (*VALLIST))


/* Returns matches, or, upon error, report explanatory message and
   exit.  */

int __xargmatch_internal
	PARAMS ((const char *kind,
		 const char *arg, const char *const *arglist,
		 const char *vallist, size_t valsize,
		 int sensitive));

# define XARGMATCH(KIND,ARG,ARGLIST,VALLIST) \
  VALLIST [__xargmatch_internal (KIND, ARG, ARGLIST, \
                        (const char *) VALLIST, sizeof (*VALLIST), 1)]

# define XARGCASEMATCH(KIND,ARG,ARGLIST,VALLIST) \
  VALLIST [__xargmatch_internal (KIND, ARG, ARGLIST, \
                        (const char *) VALLIST, sizeof (*VALLIST), 0)]



/* Convert a value into a corresponding argument. */

const char * argmatch_to_argument
	PARAMS ((char * value, const char *const *arglist,
		 const char *vallist, size_t valsize));

# define ARGMATCH_TO_ARGUMENT(VALUE,ARGLIST,VALLIST) 	\
   argmatch_to_argument ((char *) &VALUE, ARGLIST, 	\
			 (const char *) VALLIST, sizeof (*VALLIST))

#endif /* _ARGMATCH_H_ */
