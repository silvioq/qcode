/*
 *
 * Pseudo assembler para maquina virtual QCode
 * Haga lo que quiera con ella, pero no hay garantias
 * Silvio Quadri (c) 2009
 *
 * */

/*
 * Caracteristicas del lenguaje:
 *
 * */


#ifndef  QASM_H
#define  QASM_H  1

#include <stdio.h>

extern  QCode*  qasm;

#define  QASM_DEBUG    1
#define  QASM_VERBOSE  2

int   qasm_parse_filename( char* filename, int flags );
int   qasm_parse( FILE* f, int flags );
char*             qasm_filename;

#endif
