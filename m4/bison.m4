#serial 3

AC_DEFUN([gl_BISON],
[
  # getdate.y works with bison only.
  : ${YACC='bison -y'}
  AC_SUBST(YACC)
])
