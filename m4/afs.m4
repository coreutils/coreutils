#serial 5

AC_DEFUN([jm_AFS],
  [
    AC_MSG_CHECKING(for AFS)
    if test -d /afs; then
      AC_DEFINE(AFS, 1, [Define if you have the Andrew File System.])
      ac_result=yes
    else
      ac_result=no
    fi
    AC_MSG_RESULT($ac_result)
  ])
