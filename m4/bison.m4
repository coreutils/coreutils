#serial 2

AC_DEFUN([jm_BISON],
[
  # getdate.y works with bison only.
  : ${YACC='bison -y'}
  AC_SUBST(YACC)
])
