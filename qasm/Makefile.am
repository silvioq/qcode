
bin_PROGRAMS = qasm

qasm_SOURCES = qasm.c qasm-parser.y qasm-scanner.l qasm.h
qasm_LDADD = $(top_builddir)/src/libqcode.la
qasm_DEPENDENCIES = $(top_builddir)/src/libqcode.la

qasm-scanner.l: qasm-parser.c
qasm.c: qasm-parser.c qasm-parser.h

AM_YFLAGS = -d -p qasm
AM_LFLAGS = --prefix qasm -o lex.yy.c
AM_CFLAGS = -I$(top_srcdir)/src


TESTS_ENVIRONMENT = $(builddir)/qasm -vr
TESTS = test/*.qasm

EXTRA_DIST = test/*.qasm

