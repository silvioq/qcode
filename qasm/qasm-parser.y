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
#include <string.h>
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
  printf( "(%s)%.4d: ", ( qasm_filename ? qasm_filename : "-" ),  qasmlineno );
  vprintf( format, a );
  printf( "\n" );
  va_end( a );
}


unsigned char select_inst( unsigned char inst, int op1_type, int op2_type ){

    if( inst != QCSTO ) return inst;
    if( op1_type == TOK_REG && op2_type == TOK_REG ) return QCSTO;
    if( op1_type == TOK_REG && op2_type == TOK_NUM ) return QCSTI;
    if( op1_type == TOK_REG && op2_type == TOK_STR ) { 
       yyerror( "Error interno (TOK_REG = TOK_STR)" ); return QCNOP;
    }
    if( op1_type == TOK_REG && op2_type == TOK_WRD ) return QCSTP;
    if( op1_type == TOK_REG && op2_type == TOK_PTR ) return QCSTM;
    if( op1_type == TOK_PTR && op2_type == TOK_REG ) return QCSTD;
    if( op1_type == TOK_WRD && op2_type == TOK_REG ) return QCSTD;
    
    yyerror( "Error interno" ); return QCNOP;

}



%}

%token  TOK_STR  TOK_NUM  TOK_WRD  TOK_PTR
%token  TOK_INS  TOK_SEP
%token  TOK_LAB  TOK_REG  TOK_TMP

%start  command_list






%%

coma_o_blanco:
    ',' | ;


reg_or_tmp:
    TOK_REG  { $$ = $1 ; } | TOK_TMP  { $$ = $1; };


set_label:
     TOK_LAB   TOK_NUM {
          qasmprintf( "Estableciendo etiqueta %s => %d", ((char*)$1), $2 );
          qcode_dcrlab_long( qasm, (char*)($1), $2 );
     } |
     TOK_LAB   TOK_STR {
          qasmprintf( "Estableciendo etiqueta %s => %s", ((char*)$1), ((char*)$2) );
          qcode_dcrlab_str( qasm, (char*)($1), (char*)($2) );
     } |
     TOK_LAB   { 
          qasmprintf( "Estableciendo etiqueta %s", ((char*)$1) );
          int label = qcode_crlab ( qasm, ((char*)$1) );
          qcode_label ( qasm, label );
     } 
     ;

instruction_op:
     TOK_INS   reg_or_tmp  coma_o_blanco   reg_or_tmp  {
          qcode_op( qasm, select_inst( $1, TOK_REG, TOK_REG ), $2, $4 ); 
     } |
     TOK_INS   reg_or_tmp  coma_o_blanco   TOK_WRD     {
          qasmprintf( "Usando etiqueta => %s", ((char*)$4) );
          int label = qcode_slab( qasm, ((char*)$4) );
          if( !label ) yyerror( "Etiqueta inexistente" );
          qcode_op( qasm, select_inst( $1, TOK_REG, TOK_WRD ), $2, label ); 
     } |
     TOK_INS   reg_or_tmp  coma_o_blanco   TOK_PTR     {
          qasmprintf( "Usando etiqueta => & %s", ((char*)$4) );
          int label = qcode_slab( qasm, ((char*)$4) );
          if( !label ) yyerror( "Etiqueta inexistente" );
          qcode_op( qasm, select_inst( $1, TOK_REG, TOK_PTR ), $2, label ); 
     } |
     TOK_INS   reg_or_tmp  coma_o_blanco   TOK_NUM     {
          qcode_op( qasm, select_inst( $1, TOK_REG, TOK_NUM ), $2, $4 ); 
     } |
     TOK_INS   reg_or_tmp  coma_o_blanco   TOK_STR     {
          int  label = qcode_dcrlab_str( qasm, unnamed_label, ((char*)($4)) );
          qcode_op( qasm, QCSTP, 0, label );
     } |
     TOK_INS   TOK_PTR     coma_o_blanco   reg_or_tmp  {
          qcode_op( qasm, select_inst( $1, TOK_PTR, TOK_REG ), $4, $2 ); 
     } |
     TOK_INS   TOK_WRD     coma_o_blanco   reg_or_tmp  {
          qcode_op( qasm, select_inst( $1, TOK_WRD, TOK_REG ), $4, $2 ); 
     } |
     TOK_INS   reg_or_tmp {
          qcode_op( qasm, $1, $2, 0 );
     }
     
     ;
    
instruction_call:
     TOK_INS   TOK_WRD   {
          qasmprintf( "Llamando etiqueta %s", ((char*)$2) );
          qcode_opnlab( qasm, $1, ((char*)$2) );
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


command:  |
      set_label     |
      instruction    ;


command_list:                 command      |
               command_list TOK_SEP command  ;
  









%%

int   qasm_parse_filename( char* filename, int flags ){
    FILE* ff = fopen( filename, "r" );
    int  ret;
    if( !ff ){
        qasmprintf( "Error %d (%s) al abrir \"%s\"\n", errno, strerror( errno ), filename );
        return 0;
    }

    qasm_filename = filename;
    ret = qasm_parse( ff, flags );
    fclose( ff );
    return ret;

}


int   qasm_parse( FILE* f, int flags ){
    if( flags & QASM_VERBOSE ) qasm_verbose = 1; else qasm_verbose = 0;
    if( qasm_verbose ) printf( "En modo verbose!\n" );
#if YYDEBUG==1
    if( flags & QASM_DEBUG ) yydebug = 1; else yydebug = 0;
    if( qasm_verbose && yydebug ) printf( "En modo debug también (%d)\n", flags );
#endif

    qasmin = f;
    qasm = qcode_new();

    if( yyparse() ){
        puts( "Salimos por error!" );
        return 0;
    }
    if( qasm_verbose )printf( "Total analizado: %d lineas\n", qasmlineno );
    return 1;
}
