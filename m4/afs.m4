#serial 2

AC_DEFUN(jm_AFS,
  AC_MSG_CHECKING(for AFS)
  test -d /afs \
    && AC_DEFINE(AFS, 1, [Define if you have the Andrew File System.])
)
