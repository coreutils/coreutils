#serial 8

dnl From J. David Anglin.

dnl HPUX and other systems can't unlink shared text that is being executed.

# Copyright (C) 2000, 2001, 2004 Free Software Foundation, Inc.
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_UNLINK_BUSY_TEXT],
[dnl
  AC_CACHE_CHECK([whether a running program can be unlinked],
    jm_cv_func_unlink_busy_text,
    [
      AC_TRY_RUN([
        main (argc, argv)
          int argc;
          char **argv;
        {
          if (!argc)
            exit (-1);
          exit (unlink (argv[0]));
        }
	],
      jm_cv_func_unlink_busy_text=yes,
      jm_cv_func_unlink_busy_text=no,
      jm_cv_func_unlink_busy_text=no
      )
    ]
  )

  if test $jm_cv_func_unlink_busy_text = no; then
    INSTALL=$ac_install_sh
  fi
])
