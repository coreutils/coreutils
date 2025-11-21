#ifndef TERM_SIG_H
# define TERM_SIG_H

# include <signal.h>

static int const term_sig[] =
  {
    SIGALRM, /* our timeout.  */
    SIGINT,  /* Ctrl-C at terminal for example.  */
    SIGQUIT, /* Ctrl-\ at terminal for example.  */
    SIGHUP,  /* terminal closed for example.  */
    SIGTERM, /* if terminated, stop monitored proc.  */

    SIGPIPE, SIGUSR1, SIGUSR2,

    SIGILL, SIGTRAP, SIGABRT, SIGBUS, SIGFPE, SIGSEGV,

# ifdef SIGXCPU
    SIGXCPU,
# endif
# ifdef SIGXFSZ
    SIGXFSZ,
# endif
# ifdef SIGSYS
    SIGSYS,
# endif
# ifdef SIGVTALRM
    SIGVTALRM,
# endif
# ifdef SIGPROF
    SIGPROF,
# endif
# ifdef SIGPOLL
    SIGPOLL,
# endif
# ifdef SIGPWR
    SIGPWR,
# endif
# ifdef SIGSTKFLT
    SIGSTKFLT,
# endif
# ifdef SIGEMT
    SIGEMT,
# endif
# ifdef SIGBREAK
    SIGBREAK,
# endif
  };

#endif
