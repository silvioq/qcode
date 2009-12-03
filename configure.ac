AC_INIT([qcode],[0.0.2], [silvioq@gmail.com])
AM_INIT_AUTOMAKE
AC_PROG_CC
AC_PROG_RANLIB
AC_PROG_YACC
AC_PROG_LEX
AC_PROG_LIBTOOL
AC_CONFIG_HEADERS([config.h])

AC_ARG_ENABLE([qasm],
[  --disable-qasm          Disable qasm generation],
[case  "${enableval}"  in 
  yes)  qasm=true ;;
  no)   qasm=false ;;
  *)    AC_MSG_ERROR(bad value ${enableval} for --enable-qasm) ;;
esac],[qasm=true])

AM_CONDITIONAL([QASM], [test  x$qasm = xtrue])

if [[ "x$qasm" = "xtrue" ]]; then
  qasmmakefile="qasm/Makefile"
fi

AC_CONFIG_FILES([
  Makefile
  src/Makefile
  test/Makefile
  ${qasmmakefile}
])
AC_OUTPUT
