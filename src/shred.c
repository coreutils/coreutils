/* shred.c - overwrite files and devices to make it harder to recover data

   Copyright (C) 1999-2003 Free Software Foundation, Inc.
   Copyright (C) 1997, 1998, 1999 Colin Plumb.

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
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

   Written by Colin Plumb.  */

/* TODO:
   - use consistent non-capitalization in error messages
   - add standard GNU copyleft comment

  - Add -r/-R/--recursive
  - Add -i/--interactive
  - Reserve -d
  - Add -L
  - Deal with the amazing variety of gettimeofday() implementation bugs.
    (Some systems use a one-arg form; still others insist that the timezone
    either be NULL or be non-NULL.  Whee.)
  - Add an unlink-all option to emulate rm.
 */

/*
 * Do a more secure overwrite of given files or devices, to make it harder
 * for even very expensive hardware probing to recover the data.
 *
 * Although this process is also known as "wiping", I prefer the longer
 * name both because I think it is more evocative of what is happening and
 * because a longer name conveys a more appropriate sense of deliberateness.
 *
 * For the theory behind this, see "Secure Deletion of Data from Magnetic
 * and Solid-State Memory", on line at
 * http://www.cs.auckland.ac.nz/~pgut001/pubs/secure_del.html
 *
 * Just for the record, reversing one or two passes of disk overwrite
 * is not terribly difficult with hardware help.  Hook up a good-quality
 * digitizing oscilloscope to the output of the head preamplifier and copy
 * the high-res digitized data to a computer for some off-line analysis.
 * Read the "current" data and average all the pulses together to get an
 * "average" pulse on the disk.  Subtract this average pulse from all of
 * the actual pulses and you can clearly see the "echo" of the previous
 * data on the disk.
 *
 * Real hard drives have to balance the cost of the media, the head,
 * and the read circuitry.  They use better-quality media than absolutely
 * necessary to limit the cost of the read circuitry.  By throwing that
 * assumption out, and the assumption that you want the data processed
 * as fast as the hard drive can spin, you can do better.
 *
 * If asked to wipe a file, this also unlinks it, renaming it to in a
 * clever way to try to leave no trace of the original filename.
 *
 * The ISAAC code still bears some resemblance to the code written
 * by Bob Jenkins, but he permits pretty unlimited use.
 *
 * This was inspired by a desire to improve on some code titled:
 * Wipe V1.0-- Overwrite and delete files.  S. 2/3/96
 * but I've rewritten everything here so completely that no trace of
 * the original remains.
 *
 * Thanks to:
 * Bob Jenkins, for his good RNG work and patience with the FSF copyright
 * paperwork.
 * Jim Meyering, for his work merging this into the GNU fileutils while
 * still letting me feel a sense of ownership and pride.  Getting me to
 * tolerate the GNU brace style was quite a feat of diplomacy.
 * Paul Eggert, for lots of useful discussion and code.  I disagree with
 * an awful lot of his suggestions, but they're disagreements worth having.
 *
 * Things to think about:
 * - Security: Is there any risk to the race
 *   between overwriting and unlinking a file?  Will it do anything
 *   drastically bad if told to attack a named pipe or socket?
 */

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "shred"

#define AUTHORS "Colin Plumb"

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <getopt.h>
#include <stdio.h>
#include <assert.h>
#include <setjmp.h>
#include <signal.h>
#include <sys/types.h>

#include "system.h"
#include "xstrtol.h"
#include "closeout.h"
#include "error.h"
#include "human.h"
#include "inttostr.h"
#include "quotearg.h"		/* For quotearg_colon */
#include "quote.h"		/* For quotearg_colon */

#ifndef O_NOCTTY
# define O_NOCTTY 0  /* This is a very optional frill */
#endif

#define DEFAULT_PASSES 25	/* Default */

/* How many seconds to wait before checking whether to output another
   verbose output line.  */
#define VERBOSE_UPDATE 5

struct Options
{
  int force;		/* -f flag: chmod files if necessary */
  size_t n_iterations;	/* -n flag: Number of iterations */
  off_t size;		/* -s flag: size of file */
  int remove_file;	/* -u flag: remove file after shredding */
  int verbose;		/* -v flag: Print progress */
  int exact;		/* -x flag: Do not round up file size */
  int zero_fill;	/* -z flag: Add a final zero pass */
};

static struct option const long_opts[] =
{
  {"exact", no_argument, NULL, 'x'},
  {"force", no_argument, NULL, 'f'},
  {"iterations", required_argument, NULL, 'n'},
  {"size", required_argument, NULL, 's'},
  {"remove", no_argument, NULL, 'u'},
  {"verbose", no_argument, NULL, 'v'},
  {"zero", no_argument, NULL, 'z'},
  {GETOPT_HELP_OPTION_DECL},
  {GETOPT_VERSION_OPTION_DECL},
  {NULL, 0, NULL, 0}
};

/* Global variable for error printing purposes */
char const *program_name; /* Initialized before any possible use */

void
usage (int status)
{
  if (status != 0)
    fprintf (stderr, _("Try `%s --help' for more information.\n"),
	     program_name);
  else
    {
      printf (_("Usage: %s [OPTIONS] FILE [...]\n"), program_name);
      fputs (_("\
Overwrite the specified FILE(s) repeatedly, in order to make it harder\n\
for even very expensive hardware probing to recover the data.\n\
\n\
"), stdout);
      fputs (_("\
Mandatory arguments to long options are mandatory for short options too.\n\
"), stdout);
      printf (_("\
  -f, --force    change permissions to allow writing if necessary\n\
  -n, --iterations=N  Overwrite N times instead of the default (%d)\n\
  -s, --size=N   shred this many bytes (suffixes like K, M, G accepted)\n\
"), DEFAULT_PASSES);
      fputs (_("\
  -u, --remove   truncate and remove file after overwriting\n\
  -v, --verbose  show progress\n\
  -x, --exact    do not round file sizes up to the next full block;\n\
                   this is the default for non-regular files\n\
  -z, --zero     add a final overwrite with zeros to hide shredding\n\
  -              shred standard output\n\
"), stdout);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      fputs (_("\
\n\
Delete FILE(s) if --remove (-u) is specified.  The default is not to remove\n\
the files because it is common to operate on device files like /dev/hda,\n\
and those files usually should not be removed.  When operating on regular\n\
files, most people use the --remove option.\n\
\n\
"), stdout);
      fputs (_("\
CAUTION: Note that shred relies on a very important assumption:\n\
that the filesystem overwrites data in place.  This is the traditional\n\
way to do things, but many modern filesystem designs do not satisfy this\n\
assumption.  The following are examples of filesystems on which shred is\n\
not effective:\n\
\n\
"), stdout);
      fputs (_("\
* log-structured or journaled filesystems, such as those supplied with\n\
  AIX and Solaris (and JFS, ReiserFS, XFS, Ext3, etc.)\n\
\n\
* filesystems that write redundant data and carry on even if some writes\n\
  fail, such as RAID-based filesystems\n\
\n\
* filesystems that make snapshots, such as Network Appliance's NFS server\n\
\n\
"), stdout);
      fputs (_("\
* filesystems that cache in temporary locations, such as NFS\n\
  version 3 clients\n\
\n\
* compressed filesystems\n\
\n\
In addition, file system backups and remote mirrors may contain copies\n\
of the file that cannot be removed, and that will allow a shredded file\n\
to be recovered later.\n\
"), stdout);
      printf (_("\nReport bugs to <%s>.\n"), PACKAGE_BUGREPORT);
    }
  exit (status);
}

#if ! HAVE_FDATASYNC
# define fdatasync(fd) -1
#endif

/*
 * --------------------------------------------------------------------
 *     Bob Jenkins' cryptographic random number generator, ISAAC.
 *     Hacked by Colin Plumb.
 *
 * We need a source of random numbers for some of the overwrite data.
 * Cryptographically secure is desirable, but it's not life-or-death
 * so I can be a little bit experimental in the choice of RNGs here.
 *
 * This generator is based somewhat on RC4, but has analysis
 * (http://ourworld.compuserve.com/homepages/bob_jenkins/randomnu.htm)
 * pointing to it actually being better.  I like it because it's nice
 * and fast, and because the author did good work analyzing it.
 * --------------------------------------------------------------------
 */

#if defined __STDC__ && __STDC__
# define UINT_MAX_32_BITS 4294967295U
#else
# define UINT_MAX_32_BITS 0xFFFFFFFF
#endif

#if ULONG_MAX == UINT_MAX_32_BITS
typedef unsigned long word32;
#else
# if UINT_MAX == UINT_MAX_32_BITS
typedef unsigned word32;
# else
#  if USHRT_MAX == UINT_MAX_32_BITS
typedef unsigned short word32;
#  else
#   if UCHAR_MAX == UINT_MAX_32_BITS
typedef unsigned char word32;
#   else
     "No 32-bit type available!"
#   endif
#  endif
# endif
#endif

/* Size of the state tables to use.  (You may change ISAAC_LOG) */
#define ISAAC_LOG 8
#define ISAAC_WORDS (1 << ISAAC_LOG)
#define ISAAC_BYTES (ISAAC_WORDS * sizeof (word32))

/* RNG state variables */
struct isaac_state
  {
    word32 mm[ISAAC_WORDS];	/* Main state array */
    word32 iv[8];		/* Seeding initial vector */
    word32 a, b, c;		/* Extra index variables */
  };

/* This index operation is more efficient on many processors */
#define ind(mm, x) \
  (* (word32 *) ((char *) (mm) + ((x) & (ISAAC_WORDS - 1) * sizeof (word32))))

/*
 * The central step.  This uses two temporaries, x and y.  mm is the
 * whole state array, while m is a pointer to the current word.  off is
 * the offset from m to the word ISAAC_WORDS/2 words away in the mm array,
 * i.e. +/- ISAAC_WORDS/2.
 */
#define isaac_step(mix, a, b, mm, m, off, r) \
( \
  a = ((a) ^ (mix)) + (m)[off], \
  x = *(m), \
  *(m) = y = ind (mm, x) + (a) + (b), \
  *(r) = b = ind (mm, (y) >> ISAAC_LOG) + x \
)

/*
 * Refill the entire R array, and update S.
 */
static void
isaac_refill (struct isaac_state *s, word32 r[/* ISAAC_WORDS */])
{
  register word32 a, b;		/* Caches of a and b */
  register word32 x, y;		/* Temps needed by isaac_step macro */
  register word32 *m = s->mm;	/* Pointer into state array */

  a = s->a;
  b = s->b + (++s->c);

  do
    {
      isaac_step (a << 13, a, b, s->mm, m, ISAAC_WORDS / 2, r);
      isaac_step (a >> 6, a, b, s->mm, m + 1, ISAAC_WORDS / 2, r + 1);
      isaac_step (a << 2, a, b, s->mm, m + 2, ISAAC_WORDS / 2, r + 2);
      isaac_step (a >> 16, a, b, s->mm, m + 3, ISAAC_WORDS / 2, r + 3);
      r += 4;
    }
  while ((m += 4) < s->mm + ISAAC_WORDS / 2);
  do
    {
      isaac_step (a << 13, a, b, s->mm, m, -ISAAC_WORDS / 2, r);
      isaac_step (a >> 6, a, b, s->mm, m + 1, -ISAAC_WORDS / 2, r + 1);
      isaac_step (a << 2, a, b, s->mm, m + 2, -ISAAC_WORDS / 2, r + 2);
      isaac_step (a >> 16, a, b, s->mm, m + 3, -ISAAC_WORDS / 2, r + 3);
      r += 4;
    }
  while ((m += 4) < s->mm + ISAAC_WORDS);
  s->a = a;
  s->b = b;
}

/*
 * The basic seed-scrambling step for initialization, based on Bob
 * Jenkins' 256-bit hash.
 */
#define mix(a,b,c,d,e,f,g,h) \
   (       a ^= b << 11, d += a, \
   b += c, b ^= c >>  2, e += b, \
   c += d, c ^= d <<  8, f += c, \
   d += e, d ^= e >> 16, g += d, \
   e += f, e ^= f << 10, h += e, \
   f += g, f ^= g >>  4, a += f, \
   g += h, g ^= h <<  8, b += g, \
   h += a, h ^= a >>  9, c += h, \
   a += b                        )

/* The basic ISAAC initialization pass.  */
static void
isaac_mix (struct isaac_state *s, word32 const seed[/* ISAAC_WORDS */])
{
  int i;
  word32 a = s->iv[0];
  word32 b = s->iv[1];
  word32 c = s->iv[2];
  word32 d = s->iv[3];
  word32 e = s->iv[4];
  word32 f = s->iv[5];
  word32 g = s->iv[6];
  word32 h = s->iv[7];

  for (i = 0; i < ISAAC_WORDS; i += 8)
    {
      a += seed[i];
      b += seed[i + 1];
      c += seed[i + 2];
      d += seed[i + 3];
      e += seed[i + 4];
      f += seed[i + 5];
      g += seed[i + 6];
      h += seed[i + 7];

      mix (a, b, c, d, e, f, g, h);

      s->mm[i] = a;
      s->mm[i + 1] = b;
      s->mm[i + 2] = c;
      s->mm[i + 3] = d;
      s->mm[i + 4] = e;
      s->mm[i + 5] = f;
      s->mm[i + 6] = g;
      s->mm[i + 7] = h;
    }

  s->iv[0] = a;
  s->iv[1] = b;
  s->iv[2] = c;
  s->iv[3] = d;
  s->iv[4] = e;
  s->iv[5] = f;
  s->iv[6] = g;
  s->iv[7] = h;
}

#if 0 /* Provided for reference only; not used in this code */
/*
 * Initialize the ISAAC RNG with the given seed material.
 * Its size MUST be a multiple of ISAAC_BYTES, and may be
 * stored in the s->mm array.
 *
 * This is a generalization of the original ISAAC initialization code
 * to support larger seed sizes.  For seed sizes of 0 and ISAAC_BYTES,
 * it is identical.
 */
static void
isaac_init (struct isaac_state *s, word32 const *seed, size_t seedsize)
{
  static word32 const iv[8] =
  {
    0x1367df5a, 0x95d90059, 0xc3163e4b, 0x0f421ad8,
    0xd92a4a78, 0xa51a3c49, 0xc4efea1b, 0x30609119};
  int i;

# if 0
  /* The initialization of iv is a precomputed form of: */
  for (i = 0; i < 7; i++)
    iv[i] = 0x9e3779b9;		/* the golden ratio */
  for (i = 0; i < 4; ++i)	/* scramble it */
    mix (iv[0], iv[1], iv[2], iv[3], iv[4], iv[5], iv[6], iv[7]);
# endif
  s->a = s->b = s->c = 0;

  for (i = 0; i < 8; i++)
    s->iv[i] = iv[i];

  if (seedsize)
    {
      /* First pass (as in reference ISAAC code) */
      isaac_mix (s, seed);
      /* Second and subsequent passes (extension to ISAAC) */
      while (seedsize -= ISAAC_BYTES)
	{
	  seed += ISAAC_WORDS;
	  for (i = 0; i < ISAAC_WORDS; i++)
	    s->mm[i] += seed[i];
	  isaac_mix (s, s->mm);
	}
    }
  else
    {
      /* The no seed case (as in reference ISAAC code) */
      for (i = 0; i < ISAAC_WORDS; i++)
	s->mm[i] = 0;
    }

  /* Final pass */
  isaac_mix (s, s->mm);
}
#endif

/* Start seeding an ISAAC structire */
static void
isaac_seed_start (struct isaac_state *s)
{
  static word32 const iv[8] =
    {
      0x1367df5a, 0x95d90059, 0xc3163e4b, 0x0f421ad8,
      0xd92a4a78, 0xa51a3c49, 0xc4efea1b, 0x30609119
    };
  int i;

#if 0
  /* The initialization of iv is a precomputed form of: */
  for (i = 0; i < 7; i++)
    iv[i] = 0x9e3779b9;		/* the golden ratio */
  for (i = 0; i < 4; ++i)	/* scramble it */
    mix (iv[0], iv[1], iv[2], iv[3], iv[4], iv[5], iv[6], iv[7]);
#endif
  for (i = 0; i < 8; i++)
    s->iv[i] = iv[i];
  /* We could initialize s->mm to zero, but why bother? */

  /* s->c gets used for a data pointer during the seeding phase */
  s->a = s->b = s->c = 0;
}

/* Add a buffer of seed material */
static void
isaac_seed_data (struct isaac_state *s, void const *buf, size_t size)
{
  unsigned char *p;
  size_t avail;
  size_t i;

  avail = sizeof s->mm - (size_t) s->c;	/* s->c is used as a write pointer */

  /* Do any full buffers that are necessary */
  while (size > avail)
    {
      p = (unsigned char *) s->mm + s->c;
      for (i = 0; i < avail; i++)
	p[i] ^= ((unsigned char const *) buf)[i];
      buf = (char const *) buf + avail;
      size -= avail;
      isaac_mix (s, s->mm);
      s->c = 0;
      avail = sizeof s->mm;
    }

  /* And the final partial block */
  p = (unsigned char *) s->mm + s->c;
  for (i = 0; i < size; i++)
    p[i] ^= ((unsigned char const *) buf)[i];
  s->c = (word32) size;
}


/* End of seeding phase; get everything ready to produce output. */
static void
isaac_seed_finish (struct isaac_state *s)
{
  isaac_mix (s, s->mm);
  isaac_mix (s, s->mm);
  /* Now reinitialize c to start things off right */
  s->c = 0;
}
#define ISAAC_SEED(s,x) isaac_seed_data (s, &(x), sizeof (x))


#if __GNUC__ >= 2 && (__i386__ || __alpha__)
/*
 * Many processors have very-high-resolution timer registers,
 * The timer registers can be made inaccessible, so we have to deal with the
 * possibility of SIGILL while we're working.
 */
static jmp_buf env;
static RETSIGTYPE
sigill_handler (int signum)
{
  (void) signum;
  longjmp (env, 1);  /* Trivial, just return an indication that it happened */
}

/* FIXME: find a better way.
   This signal-handling code may well end up being ripped out eventually.
   An example of how fragile it is, on an i586-sco-sysv5uw7.0.1 system, with
   gcc-2.95.3pl1, the "rdtsc" instruction causes a segmentation violation.
   So now, the code catches SIGSEGV.  It'd probably be better to remove all
   of that mess and find a better source of random data.  Patches welcome.  */

static void
isaac_seed_machdep (struct isaac_state *s)
{
  RETSIGTYPE (*old_handler[2]) (int);

  /* This is how one does try/except in C */
  old_handler[0] = signal (SIGILL, sigill_handler);
  old_handler[1] = signal (SIGSEGV, sigill_handler);
  if (setjmp (env))  /* ANSI: Must be entire controlling expression */
    {
      signal (SIGILL, old_handler[0]);
      signal (SIGSEGV, old_handler[1]);
    }
  else
    {
# if __i386__
      word32 t[2];
      __asm__ __volatile__ ("rdtsc" : "=a" (t[0]), "=d" (t[1]));
# endif
# if __alpha__
      unsigned long t;
      __asm__ __volatile__ ("rpcc %0" : "=r" (t));
# endif
# if _ARCH_PPC
      /* Code not used because this instruction is available only on first-
	 generation PPCs and evokes a SIGBUS on some Linux 2.4 kernels.  */
      word32 t;
      __asm__ __volatile__ ("mfspr %0,22" : "=r" (t));
# endif
# if __mips
      /* Code not used because this is not accessible from userland */
      word32 t;
      __asm__ __volatile__ ("mfc0\t%0,$9" : "=r" (t));
# endif
# if __sparc__
      /* This doesn't compile on all platforms yet.  How to fix? */
      unsigned long t;
      __asm__ __volatile__ ("rd	%%tick, %0" : "=r" (t));
# endif
     signal (SIGILL, old_handler[0]);
     signal (SIGSEGV, old_handler[1]);
     isaac_seed_data (s, &t, sizeof t);
  }
}

#else /* !(__i386__ || __alpha__) */

/* Do-nothing stub */
# define isaac_seed_machdep(s) (void) 0

#endif /* !(__i386__ || __alpha__) */


/*
 * Get seed material.  16 bytes (128 bits) is plenty, but if we have
 * /dev/urandom, we get 32 bytes = 256 bits for complete overkill.
 */
static void
isaac_seed (struct isaac_state *s)
{
  isaac_seed_start (s);

  { pid_t t = getpid ();   ISAAC_SEED (s, t); }
  { pid_t t = getppid ();  ISAAC_SEED (s, t); }
  { uid_t t = getuid ();   ISAAC_SEED (s, t); }
  { gid_t t = getgid ();   ISAAC_SEED (s, t); }

  {
#if HAVE_GETHRTIME
    hrtime_t t = gethrtime ();
    ISAAC_SEED (s, t);
#else
# if HAVE_CLOCK_GETTIME		/* POSIX ns-resolution */
    struct timespec t;
    clock_gettime (CLOCK_REALTIME, &t);
# else
#  if HAVE_GETTIMEOFDAY
    struct timeval t;
    gettimeofday (&t, (struct timezone *) 0);
#  else
    time_t t;
    t = time (NULL);
#  endif
# endif
#endif
    ISAAC_SEED (s, t);
  }

  isaac_seed_machdep (s);

  {
    char buf[32];
    int fd = open ("/dev/urandom", O_RDONLY | O_NOCTTY);
    if (fd >= 0)
      {
	read (fd, buf, 32);
	close (fd);
	isaac_seed_data (s, buf, 32);
      }
    else
      {
	fd = open ("/dev/random", O_RDONLY | O_NONBLOCK | O_NOCTTY);
	if (fd >= 0)
	  {
	    /* /dev/random is more precious, so use less */
	    read (fd, buf, 16);
	    close (fd);
	    isaac_seed_data (s, buf, 16);
	  }
      }
  }

  isaac_seed_finish (s);
}

/* Single-word RNG built on top of ISAAC */
struct irand_state
{
  word32 r[ISAAC_WORDS];
  unsigned numleft;
  struct isaac_state *s;
};

static void
irand_init (struct irand_state *r, struct isaac_state *s)
{
  r->numleft = 0;
  r->s = s;
}

/*
 * We take from the end of the block deliberately, so if we need
 * only a small number of values, we choose the final ones which are
 * marginally better mixed than the initial ones.
 */
static word32
irand32 (struct irand_state *r)
{
  if (!r->numleft)
    {
      isaac_refill (r->s, r->r);
      r->numleft = ISAAC_WORDS;
    }
  return r->r[--r->numleft];
}

/*
 * Return a uniformly distributed random number between 0 and n,
 * inclusive.  Thus, the result is modulo n+1.
 *
 * Theory of operation: as x steps through every possible 32-bit number,
 * x % n takes each value at least 2^32 / n times (rounded down), but
 * the values less than 2^32 % n are taken one additional time.  Thus,
 * x % n is not perfectly uniform.  To fix this, the values of x less
 * than 2^32 % n are disallowed, and if the RNG produces one, we ask
 * for a new value.
 */
static word32
irand_mod (struct irand_state *r, word32 n)
{
  word32 x;
  word32 lim;

  if (!++n)
    return irand32 (r);

  lim = -n % n;			/* == (2**32-n) % n == 2**32 % n */
  do
    {
      x = irand32 (r);
    }
  while (x < lim);
  return x % n;
}

/*
 * Fill a buffer with a fixed pattern.
 *
 * The buffer must be at least 3 bytes long, even if
 * size is less.  Larger sizes are filled exactly.
 */
static void
fillpattern (int type, unsigned char *r, size_t size)
{
  size_t i;
  unsigned bits = type & 0xfff;

  bits |= bits << 12;
  ((unsigned char *) r)[0] = (bits >> 4) & 255;
  ((unsigned char *) r)[1] = (bits >> 8) & 255;
  ((unsigned char *) r)[2] = bits & 255;
  for (i = 3; i < size / 2; i *= 2)
    memcpy ((char *) r + i, (char *) r, i);
  if (i < size)
    memcpy ((char *) r + i, (char *) r, size - i);

  /* Invert the first bit of every 512-byte sector. */
  if (type & 0x1000)
    for (i = 0; i < size; i += 512)
      r[i] ^= 0x80;
}

/*
 * Fill a buffer, R (of size SIZE_MAX), with random data.
 * SIZE is rounded UP to a multiple of ISAAC_BYTES.
 */
static void
fillrand (struct isaac_state *s, word32 *r, size_t size_max, size_t size)
{
  size = (size + ISAAC_BYTES - 1) / ISAAC_BYTES;
  assert (size <= size_max);

  while (size--)
    {
      isaac_refill (s, r);
      r += ISAAC_WORDS;
    }
}

/*
 * Generate a 6-character (+ nul) pass name string
 * FIXME: allow translation of "random".
 */
#define PASS_NAME_SIZE 7
static void
passname (unsigned char const *data, char name[PASS_NAME_SIZE])
{
  if (data)
    sprintf (name, "%02x%02x%02x", data[0], data[1], data[2]);
  else
    memcpy (name, "random", PASS_NAME_SIZE);
}

/*
 * Do pass number k of n, writing "size" bytes of the given pattern "type"
 * to the file descriptor fd.   Qname, k and n are passed in only for verbose
 * progress message purposes.  If n == 0, no progress messages are printed.
 *
 * If *sizep == -1, the size is unknown, and it will be filled in as soon
 * as writing fails.
 */
static int
dopass (int fd, char const *qname, off_t *sizep, int type,
	struct isaac_state *s, unsigned long k, unsigned long n)
{
  off_t size = *sizep;
  off_t offset;			/* Current file posiiton */
  time_t thresh IF_LINT (= 0);	/* Time to maybe print next status update */
  time_t now = 0;		/* Current time */
  size_t lim;			/* Amount of data to try writing */
  size_t soff;			/* Offset into buffer for next write */
  ssize_t ssize;		/* Return value from write */
#if ISAAC_WORDS > 1024
  word32 r[ISAAC_WORDS * 3];	/* Multiple of 4K and of pattern size */
#else
  word32 r[1024 * 3];		/* Multiple of 4K and of pattern size */
#endif
  char pass_string[PASS_NAME_SIZE];	/* Name of current pass */

  /* Printable previous offset into the file */
  char previous_offset_buf[LONGEST_HUMAN_READABLE + 1];
  char const *previous_human_offset IF_LINT (= 0);

  if (lseek (fd, (off_t) 0, SEEK_SET) == -1)
    {
      error (0, errno, _("%s: cannot rewind"), qname);
      return -1;
    }

  /* Constant fill patterns need only be set up once. */
  if (type >= 0)
    {
      lim = sizeof r;
      if ((off_t) lim > size && size != -1)
	{
	  lim = (size_t) size;
	}
      fillpattern (type, (unsigned char *) r, lim);
      passname ((unsigned char *) r, pass_string);
    }
  else
    {
      passname (0, pass_string);
    }

  /* Set position if first status update */
  if (n)
    {
      error (0, 0, _("%s: pass %lu/%lu (%s)..."), qname, k, n, pass_string);
      thresh = time (NULL) + VERBOSE_UPDATE;
      previous_human_offset = "";
    }

  offset = 0;
  for (;;)
    {
      /* How much to write this time? */
      lim = sizeof r;
      if ((off_t) lim > size - offset && size != -1)
	{
	  if (size < offset)
	    break;
	  lim = (size_t) (size - offset);
	  if (!lim)
	    break;
	}
      if (type < 0)
	fillrand (s, r, sizeof r, lim);
      /* Loop to retry partial writes. */
      for (soff = 0; soff < lim; soff += ssize)
	{
	  ssize = write (fd, (char *) r + soff, lim - soff);
	  if (ssize <= 0)
	    {
	      if ((ssize == 0 || errno == ENOSPC)
		  && size == -1)
		{
		  /* Ah, we have found the end of the file */
		  *sizep = size = offset + soff;
		  break;
		}
	      else
		{
		  int errnum = errno;
		  char buf[INT_BUFSIZE_BOUND (uintmax_t)];
		  error (0, errnum, _("%s: error writing at offset %s"),
			 qname, umaxtostr ((uintmax_t) offset + soff, buf));
		  /*
		   * I sometimes use shred on bad media, before throwing it
		   * out.  Thus, I don't want it to give up on bad blocks.
		   * This code assumes 512-byte blocks and tries to skip
		   * over them.  It works because lim is always a multiple
		   * of 512, except at the end.
		   */
		  if (errnum == EIO && soff % 512 == 0 && lim >= soff + 512
		      && size != -1)
		    {
		      if (lseek (fd, (off_t) (offset + soff + 512), SEEK_SET)
			  != -1)
			{
			  soff += 512;
			  continue;
			}
		      error (0, errno, "%s: lseek", qname);
		    }
		  return -1;
		}
	    }
	}

      /* Okay, we have written "soff" bytes. */

      if (offset + soff < offset)
	{
	  error (0, 0, _("%s: file too large"), qname);
	  return -1;
	}

      offset += soff;

      /* Time to print progress? */
      if (n
	  && ((offset == size && *previous_human_offset)
	      || thresh <= (now = time (NULL))))
	{
	  char offset_buf[LONGEST_HUMAN_READABLE + 1];
	  char size_buf[LONGEST_HUMAN_READABLE + 1];
	  int human_progress_opts = (human_autoscale | human_SI
				     | human_base_1024 | human_B);
	  char const *human_offset
	    = human_readable (offset, offset_buf,
			      human_floor | human_progress_opts, 1, 1);

	  if (offset == size
	      || !STREQ (previous_human_offset, human_offset))
	    {
	      if (size == -1)
		error (0, 0, _("%s: pass %lu/%lu (%s)...%s"),
		       qname, k, n, pass_string, human_offset);
	      else
		{
		  uintmax_t off = offset;
		  int percent = (size == 0
				 ? 100
				 : (off <= TYPE_MAXIMUM (uintmax_t) / 100
				    ? off * 100 / size
				    : off / (size / 100)));
		  char const *human_size
		    = human_readable (size, size_buf,
				      human_ceiling | human_progress_opts,
				      1, 1);
		  if (offset == size)
		    human_offset = human_size;
		  error (0, 0, _("%s: pass %lu/%lu (%s)...%s/%s %d%%"),
			 qname, k, n, pass_string, human_offset, human_size,
			 percent);
		}

	      strcpy (previous_offset_buf, human_offset);
	      previous_human_offset = previous_offset_buf;
	      thresh = now + VERBOSE_UPDATE;

	      /*
	       * Force periodic syncs to keep displayed progress accurate
	       * FIXME: Should these be present even if -v is not enabled,
	       * to keep the buffer cache from filling with dirty pages?
	       * It's a common problem with programs that do lots of writes,
	       * like mkfs.
	       */
	      if (fdatasync (fd) < 0 && fsync (fd) < 0)
		{
		  error (0, errno, "%s: fsync", qname);
		  return -1;
		}
	    }
	}
    }

  /* Force what we just wrote to hit the media. */
  if (fdatasync (fd) < 0 && fsync (fd) < 0)
    {
      error (0, errno, "%s: fsync", qname);
      return -1;
    }
  return 0;
}

/*
 * The passes start and end with a random pass, and the passes in between
 * are done in random order.  The idea is to deprive someone trying to
 * reverse the process of knowledge of the overwrite patterns, so they
 * have the additional step of figuring out what was done to the disk
 * before they can try to reverse or cancel it.
 *
 * First, all possible 1-bit patterns.  There are two of them.
 * Then, all possible 2-bit patterns.  There are four, but the two
 * which are also 1-bit patterns can be omitted.
 * Then, all possible 3-bit patterns.  Likewise, 8-2 = 6.
 * Then, all possible 4-bit patterns.  16-4 = 12.
 *
 * The basic passes are:
 * 1-bit: 0x000, 0xFFF
 * 2-bit: 0x555, 0xAAA
 * 3-bit: 0x249, 0x492, 0x924, 0x6DB, 0xB6D, 0xDB6 (+ 1-bit)
 *        100100100100         110110110110
 *           9   2   4            D   B   6
 * 4-bit: 0x111, 0x222, 0x333, 0x444, 0x666, 0x777,
 *        0x888, 0x999, 0xBBB, 0xCCC, 0xDDD, 0xEEE (+ 1-bit, 2-bit)
 * Adding three random passes at the beginning, middle and end
 * produces the default 25-pass structure.
 *
 * The next extension would be to 5-bit and 6-bit patterns.
 * There are 30 uncovered 5-bit patterns and 64-8-2 = 46 uncovered
 * 6-bit patterns, so they would increase the time required
 * significantly.  4-bit patterns are enough for most purposes.
 *
 * The main gotcha is that this would require a trickier encoding,
 * since lcm(2,3,4) = 12 bits is easy to fit into an int, but
 * lcm(2,3,4,5) = 60 bits is not.
 *
 * One extension that is included is to complement the first bit in each
 * 512-byte block, to alter the phase of the encoded data in the more
 * complex encodings.  This doesn't apply to MFM, so the 1-bit patterns
 * are considered part of the 3-bit ones and the 2-bit patterns are
 * considered part of the 4-bit patterns.
 *
 *
 * How does the generalization to variable numbers of passes work?
 *
 * Here's how...
 * Have an ordered list of groups of passes.  Each group is a set.
 * Take as many groups as will fit, plus a random subset of the
 * last partial group, and place them into the passes list.
 * Then shuffle the passes list into random order and use that.
 *
 * One extra detail: if we can't include a large enough fraction of the
 * last group to be interesting, then just substitute random passes.
 *
 * If you want more passes than the entire list of groups can
 * provide, just start repeating from the beginning of the list.
 */
static int const
  patterns[] =
{
  -2,				/* 2 random passes */
  2, 0x000, 0xFFF,		/* 1-bit */
  2, 0x555, 0xAAA,		/* 2-bit */
  -1,				/* 1 random pass */
  6, 0x249, 0x492, 0x6DB, 0x924, 0xB6D, 0xDB6,	/* 3-bit */
  12, 0x111, 0x222, 0x333, 0x444, 0x666, 0x777,
  0x888, 0x999, 0xBBB, 0xCCC, 0xDDD, 0xEEE,	/* 4-bit */
  -1,				/* 1 random pass */
	/* The following patterns have the frst bit per block flipped */
  8, 0x1000, 0x1249, 0x1492, 0x16DB, 0x1924, 0x1B6D, 0x1DB6, 0x1FFF,
  14, 0x1111, 0x1222, 0x1333, 0x1444, 0x1555, 0x1666, 0x1777,
  0x1888, 0x1999, 0x1AAA, 0x1BBB, 0x1CCC, 0x1DDD, 0x1EEE,
  -1,				/* 1 random pass */
  0				/* End */
};

/*
 * Generate a random wiping pass pattern with num passes.
 * This is a two-stage process.  First, the passes to include
 * are chosen, and then they are shuffled into the desired
 * order.
 */
static void
genpattern (int *dest, size_t num, struct isaac_state *s)
{
  struct irand_state r;
  size_t randpasses;
  int const *p;
  int *d;
  size_t n;
  size_t accum, top, swap;
  int k;

  if (!num)
    return;

  irand_init (&r, s);

  /* Stage 1: choose the passes to use */
  p = patterns;
  randpasses = 0;
  d = dest;			/* Destination for generated pass list */
  n = num;			/* Passes remaining to fill */

  for (;;)
    {
      k = *p++;			/* Block descriptor word */
      if (!k)
	{			/* Loop back to the beginning */
	  p = patterns;
	}
      else if (k < 0)
	{			/* -k random passes */
	  k = -k;
	  if ((size_t) k >= n)
	    {
	      randpasses += n;
	      n = 0;
	      break;
	    }
	  randpasses += k;
	  n -= k;
	}
      else if ((size_t) k <= n)
	{			/* Full block of patterns */
	  memcpy (d, p, k * sizeof (int));
	  p += k;
	  d += k;
	  n -= k;
	}
      else if (n < 2 || 3 * n < (size_t) k)
	{			/* Finish with random */
	  randpasses += n;
	  break;
	}
      else
	{			/* Pad out with k of the n available */
	  do
	    {
	      if (n == (size_t) k-- || irand_mod (&r, k) < n)
		{
		  *d++ = *p;
		  n--;
		}
	      p++;
	    }
	  while (n);
	  break;
	}
    }
  top = num - randpasses;	/* Top of initialized data */
  /* assert (d == dest+top); */

  /*
   * We now have fixed patterns in the dest buffer up to
   * "top", and we need to scramble them, with "randpasses"
   * random passes evenly spaced among them.
   *
   * We want one at the beginning, one at the end, and
   * evenly spaced in between.  To do this, we basically
   * use Bresenham's line draw (a.k.a DDA) algorithm
   * to draw a line with slope (randpasses-1)/(num-1).
   * (We use a positive accumulator and count down to
   * do this.)
   *
   * So for each desired output value, we do the following:
   * - If it should be a random pass, copy the pass type
   *   to top++, out of the way of the other passes, and
   *   set the current pass to -1 (random).
   * - If it should be a normal pattern pass, choose an
   *   entry at random between here and top-1 (inclusive)
   *   and swap the current entry with that one.
   */
  randpasses--;			/* To speed up later math */
  accum = randpasses;		/* Bresenham DDA accumulator */
  for (n = 0; n < num; n++)
    {
      if (accum <= randpasses)
	{
	  accum += num - 1;
	  dest[top++] = dest[n];
	  dest[n] = -1;
	}
      else
	{
	  swap = n + irand_mod (&r, top - n - 1);
	  k = dest[n];
	  dest[n] = dest[swap];
	  dest[swap] = k;
	}
      accum -= randpasses;
    }
  /* assert (top == num); */

  memset (&r, 0, sizeof r);	/* Wipe this on general principles */
}

/*
 * The core routine to actually do the work.  This overwrites the first
 * size bytes of the given fd.  Returns -1 on error, 0 on success.
 */
static int
do_wipefd (int fd, char const *qname, struct isaac_state *s,
	   struct Options const *flags)
{
  size_t i;
  struct stat st;
  off_t size;			/* Size to write, size to read */
  unsigned long n;		/* Number of passes for printing purposes */
  int *passarray;

  n = 0;		/* dopass takes n -- 0 to mean "don't print progress" */
  if (flags->verbose)
    n = flags->n_iterations + ((flags->zero_fill) != 0);

  if (fstat (fd, &st))
    {
      error (0, errno, "%s: fstat", qname);
      return -1;
    }

  /* If we know that we can't possibly shred the file, give up now.
     Otherwise, we may go into a infinite loop writing data before we
     find that we can't rewind the device.  */
  if ((S_ISCHR (st.st_mode) && isatty (fd))
      || S_ISFIFO (st.st_mode)
      || S_ISSOCK (st.st_mode))
    {
      error (0, 0, _("%s: invalid file type"), qname);
      return -1;
    }

  /* Allocate pass array */
  passarray = xmalloc (flags->n_iterations * sizeof (int));

  size = flags->size;
  if (size == -1)
    {
      /* Accept a length of zero only if it's a regular file.
	 For any other type of file, try to get the size another way.  */
      if (S_ISREG (st.st_mode))
	{
	  size = st.st_size;
	  if (size < 0)
	    {
	      error (0, 0, _("%s: file has negative size"), qname);
	      return -1;
	    }
	}
      else
	{
	  size = lseek (fd, (off_t) 0, SEEK_END);
	  if (size <= 0)
	    {
	      /* We are unable to determine the length, up front.
		 Let dopass do that as part of its first iteration.  */
	      size = -1;
	    }
	}

      /* Allow `rounding up' only for regular files.  */
      if (0 <= size && !(flags->exact) && S_ISREG (st.st_mode))
	{
	  size += ST_BLKSIZE (st) - 1 - (size - 1) % ST_BLKSIZE (st);

	  /* If in rounding up, we've just overflowed, use the maximum.  */
	  if (size < 0)
	    size = TYPE_MAXIMUM (off_t);
	}
    }

  /* Schedule the passes in random order. */
  genpattern (passarray, flags->n_iterations, s);

  /* Do the work */
  for (i = 0; i < flags->n_iterations; i++)
    {
      if (dopass (fd, qname, &size, passarray[i], s, i + 1, n) < 0)
	{
	  memset (passarray, 0, flags->n_iterations * sizeof (int));
	  free (passarray);
	  return -1;
	}
    }

  memset (passarray, 0, flags->n_iterations * sizeof (int));
  free (passarray);

  if (flags->zero_fill)
    if (dopass (fd, qname, &size, 0, s, flags->n_iterations + 1, n) < 0)
      return -1;

  /* Okay, now deallocate the data.  The effect of ftruncate on
     non-regular files is unspecified, so don't worry about any
     errors reported for them.  */
  if (flags->remove_file && ftruncate (fd, (off_t) 0) != 0
      && S_ISREG (st.st_mode))
    {
      error (0, errno, _("%s: error truncating"), qname);
      return -1;
    }

  return 0;
}

/* A wrapper with a little more checking for fds on the command line */
static int
wipefd (int fd, char const *qname, struct isaac_state *s,
	struct Options const *flags)
{
  int fd_flags = fcntl (fd, F_GETFL);

  if (fd_flags < 0)
    {
      error (0, errno, "%s: fcntl", qname);
      return -1;
    }
  if (fd_flags & O_APPEND)
    {
      error (0, 0, _("%s: cannot shred append-only file descriptor"), qname);
      return -1;
    }
  return do_wipefd (fd, qname, s, flags);
}

/* --- Name-wiping code --- */

/* Characters allowed in a file name - a safe universal set. */
static char const nameset[] =
"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_+=%@#.";

/*
 * This increments the name, considering it as a big-endian base-N number
 * with the digits taken from nameset.  Characters not in the nameset
 * are considered to come before nameset[0].
 *
 * It's not obvious, but this will explode if name[0..len-1] contains
 * any 0 bytes.
 *
 * This returns the carry (1 on overflow).
 */
static int
incname (char *name, unsigned len)
{
  char const *p;

  if (!len)
    return 1;

  p = strchr (nameset, name[--len]);
  /* If the character is not found, replace it with a 0 digit */
  if (!p)
    {
      name[len] = nameset[0];
      return 0;
    }
  /* If this character has a successor, use it */
  if (p[1])
    {
      name[len] = p[1];
      return 0;
    }
  /* Otherwise, set this digit to 0 and increment the prefix */
  name[len] = nameset[0];
  return incname (name, len);
}

/*
 * Repeatedly rename a file with shorter and shorter names,
 * to obliterate all traces of the file name on any system that
 * adds a trailing delimiter to on-disk file names and reuses
 * the same directory slot.  Finally, unlink it.
 * The passed-in filename is modified in place to the new filename.
 * (Which is unlinked if this function succeeds, but is still present if
 * it fails for some reason.)
 *
 * The main loop is written carefully to not get stuck if all possible
 * names of a given length are occupied.  It counts down the length from
 * the original to 0.  While the length is non-zero, it tries to find an
 * unused file name of the given length.  It continues until either the
 * name is available and the rename succeeds, or it runs out of names
 * to try (incname wraps and returns 1).  Finally, it unlinks the file.
 *
 * The unlink is Unix-specific, as ANSI-standard remove has more
 * portability problems with C libraries making it "safe".  rename
 * is ANSI-standard.
 *
 * To force the directory data out, we try to open the directory and
 * invoke fdatasync on it.  This is rather non-standard, so we don't
 * insist that it works, just fall back to a global sync in that case.
 * This is fairly significantly Unix-specific.  Of course, on any
 * filesystem with synchronous metadata updates, this is unnecessary.
 */
static int
wipename (char *oldname, char const *qoldname, struct Options const *flags)
{
  char *newname, *base;	  /* Base points to filename part of newname */
  unsigned len;
  int first = 1;
  int err;
  int dir_fd;			/* Try to open directory to sync *it* */

  newname = xstrdup (oldname);
  if (flags->verbose)
    error (0, 0, _("%s: removing"), qoldname);

  /* Find the file name portion */
  base = strrchr (newname, '/');
  /* Temporary hackery to get a directory fd */
  if (base)
    {
      *base = '\0';
      dir_fd = open (newname, O_RDONLY | O_NOCTTY);
      *base = '/';
    }
  else
    {
      dir_fd = open (".", O_RDONLY | O_NOCTTY);
    }
  base = base ? base + 1 : newname;
  len = strlen (base);

  while (len)
    {
      memset (base, nameset[0], len);
      base[len] = 0;
      do
	{
	  struct stat st;
	  if (lstat (newname, &st) < 0)
	    {
	      if (rename (oldname, newname) == 0)
		{
		  if (dir_fd < 0
		      || (fdatasync (dir_fd) < 0 && fsync (dir_fd) < 0))
		    sync ();	/* Force directory out */
		  if (flags->verbose)
		    {
		      /*
		       * People seem to understand this better than talking
		       * about renaming oldname.  newname doesn't need
		       * quoting because we picked it.  oldname needs to
		       * be quoted only the first time.
		       */
		      char const *old = (first ? qoldname : oldname);
		      error (0, 0, _("%s: renamed to %s"), old, newname);
		      first = 0;
		    }
		  memcpy (oldname + (base - newname), base, len + 1);
		  break;
		}
	      else
		{
		  /* The rename failed: give up on this length.  */
		  break;
		}
	    }
	  else
	    {
	      /* newname exists, so increment BASE so we use another */
	    }
	}
      while (!incname (base, len));
      len--;
    }
  free (newname);
  err = unlink (oldname);
  if (dir_fd < 0 || (fdatasync (dir_fd) < 0 && fsync (dir_fd) < 0))
    sync ();
  close (dir_fd);
  if (!err && flags->verbose)
    error (0, 0, _("%s: removed"), qoldname);
  return err;
}

/*
 * Finally, the function that actually takes a filename and grinds
 * it into hamburger.
 *
 * FIXME
 * Detail to note: since we do not restore errno to EACCES after
 * a failed chmod, we end up printing the error code from the chmod.
 * This is actually the error that stopped us from proceeding, so
 * it's arguably the right one, and in practice it'll be either EACCES
 * again or EPERM, which both give similar error messages.
 * Does anyone disagree?
 */
static int
wipefile (char *name, char const *qname,
	  struct isaac_state *s, struct Options const *flags)
{
  int err, fd;

  fd = open (name, O_WRONLY | O_NOCTTY);
  if (fd < 0)
    {
      if (errno == EACCES && flags->force)
	{
	  if (chmod (name, S_IWUSR) >= 0) /* 0200, user-write-only */
	    fd = open (name, O_WRONLY | O_NOCTTY);
	}
      else if ((errno == ENOENT || errno == ENOTDIR)
	       && strncmp (name, "/dev/fd/", 8) == 0)
	{
	  /* We accept /dev/fd/# even if the OS doesn't support it */
	  int errnum = errno;
	  unsigned long num;
	  char *p;
	  errno = 0;
	  num = strtoul (name + 8, &p, 10);
	  /* If it's completely decimal with no leading zeros... */
	  if (errno == 0 && !*p && num <= INT_MAX &&
	      (('1' <= name[8] && name[8] <= '9')
	       || (name[8] == '0' && !name[9])))
	    {
	      return wipefd ((int) num, qname, s, flags);
	    }
	  errno = errnum;
	}
    }
  if (fd < 0)
    {
      error (0, errno, "%s", qname);
      return -1;
    }

  err = do_wipefd (fd, qname, s, flags);
  if (close (fd) != 0)
    {
      error (0, 0, "%s: close", qname);
      err = -1;
    }
  if (err == 0 && flags->remove_file)
    {
      err = wipename (name, qname, flags);
      if (err < 0)
	error (0, 0, _("%s: cannot remove"), qname);
    }
  return err;
}

int
main (int argc, char **argv)
{
  struct isaac_state s;
  int err = 0;
  struct Options flags;
  char **file;
  int n_files;
  int c;
  int i;

  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  isaac_seed (&s);

  memset (&flags, 0, sizeof flags);

  flags.n_iterations = DEFAULT_PASSES;
  flags.size = -1;

  while ((c = getopt_long (argc, argv, "fn:s:uvxz", long_opts, NULL)) != -1)
    {
      switch (c)
	{
	case 0:
	  break;

	case 'f':
	  flags.force = 1;
	  break;

	case 'n':
	  {
	    uintmax_t tmp;
	    if (xstrtoumax (optarg, NULL, 10, &tmp, NULL) != LONGINT_OK
		|| (word32) tmp != tmp
		|| ((size_t) (tmp * sizeof (int)) / sizeof (int) != tmp))
	      {
		error (EXIT_FAILURE, 0, _("%s: invalid number of passes"),
		       quotearg_colon (optarg));
	      }
	    flags.n_iterations = (size_t) tmp;
	  }
	  break;

	case 'u':
	  flags.remove_file = 1;
	  break;

	case 's':
	  {
	    uintmax_t tmp;
	    if (xstrtoumax (optarg, NULL, 0, &tmp, "cbBkKMGTPEZY0")
		!= LONGINT_OK)
	      {
		error (EXIT_FAILURE, 0, _("%s: invalid file size"),
		       quotearg_colon (optarg));
	      }
	    flags.size = tmp;
	  }
	  break;

	case 'v':
	  flags.verbose = 1;
	  break;

	case 'x':
	  flags.exact = 1;
	  break;

	case 'z':
	  flags.zero_fill = 1;
	  break;

	case_GETOPT_HELP_CHAR;

	case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);

	default:
	  usage (EXIT_FAILURE);
	}
    }

  file = argv + optind;
  n_files = argc - optind;

  if (n_files == 0)
    {
      error (0, 0, _("missing file argument"));
      usage (EXIT_FAILURE);
    }

  for (i = 0; i < n_files; i++)
    {
      char *qname = xstrdup (quotearg_colon (file[i]));
      if (STREQ (file[i], "-"))
	{
	  if (wipefd (STDOUT_FILENO, qname, &s, &flags) < 0)
	    err = 1;
	}
      else
	{
	  /* Plain filename - Note that this overwrites *argv! */
	  if (wipefile (file[i], qname, &s, &flags) < 0)
	    err = 1;
	}
      free (qname);
    }

  /* Just on general principles, wipe s. */
  memset (&s, 0, sizeof s);

  exit (err);
}
/*
 * vim:sw=2:sts=2:
 */
