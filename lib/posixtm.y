/* Parse dates for touch.
   Copyright (C) 1989, 1990, 1991 Free Software Foundation Inc.

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

/* Written by Jim Kingdon and David MacKenzie. */
%{

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/* The following block of alloca-related preprocessor directives is here
   solely to allow compilation by non GNU-C compilers of the C parser
   produced from this file by old versions of bison.  Newer versions of
   bison include a block similar to this one in bison.simple.  */

#ifdef __GNUC__
#define alloca __builtin_alloca
#else
#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#else
#ifdef _AIX
 #pragma alloca
#else
void *alloca ();
#endif
#endif
#endif

#include <stdio.h>
#include <sys/types.h>

#ifdef TM_IN_SYS_TIME
#include <sys/time.h>
#else
#include <time.h>
#endif

/* Some old versions of bison generate parsers that use bcopy.
   That loses on systems that don't provide the function, so we have
   to redefine it here.  */
#if !defined (HAVE_BCOPY) && defined (HAVE_MEMCPY) && !defined (bcopy)
#define bcopy(from, to, len) memcpy ((to), (from), (len))
#endif

#define YYDEBUG 1

/* Lexical analyzer's current scan position in the input string. */
static char *curpos;

/* The return value. */
static struct tm t;

time_t mktime ();

/* Remap normal yacc parser interface names (yyparse, yylex, yyerror, etc),
   as well as gratuitiously global symbol names, so we can have multiple
   yacc generated parsers in the same program.  Note that these are only
   the variables produced by yacc.  If other parser generators (bison,
   byacc, etc) produce additional global names that conflict at link time,
   then those parser generators need to be fixed instead of adding those
   names to this list. */

#define yymaxdepth pt_maxdepth
#define yyparse pt_parse
#define yylex   pt_lex
#define yyerror pt_error
#define yylval  pt_lval
#define yychar  pt_char
#define yydebug pt_debug
#define yypact  pt_pact
#define yyr1    pt_r1
#define yyr2    pt_r2
#define yydef   pt_def
#define yychk   pt_chk
#define yypgo   pt_pgo
#define yyact   pt_act
#define yyexca  pt_exca
#define yyerrflag pt_errflag
#define yynerrs pt_nerrs
#define yyps    pt_ps
#define yypv    pt_pv
#define yys     pt_s
#define yy_yys  pt_yys
#define yystate pt_state
#define yytmp   pt_tmp
#define yyv     pt_v
#define yy_yyv  pt_yyv
#define yyval   pt_val
#define yylloc  pt_lloc
#define yyreds  pt_reds          /* With YYDEBUG defined */
#define yytoks  pt_toks          /* With YYDEBUG defined */
#define yylhs   pt_yylhs
#define yylen   pt_yylen
#define yydefred pt_yydefred
#define yydgoto pt_yydgoto
#define yysindex pt_yysindex
#define yyrindex pt_yyrindex
#define yygindex pt_yygindex
#define yytable  pt_yytable
#define yycheck  pt_yycheck

static int yylex ();
static int yyerror ();

%}

%token DIGIT

%%
date :
       digitpair /* month */
       digitpair /* day */
       digitpair /* hours */
       digitpair /* minutes */
       year
       seconds {
	         if ($1 >= 1 && $1 <= 12)
		   t.tm_mon = $1 - 1;
		 else {
		   YYABORT;
		 }
		 if ($2 >= 1 && $2 <= 31)
		   t.tm_mday = $2;
		 else {
		   YYABORT;
		 }
		 if ($3 >= 0 && $3 <= 23)
		   t.tm_hour = $3;
		 else {
		   YYABORT;
		 }
		 if ($4 >= 0 && $4 <= 59)
		   t.tm_min = $4;
		 else {
		   YYABORT;
		 }
	       }

year : digitpair {
                   t.tm_year = $1;
		   /* Deduce the century based on the year.
		      See POSIX.2 section 4.63.3.  */
		   if ($1 <= 68)
		     t.tm_year += 100;
		 }
    | digitpair digitpair {
                            t.tm_year = $1 * 100 + $2;
			    if (t.tm_year < 1900) {
			      YYABORT;
			    } else
			      t.tm_year -= 1900;
			  }
    | /* empty */ {
                    time_t now;
		    struct tm *tmp;

                    /* Use current year.  */
                    time (&now);
		    tmp = localtime (&now);
		    t.tm_year = tmp->tm_year;
		  }
    ;

seconds : /* empty */ {
                        t.tm_sec = 0;
		      }
        | '.' digitpair {
	                  if ($2 >= 0 && $2 <= 61)
			    t.tm_sec = $2;
			  else {
			    YYABORT;
			  }
			}
        ;

digitpair : DIGIT DIGIT {
                          $$ = $1 * 10 + $2;
			}
          ;
%%
static int
yylex ()
{
  char ch = *curpos++;

  if (ch >= '0' && ch <= '9')
    {
      yylval = ch - '0';
      return DIGIT;
    }
  else if (ch == '.' || ch == 0)
    return ch;
  else
    return '?';			/* Cause an error.  */
}

static int
yyerror ()
{
  return 0;
}

/* Parse a POSIX-style date and return it, or (time_t)-1 for an error.  */

time_t
posixtime (s)
     char *s;
{
  curpos = s;
  /* Let mktime decide whether it is daylight savings time.  */
  t.tm_isdst = -1;
  if (yyparse ())
    return (time_t)-1;
  else
    return mktime (&t);
}

/* Parse a POSIX-style date and return it, or NULL for an error.  */

struct tm *
posixtm (s)
     char *s;
{
  if (posixtime (s) == -1)
    return NULL;
  return &t;
}
