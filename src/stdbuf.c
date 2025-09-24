/* stdbuf -- setup the standard streams for a command
   Copyright (C) 2009-2025 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* Written by Pádraig Brady.  */

#include <config.h>
#include <stdio.h>
#include <getopt.h>
#include <sys/types.h>

#include "system.h"
#include "assure.h"
#include "filenamecat.h"
#include "quote.h"
#include "xreadlink.h"
#include "xstrtol.h"
#include "c-ctype.h"

/* The official name of this program (e.g., no 'g' prefix).  */
#define PROGRAM_NAME "stdbuf"
#define LIB_NAME "libstdbuf.so" /* FIXME: don't hardcode  */

#define AUTHORS proper_name_lite ("Padraig Brady", "P\303\241draig Brady")

static char *program_path;

static struct
{
  size_t size;
  int optc;
  char *optarg;
} stdbuf[3];

static struct option const longopts[] =
{
  {"input", required_argument, nullptr, 'i'},
  {"output", required_argument, nullptr, 'o'},
  {"error", required_argument, nullptr, 'e'},
  {GETOPT_HELP_OPTION_DECL},
  {GETOPT_VERSION_OPTION_DECL},
  {nullptr, 0, nullptr, 0}
};

/* Set size to the value of STR, interpreted as a decimal integer,
   optionally multiplied by various values.
   Return -1 on error, 0 on success.

   This supports dd BLOCK size suffixes.
   Note we don't support dd's b=512, c=1, w=2 or 21x512MiB formats.  */
static int
parse_size (char const *str, size_t *size)
{
  uintmax_t tmp_size;
  enum strtol_error e = xstrtoumax (str, nullptr, 10,
                                    &tmp_size, "EGkKMPQRTYZ0");
  if (e == LONGINT_OK && SIZE_MAX < tmp_size)
    e = LONGINT_OVERFLOW;

  if (e == LONGINT_OK)
    {
      errno = 0;
      *size = tmp_size;
      return 0;
    }

  errno = (e == LONGINT_OVERFLOW ? EOVERFLOW : errno);
  return -1;
}

void
usage (int status)
{
  if (status != EXIT_SUCCESS)
    emit_try_help ();
  else
    {
      printf (_("Usage: %s OPTION... COMMAND\n"), program_name);
      fputs (_("\
Run COMMAND, with modified buffering operations for its standard streams.\n\
"), stdout);

      emit_mandatory_arg_note ();

      fputs (_("\
  -i, --input=MODE   adjust standard input stream buffering\n\
  -o, --output=MODE  adjust standard output stream buffering\n\
  -e, --error=MODE   adjust standard error stream buffering\n\
"), stdout);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      fputs (_("\n\
If MODE is 'L' the corresponding stream will be line buffered.\n\
This option is invalid with standard input.\n"), stdout);
      fputs (_("\n\
If MODE is '0' the corresponding stream will be unbuffered.\n\
"), stdout);
      fputs (_("\n\
Otherwise MODE is a number which may be followed by one of the following:\n\
KB 1000, K 1024, MB 1000*1000, M 1024*1024, and so on for G,T,P,E,Z,Y,R,Q.\n\
Binary prefixes can be used, too: KiB=K, MiB=M, and so on.\n\
In this case the corresponding stream will be fully buffered with the buffer\n\
size set to MODE bytes.\n\
"), stdout);
      fputs (_("\n\
NOTE: If COMMAND adjusts the buffering of its standard streams ('tee' does\n\
for example) then that will override corresponding changes by 'stdbuf'.\n\
Also some filters (like 'dd' and 'cat' etc.) don't use streams for I/O,\n\
and are thus unaffected by 'stdbuf' settings.\n\
"), stdout);
      emit_exec_status (PROGRAM_NAME);
      emit_ancillary_info (PROGRAM_NAME);
    }
  exit (status);
}

/* argv[0] can be anything really, but generally it contains
   the path to the executable or just a name if it was executed
   using $PATH. In the latter case to get the path we can:
   search getenv("PATH"), readlink("/prof/self/exe"), getenv("_"),
   dladdr(), pstat_getpathname(), etc.  */

static void
set_program_path (char const *arg)
{
  if (strchr (arg, '/'))        /* Use absolute or relative paths directly.  */
    {
      program_path = dir_name (arg);
    }
  else
    {
      char *path = xreadlink ("/proc/self/exe");
      if (path)
        program_path = dir_name (path);
      else if ((path = getenv ("PATH")))
        {
          char *dir;
          path = xstrdup (path);
          for (dir = strtok (path, ":"); dir != nullptr;
               dir = strtok (nullptr, ":"))
            {
              char *candidate = file_name_concat (dir, arg, nullptr);
              if (access (candidate, X_OK) == 0)
                {
                  program_path = dir_name (candidate);
                  free (candidate);
                  break;
                }
              free (candidate);
            }
        }
      free (path);
    }
}

static int
optc_to_fileno (int c)
{
  int ret = -1;

  switch (c)
    {
    case 'e':
      ret = STDERR_FILENO;
      break;
    case 'i':
      ret = STDIN_FILENO;
      break;
    case 'o':
      ret = STDOUT_FILENO;
      break;
    }

  return ret;
}

static void
set_LD_PRELOAD (void)
{
  int ret;
#ifdef __APPLE__
  char const *preload_env = "DYLD_INSERT_LIBRARIES";
#elif defined _AIX
  char const *preload_env = (sizeof (void *) < 8
                             ? "LDR_PRELOAD" : "LDR_PRELOAD64");
#else
  char const *preload_env = "LD_PRELOAD";
#endif
  char *old_libs = getenv (preload_env);
  char *LD_PRELOAD;

  /* Note this would auto add the appropriate search path for "libstdbuf.so":
     gcc stdbuf.c -Wl,-rpath,'$ORIGIN' -Wl,-rpath,$PKGLIBEXECDIR
     However we want the lookup done for the exec'd command not stdbuf.

     Since we don't link against libstdbuf.so add it to PKGLIBEXECDIR
     rather than to LIBDIR.

     Note we could add "" as the penultimate item in the following list
     to enable searching for libstdbuf.so in the default system lib paths.
     However that would not indicate an error if libstdbuf.so was not found.
     Also while this could support auto selecting the right arch in a multilib
     environment, what we really want is to auto select based on the arch of the
     command being run, rather than that of stdbuf itself.  This is currently
     not supported due to the unusual need for controlling the stdio buffering
     of programs that are a different architecture to the default on the
     system (and that of stdbuf itself).  */
  char const *const search_path[] = {
    program_path,
    PKGLIBEXECDIR,
    nullptr
  };

  char const *const *path = search_path;
  char *libstdbuf;

  while (true)
    {
      struct stat sb;

      if (!**path)              /* system default  */
        {
          libstdbuf = xstrdup (LIB_NAME);
          break;
        }
      ret = asprintf (&libstdbuf, "%s/%s", *path, LIB_NAME);
      if (ret < 0)
        xalloc_die ();
      if (stat (libstdbuf, &sb) == 0)   /* file_exists  */
        break;
      free (libstdbuf);

      ++path;
      if ( ! *path)
        error (EXIT_CANCELED, 0, _("failed to find %s"), quote (LIB_NAME));
    }

  /* FIXME: Do we need to support libstdbuf.dll, c:, '\' separators etc?  */

  if (old_libs)
    ret = asprintf (&LD_PRELOAD, "%s=%s:%s", preload_env, old_libs, libstdbuf);
  else
    ret = asprintf (&LD_PRELOAD, "%s=%s", preload_env, libstdbuf);

  if (ret < 0)
    xalloc_die ();

  free (libstdbuf);

  ret = putenv (LD_PRELOAD);
#ifdef __APPLE__
  if (ret == 0)
    ret = setenv ("DYLD_FORCE_FLAT_NAMESPACE", "y", 1);
#endif

  if (ret != 0)
    error (EXIT_CANCELED, errno,
           _("failed to update the environment with %s"),
           quote (LD_PRELOAD));
}

/* Populate environ with _STDBUF_I=$MODE _STDBUF_O=$MODE _STDBUF_E=$MODE.
   Return TRUE if any environment variables set.   */

static bool
set_libstdbuf_options (void)
{
  bool env_set = false;

  for (size_t i = 0; i < countof (stdbuf); i++)
    {
      if (stdbuf[i].optarg)
        {
          char *var;
          int ret;

          if (*stdbuf[i].optarg == 'L')
            ret = asprintf (&var, "%s%c=L", "_STDBUF_",
                            c_toupper (stdbuf[i].optc));
          else
            ret = asprintf (&var, "%s%c=%zu", "_STDBUF_",
                            c_toupper (stdbuf[i].optc),
                            stdbuf[i].size);
          if (ret < 0)
            xalloc_die ();

          if (putenv (var) != 0)
            error (EXIT_CANCELED, errno,
                   _("failed to update the environment with %s"),
                   quote (var));

          env_set = true;
        }
    }

  return env_set;
}

int
main (int argc, char **argv)
{
  int c;

  initialize_main (&argc, &argv);
  set_program_name (argv[0]);
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  initialize_exit_failure (EXIT_CANCELED);
  atexit (close_stdout);

  while ((c = getopt_long (argc, argv, "+i:o:e:", longopts, nullptr)) != -1)
    {
      int opt_fileno;

      switch (c)
        {
        /* Old McDonald had a farm ei...  */
        case 'e':
        case 'i':
        case 'o':
          opt_fileno = optc_to_fileno (c);
          affirm (0 <= opt_fileno && opt_fileno < countof (stdbuf));
          stdbuf[opt_fileno].optc = c;
          while (c_isspace (*optarg))
            optarg++;
          stdbuf[opt_fileno].optarg = optarg;
          if (c == 'i' && *optarg == 'L')
            {
              /* -oL will be by far the most common use of this utility,
                 but one could easily think -iL might have the same affect,
                 so disallow it as it could be confusing.  */
              error (0, 0, _("line buffering standard input is meaningless"));
              usage (EXIT_CANCELED);
            }

          if (!streq (optarg, "L")
              && parse_size (optarg, &stdbuf[opt_fileno].size) == -1)
            error (EXIT_CANCELED, errno, _("invalid mode %s"), quote (optarg));

          break;

        case_GETOPT_HELP_CHAR;

        case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);

        default:
          usage (EXIT_CANCELED);
        }
    }

  argv += optind;
  argc -= optind;

  /* must specify at least 1 command.  */
  if (argc < 1)
    {
      error (0, 0, _("missing operand"));
      usage (EXIT_CANCELED);
    }

  if (! set_libstdbuf_options ())
    {
      error (0, 0, _("you must specify a buffering mode option"));
      usage (EXIT_CANCELED);
    }

  /* Try to preload libstdbuf first from the same path as
     stdbuf is running from.  */
  set_program_path (program_name);
  if (!program_path)
    program_path = xstrdup (PKGLIBDIR);  /* Need to init to non-null.  */
  set_LD_PRELOAD ();
  free (program_path);

  execvp (*argv, argv);

  int exit_status = errno == ENOENT ? EXIT_ENOENT : EXIT_CANNOT_INVOKE;
  error (0, errno, _("failed to run command %s"), quote (argv[0]));
  return exit_status;
}
