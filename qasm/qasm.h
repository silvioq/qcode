/*
 *
 * Pseudo assembler para maquina virtual QCode
 * Haga lo que quiera con ella, pero no hay garantias
 * Silvio Quadri (c) 2009
 *
 * */





QCode*  qasm;

#define  QASM_DEBUG    1
#define  QASM_VERBOSE  2

int   qasm_parse( char* filename, int flags );

