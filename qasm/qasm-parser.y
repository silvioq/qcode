%{
/*
 *
 * Pseudo assembler para maquina virtual QCode
 * Haga lo que quiera con ella, pero no hay garantias
 * Silvio Quadri (c) 2009
 *
 * */


#include <qcode.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include "qasm.h"

#define YYSTYPE long
#define YYDEBUG 1

extern  int  qasmlineno;
extern FILE* qasmin;
int  qasm_verbose  = 0;


void yyerror(const char *str) { 
    fprintf(stderr,"error: %s (linea: %d)\n",str, qasmlineno); 
}

int qasmwrap() { return 1; } 


void  qasmprintf( char* format, ... ){
  if( !qasm_verbose ) return;
  va_list  a;
  va_start( a, format );
  printf( "%.4d: ", qasmlineno );
  vprintf( format, a );
  printf( "\n" );
  va_end( a );
}

%}

%token  TOK_STR  TOK_NUM
%token  TOK_INS
%token  TOK_LAB  TOK_REG  TOK_TMP

%start  command_list






%%


set_label:
     TOK_LAB   TOK_NUM {
          qcode_dcrlab_long( qasm, (char*)($1), $2 );
     } |
     TOK_LAB   TOK_STR {
          qcode_dcrlab_str( qasm, (char*)($1), (char*)($2) );
     } |
     TOK_LAB   { 
          qcode_crlab ( qasm, (char*)($1) );
     } 
     ;

instruction_op:
     TOK_INS   TOK_REG  ','  TOK_TMP  {
          qcode_op( qasm, $1, $2, $4 ); 
     } |
     TOK_INS   TOK_TMP  ','  TOK_REG  {
          qcode_op( qasm, $1, $2, $4 ); 
     } 
     ;
    
instruction_call:
     TOK_INS   TOK_LAB   {
          qcode_opnlab( qasm, $1, (char*)($2) );
     }
     ;


instruction_ret:
     TOK_INS  { 
          qcode_op( qasm, $1, 0, 0 );
     }
     ;

instruction:
     instruction_op                   |
     instruction_call                 |
     instruction_ret                  ;


command:
      set_label     |
      instruction    ;


command_list:  command      |
               command_list command ;
  











%%

int   qasm_parse( char* filename, int flags ){
    if( flags | QASM_DEBUG ) yydebug = 1;
    if( flags | QASM_VERBOSE ) qasm_verbose = 1;

    qasmin = fopen( filename, "r" );
    if( !qasmin ){
        qasmprintf( "Error %s (%d) al abrir %s\n", strerror( errno ), errno, filename );
        return NULL;
    }

    if( yyparse() ){
        puts( "Salimos por error!" );
        return 0;
    }
    if( qasm_verbose )printf( "Total analizado: %d lineas\n", qasmlineno );
    fclose( qasmin );
}
