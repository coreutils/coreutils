/* TODO:
   - don't use pfstatus (at least vprintf isn't portable) -- or maybe
       leave it in and see who complains
   - use consistent non-capitalization in error messages
   - add standard GNU copyleft comment
 */

/*
 * shred.c - by Colin Plumb.
 *
 * Do a more-secure overwrite of given files or devices, so that not even
 * very expensive hardware probing can recover the data.
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
 * If asked to wipe a file, this also removes it, renaming it to in a
 * clever way to try to leave no trace of the original filename.
 *
 * Copyright 1997-1999 Colin Plumb <colin@nyx.net>.  This program may
 * be freely distributed under the terms of the GNU GPL, the BSD license,
 * or Larry Wall's "Artistic License"   Even if you use the BSD license,
 * which does not require it, I'd really like to get improvements back.
 *
 * The ISAAC code still bears some resemblance to the code written
 * by Bob Jenkins, but he permits pretty unlimited use.
 *
 * This was inspired by a desire to improve on some code titled:
 * Wipe V1.0-- Overwrite and delete files.  S. 2/3/96
 * but I've rewritten everything here so completely that no trace of
 * the original remains.
 *
 * Things to think about:
 * - Security: Is there any risk to the race
 *   between overwriting and unlinking a file?  Will it do anything
 *   drastically bad if told to attack a named pipes or a sockets?
 */


#include <config.h>
#include <getopt.h>
#include <stdio.h>
#include <stdarg.h>		/* Used by pferror */

#include "system.h"
#include "error.h"
#include "human.h"
#include "quotearg.h"
#include "xstrtoul.h"

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "shred"

#define AUTHORS "Colin Plumb"

#define DEFAULT_PASSES 25	/* Default */

/* How often to update wiping display */
#define VERBOSE_UPDATE	100*1024

/* FIXME: add comments */
struct Options
{
  enum { NO_CONTENTS, FREED_CONTENTS, ALL_CONTENTS } contents;
  enum { NO_LINKS, ORDINARY_LINKS, ALL_LINKS } links;
  int force;
  size_t n_iterations;
  int verbose;
  int exact;
  int zero_fill;
};

/* If positive, the units to use when printing sizes;
   if negative, the human-readable base.
   For now, this is a constant.  */
static int const output_block_size = -1024;

static struct option const long_opts[] =
{
  {"no-contents", no_argument, NULL, 'b'},
  {"freed-contents", no_argument, NULL, 'c'},
  {"all-contents", no_argument, NULL, 'C'},
  {"no-links", no_argument, NULL, 'k'},
  {"ordinary-links", no_argument, NULL, 'l'},
  {"all-links", no_argument, NULL, 'L'},
  {"exact", required_argument, NULL, 'x'},
  {"force", no_argument, NULL, 'f'},
  {"iterations", required_argument, NULL, 'n'},
  {"verbose", no_argument, NULL, 'v'},
  {"zero", required_argument, NULL, 'z'},
  {GETOPT_HELP_OPTION_DECL},
  {GETOPT_VERSION_OPTION_DECL},
  {NULL, 0, NULL, 0}
};

/* Global variable for error printing purposes */
char const *program_name;

void usage (int status) __attribute__ ((__noreturn__));

void
usage (int status)
{
  if (status != 0)
    fprintf (stderr, _("Try `%s --help' for more information.\n"),
	     program_name);
  else
    {
      printf (_("Usage: %s [OPTIONS] FILE [...]\n"), program_name);
      printf (_("\
Overwrite a file to hide its contents.\n\
\n\
  -b, --no-contents  do not shred contents\n\
  -c, --freed-contents  shred contents that will be freed\n\
  -C, --all-contents  shred all contents (default)\n\
  -f, --force    change permissions to allow writing if necessary\n\
  -k, --no-links  do not shred links (default)\n\
  -l, --ordinary-links  shred links to regular files\n\
  -L, --all-links  shred all links\n\
  -n, --iterations=N  Overwrite N times instead of the default (25)\n\
  -v, --verbose  indicate progress (-vv to leave progress on screen)\n\
  -x, --exact    do not round file sizes up to the next full block\n\
  -z, --zero     add a final overwrite with zeros to hide shredding\n\
  -              shred standard output (but don't remove it)\n\
  --help         display this help and exit\n\
  --version      print version information and exit\n\
\n\
FIXME maybe add more discussion here?\n\
"));
      puts (_("\nReport bugs to <bug-fileutils@gnu.org>."));
      close_stdout ();
    }
  exit (status);
}

#if ! HAVE_FDATASYNC
# define fdatasync(fd) (-1)
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

#if ULONG_MAX == 0xffffffff
typedef unsigned long word32;
#else
# if UINT_MAX == 0xffffffff
typedef unsigned word32;
# else
#  if USHRT_MAX == 0xffffffff
typedef unsigned short word32;
#  else
#   if UCHAR_MAX == 0xffffffff
typedef unsigned char word32;
#   else
#    error No 32-bit type available!
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
#define ind(mm, x)  *(unsigned *)((char *)(mm) + ( (x) & (ISAAC_WORDS-1)<<2 ))

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
  *(m) = y = ind ((mm), x) + (a) + (b), \
  *(r) = b = ind ((mm), (y) >> ISAAC_LOG) + x \
)

/*
 * Refill the entire R array, and update S.
 */
static void
isaac_refill (struct isaac_state *s, word32 r[ISAAC_WORDS])
{
  register word32 a, b;		/* Caches of a and b */
  register word32 x, y;		/* Temps needed by isaac_step() macro */
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
isaac_mix (struct isaac_state *s, word32 const seed[ISAAC_WORDS])
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

#if 0
  /* The initialization of iv is a precomputed form of: */
  for (i = 0; i < 7; i++)
    iv[i] = 0x9e3779b9;		/* the golden ratio */
  for (i = 0; i < 4; ++i)	/* scramble it */
    mix (iv[0], iv[1], iv[2], iv[3], iv[4], iv[5], iv[6], iv[7]);
#endif
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

/*
 * Get seed material.  16 bytes (128 bits) is plenty, but if we have
 * /dev/urandom, we get 32 bytes = 256 bits for complete overkill.
 */
static void
isaac_seed (struct isaac_state *s)
{
  char *p = (char *) s->mm;
  char *lim = p + sizeof s->mm;
#define MIXIN_BOUND(s) ((s) < lim - p ? (s) : lim - p)
#define MIXIN(o) \
    do \
      { \
	size_t s = MIXIN_BOUND (sizeof (o)); \
        memcpy (p, (char *) &(o), s); \
	p += s; \
      } \
    while (0)

  /* Mix in bits of random information from the environment.
     Mix the most random items first, in case lim - p is small
     and we have to truncate.  */

  {
    int fd = open ("/dev/urandom", O_RDONLY);
    if (fd >= 0)
      {
	size_t s = MIXIN_BOUND (32);
	read (fd, p, s);
	p += s;
	close (fd);
      }
    else
      {
	fd = open ("/dev/random", O_RDONLY | O_NONBLOCK);
	if (fd >= 0)
	  {
	    /* /dev/random is more precious, so use less */
	    size_t s = MIXIN_BOUND (16);
	    read (fd, p, s);
	    p += s;
	    close (fd);
	  }
      }
  }

#ifdef HAVE_GETHRTIME
  {
    hrtime_t t = gethrtime ();
    MIXIN (t);
  }
#endif

#ifdef HAVE_CLOCK_GETTIME
  {
    struct timespec t;
    clock_gettime (CLOCK_REALTIME, &t);
    MIXIN (t);
  }
#endif

  {
    time_t t = time ((time_t *) 0);
    MIXIN (t);
  }

  {
    pid_t t = getpid ();
    MIXIN (t);
    t = getppid ();
    MIXIN (t);
  }

  {
    uid_t t = getuid ();
    MIXIN (t);
  }

  {
    gid_t t = getgid ();
    MIXIN (t);
  }

  isaac_init (s, s->mm, sizeof (s->mm));
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
 * Like perror() but fancier.  (And fmt is not allowed to be NULL)
 * This apparent use of features specific to GNU C is actually portable;
 * see the definitions in error.h.
 */
static void pfstatus (char const *,...)
     __attribute__ ((__format__ (__printf__, 1, 2)));

/*
 * Maintain a status line on stdout.  This is done by using CR and
 * overprinting a new line when it changes, padding with trailing blanks
 * as needed to hide all of the previous line.  (Assuming that the return
 * value of printf is an accurate width.)
 FIXME: we can't assume that
 */
static int status_visible = 0;	/* Number of visible characters */
static int status_pos = 0;	/* Current position, including padding */

/* Print a new status line, overwriting the previous one. */
static void
pfstatus (char const *fmt,...)
{
  int new;			/* New status_visible value */
  va_list ap;

  /* If we weren't at beginning, go there. */
  if (status_pos)
    putchar ('\r');
  va_start (ap, fmt);
  new = vprintf (fmt, ap);
  va_end (ap);
  if (new >=0)
    {
      status_pos = new;
      while (status_pos < status_visible)
	{
	  putchar (' ');
	  status_pos++;
	}
      status_visible = new;
    }
  fflush (stdout);
}

/* Leave current status (if any) visible and go to the next free line. */
static void
flushstatus (void)
{
  if (status_visible)
    {
      putchar ('\n');		/* Leave line visible */
      fflush (stdout);
      status_visible = status_pos = 0;
    }
  else if (status_pos)
    {
      putchar ('\r');		/* Go back to beginning of line */
      fflush (stdout);
      status_pos = 0;
    }
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
 * Fill a buffer with random data.
 * size is rounded UP to a multiple of ISAAC_BYTES.
 */
static void
fillrand (struct isaac_state *s, word32 *r, size_t size)
{
  size = (size + ISAAC_BYTES - 1) / ISAAC_BYTES;

  while (size--)
    {
      isaac_refill (s, r);
      r += ISAAC_WORDS;
    }
}

/* Generate a 6-character (+ nul) pass name string */
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
 * to the file descriptor fd.   Name, k and n are passed in only for verbose
 * progress message purposes.  If n == 0, no progress messages are printed.
 * If size is negative, write until we fall off the end.
 */
static int
dopass (int fd, char const *name, off_t size, int type,
	struct isaac_state *s, unsigned long k, unsigned long n)
{
  off_t off;			/* Offset into file */
  off_t thresh;			/* Offset for next status update */
  size_t lim;			/* Amount of data to try writing */
  size_t soff;			/* Offset into buffer for next write */
  ssize_t ssize;		/* Return value from write() */
#if ISAAC_WORDS > 1024
  word32 r[ISAAC_WORDS * 3];	/* Multiple of 4K and of pattern size */
#else
  word32 r[1024 * 3];		/* Multiple of 4K and of pattern size */
#endif
  char pass_string[PASS_NAME_SIZE];	/* Name of current pass */

  if (lseek (fd, 0, SEEK_SET) == (off_t) -1)
    {
      error (0, errno, "%s: %s", quotearg_colon (name), _("cannot rewind"));
      return -1;
    }

  /* Constant fill patterns need only be set up once. */
  if (type >= 0)
    {
      lim = sizeof (r);
      if (0 <= size && (off_t) lim > size)
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
  thresh = 0;
  if (n)
    {
      pfstatus (_("%s: pass %lu/%lu (%s)...)"), quotearg_colon (name),
		k, n, pass_string);
      thresh = VERBOSE_UPDATE;
    }

  for (off = 0; off < size || size < 0; )
    {
      /* How much to write this time? */
      lim = sizeof (r);
      if (0 <= size && size - off < (off_t) lim)
	lim = (size_t) (size - off);
      if (type < 0)
	fillrand (s, r, lim);
      /* Loop to retry partial writes. */
      for (soff = 0; soff < lim; soff += ssize)
	{
	  ssize = write (fd, (char *) r + soff, lim - soff);
	  if (ssize < 0)
	    {
	      char buf[LONGEST_HUMAN_READABLE + 1];
	      if (size < 0 && errno == EIO)
		{
		  /* Now we know the file size, since we fell off the end.  */
		  thresh = size = off + lim;
		  break;
		}
	      error (0, errno, _("%s: cannot write at offset %s"),
		     quotearg_colon (name),
		     human_readable ((uintmax_t) (off + soff),
				     buf, 1, 1));
	      return -1;
	    }
	}

      /* Okay, we have written "lim" bytes. */
      off += lim;

      /* Time to print progress? */
      if (thresh <= off && n)
	{
	  char offbuf[LONGEST_HUMAN_READABLE + 1];
	  char sizebuf[LONGEST_HUMAN_READABLE + 1];
	  pfstatus (_("%s: pass %lu/%lu (%s)...%s/%s"),
		    quotearg_colon (name), k, n, pass_string,
		    human_readable ((uintmax_t) off, offbuf, 1,
				    output_block_size),
		    (size < 0
		     ? "?"
		     : human_readable ((uintmax_t) size, sizebuf, 1,
				       output_block_size)));
	  thresh += VERBOSE_UPDATE;
	  if (! (0 <= thresh && (thresh < size || size < 0)))
	    thresh = size;
	}
    }
  /* Force what we just wrote to hit the media. */
  if (fdatasync (fd) < 0 && fsync (fd) < 0)
    {
      error (0, errno, "%s: fsync", quotearg_colon (name));
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
 * Then, all possible 3-bit patterns.  Again, 8-2 = 6.
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

  /* assert(d == dest+top); */

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
  /* assert(top == num); */

  memset (&r, 0, sizeof (r));	/* Wipe this on general principles */
}

/*
 * The core routine to actually do the work.  This overwrites the first
 * size bytes of the given fd.  Returns -1 on error, 0 on success.
 */
static int
wipefd (int fd, char const *name, struct isaac_state *s,
	struct Options const *flags)
{
  size_t i;
  struct stat st;
  off_t size;			/* Size to write; -1 if not known */
  size_t passes = flags->n_iterations;
  unsigned long n;		/* Number of passes for printing purposes */

  n = 0;		/* dopass takes n -- 0 to mean "don't print progress" */
  if (flags->verbose)
    n = passes + ((flags->zero_fill) != 0);

  if (fstat (fd, &st) != 0)
    {
      error (0, errno, "%s: fstat", quotearg_colon (name));
      return -1;
    }

  size = S_ISREG (st.st_mode) ? st.st_size : (off_t) -1;

  if (0 < size && 0 < st.st_blksize && !(flags->exact))
    {
      /* Round up to the next st_blksize to include "slack" */
      size += st.st_blksize - 1 - (size - 1) % st.st_blksize;
      if (size < 0)
	size = TYPE_MAXIMUM (off_t);
    }

  if (passes)
    {
      /* Allocate pass array */
      int *passarray = malloc (passes * sizeof (int));
      if (!passarray)
	{
	  error (0, 0, _("virtual memory exhausted"));
	  return -1;
	}

      /* Schedule the passes in random order. */
      genpattern (passarray, passes, s);

      /* Do the work */
      for (i = 0; i < passes; i++)
	{
	  if (dopass (fd, name, size, passarray[i], s, i + 1, n) < 0)
	    {
	      memset (passarray, 0, passes * sizeof (int));
	      free (passarray);
	      return -1;
	    }
	  if (flags->verbose > 1)
	    flushstatus ();
	}

      memset (passarray, 0, passes * sizeof (int));
      free (passarray);
    }

  if (flags->zero_fill)
    if (dopass (fd, name, size, 0, s, passes + 1, n) < 0)
      return -1;

  return 0;
}

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
    return -1;

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
 * the same directory slot.  Finally, remove it.
 * The passed-in filename is modified in place to the new filename.
 * (Which is removed if this function succeeds, but is still present if
 * it fails for some reason.)
 *
 * The main loop is written carefully to not get stuck if all possible
 * names of a given length are occupied.  It counts down the length from
 * the original to 0.  While the length is non-zero, it tries to find an
 * unused file name of the given length.  It continues until either the
 * name is available and the rename succeeds, or it runs out of names
 * to try (incname() wraps and returns 1).  Finally, it removes the file.
 *
 * Note that rename() and remove() are both in the ANSI C standard,
 * so that part, at least, is NOT Unix-specific.
 *
 * FIXME: update this comment.
 * To force the directory data out, we try to open() the directory and
 * invoke fdatasync() on it.  This is rather non-standard, so we don't
 * insist that it works, just fall back to a global sync() in that case.
 * Unfortunately, this code is Unix-specific.
 */
static int
wipename (char *oldname, struct Options const *flags)
{
  char *newname, *origname = 0;
  char *base;			/* Pointer to filename component, after directories. */
  unsigned len;
  int err;
  int dir_fd;			/* Try to open directory to sync *it* */

  if (flags->verbose)
    pfstatus (_("%s: removing"), quotearg_colon (oldname));

  newname = strdup (oldname);	/* This is a malloc */
  if (!newname)
    {
      error (0, 0, _("virtual memory exhausted"));
      return -1;
    }
  if (flags->verbose)
    {
      origname = strdup (oldname);
      if (!origname)
	{
	  error (0, 0, _("virtual memory exhausted"));
	  free (newname);
	  return -1;
	}
    }

  /* Find the file name portion */
  base = strrchr (newname, '/');
  /* Temporary hackery to get a directory fd */
  if (base)
    {
      *base = '\0';
      dir_fd = open (newname, O_RDONLY);
      *base = '/';
    }
  else
    {
      dir_fd = open (".", O_RDONLY);
    }
  base = base ? base + 1 : newname;
  len = strlen (base);

  while (len)
    {
      memset (base, nameset[0], len);
      base[len] = 0;
      do
	{
	  if (access (newname, F_OK) < 0
	      && !rename (oldname, newname))
	    {
	      if (dir_fd < 0 || (fdatasync (dir_fd) < 0 && fsync (dir_fd) < 0))
		sync ();	/* Force directory out */
	      if (origname)
		{
		  /* The use of origname (rather than oldname) here is
		     deliberate.  It makes the -v output more intelligible
		     at the expense of making the `renamed to ...' messages
		     use the logical (original) file name.  */
		  pfstatus (_("%s: renamed to: %s"),
			    quotearg_colon (origname), newname);
		  if (flags->verbose > 1)
		    flushstatus ();
		}
	      memcpy (oldname + (base - newname), base, len + 1);
	      break;
	    }
	}
      while (!incname (base, len));
      len--;
    }
  free (newname);
  err = remove (oldname) ? errno : 0;
  if (dir_fd < 0 || (fdatasync (dir_fd) < 0 && fsync (dir_fd) < 0))
    sync ();
  close (dir_fd);
  if (origname)
    {
      if (!err && flags->verbose)
	pfstatus (_("%s: removed"), quotearg_colon (origname));
      free (origname);
    }
  return err;
}

/*
 * Finally, the function that actually takes a filename and grinds
 * it into hamburger.  Returns nonzero if there was an error.
 */
static int
wipefile (char *name, struct isaac_state *s, struct Options const *flags)
{
  int err = 0;
  struct stat st;
  int remove_link = flags->links == ALL_LINKS;

  if (flags->links == ORDINARY_LINKS
      || (flags->contents == FREED_CONTENTS && remove_link))
    {
      if (lstat (name, &st) != 0)
	{
	  error (0, errno, "%s", quotearg_colon (name));
	  return -1;
	}
      if (flags->links == ORDINARY_LINKS)
	remove_link = S_ISREG (st.st_mode) || S_ISLNK (st.st_mode);
    }

  if (flags->contents == ALL_CONTENTS
      || (flags->contents == FREED_CONTENTS && remove_link
	  && !S_ISLNK (st.st_mode) && st.st_nlink <= 1))
    {
      int fd = open (name, O_WRONLY);
      if (fd < 0)
	{
	  if (errno == EACCES && flags->force)
	    {
	      if (chmod (name, S_IWUSR) != 0)
		{
		  error (0, errno, "%s: %s", quotearg_colon (name),
			 _("cannot change permissions"));
		  return -1;
		}
	      fd = open (name, O_WRONLY);
	    }
	  if (fd < 0)
	    {
	      error (0, errno, "%s", quotearg_colon (name));
	      return -1;
	    }
	}

      err = wipefd (fd, name, s, flags);
      if (close (fd) != 0)
	{
	  error (0, errno, "%s: close", quotearg_colon (name));
	  return -1;
	}
    }

  if (err == 0 && remove_link)
    {
      err = wipename (name, flags);
      if (err != 0)
	error (0, err < 0 ? 0 : err, "%s: %s", quotearg_colon (name),
	       _("cannot remove"));
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

  isaac_seed (&s);

  memset (&flags, 0, sizeof flags);
  flags.contents = ALL_CONTENTS;
  flags.n_iterations = DEFAULT_PASSES;

  while ((c = getopt_long (argc, argv, "bcCfklLn:pvxz", long_opts, NULL)) != -1)
    {
      switch (c)
	{
	case 0:
	  break;

	case 'b':
	  flags.contents = NO_CONTENTS;
	  break;

	case 'c':
	  flags.contents = FREED_CONTENTS;
	  break;

	case 'C':
	  flags.contents = ALL_CONTENTS;
	  break;

	case 'f':
	  flags.force = 1;
	  break;

	case 'k':
	  flags.links = NO_LINKS;
	  break;

	case 'l':
	  flags.links = ORDINARY_LINKS;
	  break;

	case 'L':
	  flags.links = ALL_LINKS;
	  break;

	case 'n':
	  {
	    unsigned long int tmp_ulong;
	    if (xstrtoul (optarg, NULL, 10, &tmp_ulong, NULL) != LONGINT_OK
		|| (word32) tmp_ulong != tmp_ulong
		|| ((size_t) (tmp_ulong * sizeof (int)) / sizeof (int)
		    != tmp_ulong))
	      error (1, 0, "%s: %s", quotearg_colon (optarg),
		     _("invalid number of passes"));
	    flags.n_iterations = tmp_ulong;
	  }
	  break;

	case 'v':
	  flags.verbose++;
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
	  usage (1);
	}
    }

  file = argv + optind;
  n_files = argc - optind;

  if (n_files == 0)
    {
      error (0, 0, _("too few arguments"));
      usage (1);
    }

  for (i = 0; i < n_files; i++)
    {
      if (STREQ (file[i], "-"))
	{
	  int fd_flags = fcntl (STDOUT_FILENO, F_GETFL);
	  if (fd_flags < 0)
	    {
	      error (0, errno, _("standard output"));
	      err = 1;
	    }
	  else if ((fd_flags & O_APPEND) != 0)
	    {
	      error (0, 0, _("cannot shred append-only standard output"));
	      err = 1;
	    }
	  else if (wipefd (STDOUT_FILENO, file[i], &s, &flags) < 0)
	    err = 1;
	}
      else
	{
	  /* Plain filename - Note that this overwrites *argv! */
	  if (wipefile (file[i], &s, &flags) < 0)
	    err = 1;
	}
      flushstatus ();
    }

  /* Just on general principles, wipe s. */
  memset (&s, 0, sizeof (s));

  close_stdout ();

  exit (err);
}
