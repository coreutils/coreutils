/* tsort - topological sort.
   Copyright (C) 1998-2002 Free Software Foundation, Inc.

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

/* Written by Mark Kettenis <kettenis@phys.uva.nl>.  */

/* The topological sort is done according to Algorithm T (Topological
   sort) in Donald E. Knuth, The Art of Computer Programming, Volume
   1/Fundamental Algorithms, page 262.  */

#include <config.h>

#include <stdio.h>
#include <assert.h>
#include <getopt.h>
#include <sys/types.h>

#include "system.h"
#include "closeout.h"
#include "long-options.h"
#include "error.h"
#include "readtokens.h"

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "tsort"

#define AUTHORS "Mark Kettenis"

/* Token delimiters when reading from a file.  */
#define DELIM " \t\n"

/* Members of the list of successors.  */
struct successor
{
  struct item *suc;
  struct successor *next;
};

/* Each string is held in core as the head of a list of successors.  */
struct item
{
  const char *str;
  struct item *left, *right;
  int balance;
  int count;
  struct item *qlink;
  struct successor *top;
};

/* The name this program was run with. */
char *program_name;

/* Nonzero if any of the input files are the standard input. */
static int have_read_stdin;

/* The error code to return to the system. */
static int exit_status;

/* The head of the sorted list.  */
static struct item *head = NULL;

/* The tail of the list of `zeros', strings that have no predecessors.  */
static struct item *zeros = NULL;

/* Used for loop detection.  */
static struct item *loop = NULL;

/* The number of strings to sort.  */
static int n_strings = 0;

static struct option const long_options[] =
{
  { NULL, 0, NULL, 0}
};

void
usage (int status)
{
  if (status != 0)
    fprintf (stderr, _("Try `%s --help' for more information.\n"),
	     program_name);
  else
    {
      printf (_("\
Usage: %s [OPTION] [FILE]\n\
Write totally ordered list consistent with the partial ordering in FILE.\n\
With no FILE, or when FILE is -, read standard input.\n\
\n\
"), program_name);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      printf (_("\nReport bugs to <%s>.\n"), PACKAGE_BUGREPORT);
    }

  exit (status == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}

/* Create a new item/node for STR.  */
static struct item *
new_item (const char *str)
{
  struct item *k = xmalloc (sizeof (struct item));

  k->str = (str ? xstrdup (str): NULL);
  k->left = k->right = NULL;
  k->balance = 0;

  /* T1. Initialize (COUNT[k] <- 0 and TOP[k] <- ^).  */
  k->count = 0;
  k->qlink = NULL;
  k->top = NULL;

  return k;
}

/* Search binary tree rooted at *ROOT for STR.  Allocate a new tree if
   *ROOT is NULL.  Insert a node/item for STR if not found.  Return
   the node/item found/created for STR.

   This is done according to Algorithm A (Balanced tree search and
   insertion) in Donald E. Knuth, The Art of Computer Programming,
   Volume 3/Searching and Sorting, pages 455--457.  */

static struct item *
search_item (struct item *root, const char *str)
{
  struct item *p, *q, *r, *s, *t;
  int a;

  assert (root);

  /* Make sure the tree is not empty, since that is what the algorithm
     below expects.  */
  if (root->right == NULL)
    return (root->right = new_item (str));

  /* A1. Initialize.  */
  t = root;
  s = p = root->right;

  for (;;)
    {
      /* A2. Compare.  */
      a = strcmp (str, p->str);
      if (a == 0)
	return p;

      /* A3 & A4.  Move left & right.  */
      if (a < 0)
	q = p->left;
      else
	q = p->right;

      if (q == NULL)
	{
	  /* A5. Insert.  */
	  q = new_item (str);

	  /* A3 & A4.  (continued).  */
	  if (a < 0)
	    p->left = q;
	  else
	    p->right = q;

	  /* A6. Adjust balance factors.  */
	  assert (!STREQ (str, s->str));
	  if (strcmp (str, s->str) < 0)
	    {
	      r = p = s->left;
	      a = -1;
	    }
	  else
	    {
	      r = p = s->right;
	      a = 1;
	    }

	  while (p != q)
	    {
	      assert (!STREQ (str, p->str));
	      if (strcmp (str, p->str) < 0)
		{
		  p->balance = -1;
		  p = p->left;
		}
	      else
		{
		  p->balance = 1;
		  p = p->right;
		}
	    }

	  /* A7. Balancing act.  */
	  if (s->balance == 0 || s->balance == -a)
	    {
	      s->balance += a;
	      return q;
	    }

	  if (r->balance == a)
	    {
	      /* A8. Single Rotation.  */
	      p = r;
	      if (a < 0)
		{
		  s->left = r->right;
		  r->right = s;
		}
	      else
		{
		  s->right = r->left;
		  r->left = s;
		}
	      s->balance = r->balance = 0;
	    }
	  else
	    {
	      /* A9. Double rotation.  */
	      if (a < 0)
		{
		  p = r->right;
		  r->right = p->left;
		  p->left = r;
		  s->left = p->right;
		  p->right = s;
		}
	      else
		{
		  p = r->left;
		  r->left = p->right;
		  p->right = r;
		  s->right = p->left;
		  p->left = s;
		}

	      s->balance = 0;
	      r->balance = 0;
	      if (p->balance == a)
		s->balance = -a;
	      else if (p->balance == -a)
		r->balance = a;
	      p->balance = 0;
	    }

	  /* A10. Finishing touch.  */
	  if (s == t->right)
	    t->right = p;
	  else
	    t->left = p;

	  return q;
	}

      /* A3 & A4.  (continued).  */
      if (q->balance)
	{
	  t = p;
	  s = q;
	}

      p = q;
    }

  /* NOTREACHED */
}

/* Record the fact that J precedes K.  */

static void
record_relation (struct item *j, struct item *k)
{
  struct successor *p;

  if (!STREQ (j->str, k->str))
    {
      k->count++;
      p = xmalloc (sizeof (struct successor));
      p->suc = k;
      p->next = j->top;
      j->top = p;
    }
}

static int
count_items (struct item *unused ATTRIBUTE_UNUSED)
{
  n_strings++;
  return 0;
}

static int
scan_zeros (struct item *k)
{
  /* Ignore strings that have already been printed.  */
  if (k->count == 0 && k->str)
    {
      if (head == NULL)
	head = k;
      else
	zeros->qlink = k;

      zeros = k;
    }

  return 0;
}

/* Try to detect the loop.  If we have detected that K is part of a
   loop, print the loop on standard error, remove a relation to break
   the loop, and return non-zero.

   The loop detection strategy is as follows: Realise that what we're
   dealing with is essentially a directed graph.  If we find an item
   that is part of a graph that contains a cycle we traverse the graph
   in backwards direction.  In general there is no unique way to do
   this, but that is no problem.  If we encounter an item that we have
   encountered before, we know that we've found a cycle.  All we have
   to do now is retrace our steps, printing out the items until we
   encounter that item again.  (This is not necessarily the item that
   we started from originally.)  Since the order in which the items
   are stored in the tree is not related to the specified partial
   ordering, we may need to walk the tree several times before the
   loop has completely been constructed.  If the loop was found, the
   global variable LOOP will be NULL.  */

static int
detect_loop (struct item *k)
{
  if (k->count > 0)
    {
      /* K does not have to be part of a cycle.  It is however part of
	 a graph that contains a cycle.  */

      if (loop == NULL)
	/* Start traversing the graph at K.  */
	loop = k;
      else
	{
	  struct successor **p = &k->top;

	  while (*p)
	    {
	      if ((*p)->suc == loop)
		{
		  if (k->qlink)
		    {
		      /* We have found a loop.  Retrace our steps.  */
		      while (loop)
			{
			  struct item *tmp = loop->qlink;

			  fprintf (stderr, "%s: %s\n", program_name,
				   loop->str);

			  /* Until we encounter K again.  */
			  if (loop == k)
			    {
			      /* Remove relation.  */
			      (*p)->suc->count--;
			      *p = (*p)->next;
			      break;
			    }

			  /* Tidy things up since we might have to
                             detect another loop.  */
			  loop->qlink = NULL;
			  loop = tmp;
			}

		      while (loop)
			{
			  struct item *tmp = loop->qlink;

			  loop->qlink = NULL;
			  loop = tmp;
			}

		      /* Since we have found the loop, stop walking
                         the tree.  */
		      return 1;
		    }
		  else
		    {
		      k->qlink = loop;
		      loop = k;
		      break;
		    }
		}

	      p = &(*p)->next;
	    }
	}
    }

  return 0;
}

/* Recurse (sub)tree rooted at ROOT, calling ACTION for each node.
   Stop when ACTION returns non-zero.  */

static int
recurse_tree (struct item *root, int (*action) (struct item *))
{
  if (root->left == NULL && root->right == NULL)
    return (*action) (root);
  else
    {
      if (root->left != NULL)
	if (recurse_tree (root->left, action))
	  return 1;
      if ((*action) (root))
	return 1;
      if (root->right != NULL)
	if (recurse_tree (root->right, action))
	  return 1;
    }

  return 0;
}

/* Walk the tree specified by the head ROOT, calling ACTION for
   each node.  */

static void
walk_tree (struct item *root, int (*action) (struct item *))
{
  if (root->right)
    recurse_tree (root->right, action);
}

/* Do a topological sort on FILE.   */

static void
tsort (const char *file)
{
  struct item *root;
  struct item *j = NULL;
  struct item *k = NULL;
  register FILE *fp;
  token_buffer tokenbuffer;

  /* Intialize the head of the tree will hold the strings we're sorting.  */
  root = new_item (NULL);

  if (STREQ (file, "-"))
    {
      fp = stdin;
      have_read_stdin = 1;
    }
  else
    {
      fp = fopen (file, "r");
      if (fp == NULL)
	error (EXIT_FAILURE, errno, "%s", file);
    }

  init_tokenbuffer (&tokenbuffer);

  while (1)
    {
      long int len;

      /* T2. Next Relation.  */
      len = readtoken (fp, DELIM, sizeof (DELIM) - 1, &tokenbuffer);
      if (len < 0)
	break;

      assert (len != 0);

      k = search_item (root, tokenbuffer.buffer);
      if (j)
	{
	  /* T3. Record the relation.  */
	  record_relation (j, k);
	  k = NULL;
	}

      j = k;
    }

  /* T1. Initialize (N <- n).  */
  walk_tree (root, count_items);

  while (n_strings > 0)
    {
      /* T4. Scan for zeros.  */
      walk_tree (root, scan_zeros);

      while (head)
	{
	  struct successor *p = head->top;

	  /* T5. Output front of queue.  */
	  printf ("%s\n", head->str);
	  head->str = NULL;	/* Avoid printing the same string twice.  */
	  n_strings--;

	  /* T6. Erase relations.  */
	  while (p)
	    {
	      p->suc->count--;
	      if (p->suc->count == 0)
		{
		  zeros->qlink = p->suc;
		  zeros = p->suc;
		}

	      p = p->next;
	    }

	  /* T7. Remove from queue.  */
	  head = head->qlink;
	}

      /* T8.  End of process.  */
      assert (n_strings >= 0);
      if (n_strings > 0)
	{
	  /* The input contains a loop.  */
	  error (0, 0, _("%s: input contains a loop:"),
		 (have_read_stdin ? "-" : file));
	  exit_status = 1;

	  /* Print the loop and remove a relation to break it.  */
	  do
	    walk_tree (root, detect_loop);
	  while (loop);
	}
    }
}

int
main (int argc, char **argv)
{
  int opt;

  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  exit_status = 0;

  parse_long_options (argc, argv, PROGRAM_NAME, PACKAGE, VERSION,
		      AUTHORS, usage);

  while ((opt = getopt_long (argc, argv, "", long_options, NULL)) != -1)
    switch (opt)
      {
      case 0:			/* long option */
	break;
      default:
	usage (EXIT_FAILURE);
      }

  have_read_stdin = 0;

  if (optind + 1 < argc)
    {
      error (0, 0, _("only one argument may be specified"));
      usage (EXIT_FAILURE);
    }

  if (optind < argc)
    tsort (argv[optind]);
  else
    tsort ("-");

  if (have_read_stdin && fclose (stdin) == EOF)
    error (EXIT_FAILURE, errno, _("standard input"));

  exit (exit_status == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}
