#serial 3
# Make sure _GNU_SOURCE is defined where necessary: as early as possible
# for configure-time tests, as well as for every source file that includes
# config.h.

# From Jim Meyering.

AC_DEFUN([AC__GNU_SOURCE],
[
  # Make sure that _GNU_SOURCE is defined for all subsequent
  # configure-time compile tests.
  # This definition must be emitted (into confdefs.h) before any
  # test that involves compilation.
  cat >>confdefs.h <<\EOF
#ifndef _GNU_SOURCE
# define _GNU_SOURCE
#endif
EOF

  # Emit this code into config.h.in.
  # The ifndef is to avoid redefinition warnings.
  AH_VERBATIM([_GNU_SOURCE], [#ifndef _GNU_SOURCE
# define _GNU_SOURCE
#endif])
])
