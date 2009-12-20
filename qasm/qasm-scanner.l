%{
/*
 *
 * Pseudo assembler para maquina virtual QCode
 * Haga lo que quiera con ella, pero no hay garantias
 * Silvio Quadri (c) 2009
 *
 * */
#include <stdio.h>
#include <qcode.h>
#include "qasm-parser.h"
#include "qasm.h"

/*
 * Estas definiciones son para mantener el 
 * stack de llamados de defines y todo eso
 */

#define MAX_INCLUDE_DEPTH 32
YY_BUFFER_STATE   include_stack[MAX_INCLUDE_DEPTH];
char*             include_strings[MAX_INCLUDE_DEPTH];
int               include_lineno[MAX_INCLUDE_DEPTH];
int               include_stack_ptr = 0;
void              include_file( char* filename );


%}

%option batch
%option yylineno
%option case-insensitive

%x      string

WORD    [A-Za-z][A-Za-z0-9\_\-]*
NUM     [0-9]+

%%


^{WORD}\:       { qasmlval = (long)strdup(yytext); 
                  ((char*)(long)qasmlval)[yyleng - 1] = 0;
                  return  TOK_LAB; 
                }

^\.include[ \t]+\"[^\"\n]+\"[ \t]*\n { 
                  char* s;
                  s = strchr( yytext, '"' );
                  if( s ){
                     char* filename = strdup( s + 1 );
                     char* pos_comillas = strchr( filename, '"' );
                     if( pos_comillas ){
                        pos_comillas[0]  = 0;
                        include_file( filename );
                     }
                  }
                }

nop             { qasmlval = QCNOP; return TOK_INS; }
sto             { qasmlval = QCSTO; return TOK_INS; }
add             { qasmlval = QCADD; return TOK_INS; }
sub             { qasmlval = QCSUB; return TOK_INS; }
inc             { qasmlval = QCINC; return TOK_INS; }
dec             { qasmlval = QCDEC; return TOK_INS; }
mul             { qasmlval = QCMUL; return TOK_INS; }
div             { qasmlval = QCDIV; return TOK_INS; }
equ             { qasmlval = QCEQU; return TOK_INS; }
neq             { qasmlval = QCNEQ; return TOK_INS; }
lth             { qasmlval = QCLTH; return TOK_INS; }
leq             { qasmlval = QCLEQ; return TOK_INS; }
gth             { qasmlval = QCGTH; return TOK_INS; }
geq             { qasmlval = QCGEQ; return TOK_INS; }
psh             { qasmlval = QCPSH; return TOK_INS; }
pop             { qasmlval = QCPOP; return TOK_INS; }
jmp             { qasmlval = QCJMP; return TOK_INS; }
jpz             { qasmlval = QCJPZ; return TOK_INS; }
jnz             { qasmlval = QCJNZ; return TOK_INS; }
cll             { qasmlval = QCCLL; return TOK_INS; }
alt             { qasmlval = QCALT; return TOK_INS; }
clx             { qasmlval = QCCLX; return TOK_INS; }
ret             { qasmlval = QCRET; return TOK_INS; }

t{NUM}          { int i = atoi( yytext + 1 );
                  if( i > 256 - 16 ) REJECT;
                  qasmlval = i + 16;
                  return  TOK_TMP;
                }

r{NUM}          { int i = atoi( yytext + 1 );
                  if( i > 16 ) REJECT;
                  qasmlval = i; 
                  return  TOK_REG;
                }

{NUM}           { qasmlval = atoi( yytext ); 
                  return  TOK_NUM;
                }

\&{WORD}        { qasmlval = (long)strdup(yytext + 1); 
                  return  TOK_PTR; 
                }

{WORD}          { qasmlval = (long)strdup(yytext); 
                  return  TOK_WRD; 
                }

\"              { BEGIN(string); }
<string>[^\"]*  { qasmlval = (long)strdup(yytext); 
                  return TOK_STR;
                }
<string>\"      { BEGIN(INITIAL); }

[ \t\r]+        /* ignore whitespace */
\#[^\n]*        /* ignora comentario */
;[^\n]*         /* ignora comentario */
\n              { return  TOK_SEP; }
.               { return (int) yytext[0]; }

<<EOF>>         {
                  if( include_stack_ptr == 0 ) yyterminate(); 
                  else {
                    if( include_strings[include_stack_ptr] ){
                      free( include_strings[include_stack_ptr] );
                      include_strings[include_stack_ptr] = NULL;
                    }
                    include_stack_ptr --;
                    YY_BUFFER_STATE b = YY_CURRENT_BUFFER;
                    yy_switch_to_buffer( include_stack[include_stack_ptr] );
                    yylineno = include_lineno[include_stack_ptr];
                    yy_delete_buffer( b );
                  } 
                }

%%

void   qasm_scan_save( ){
  include_stack[include_stack_ptr]   = YY_CURRENT_BUFFER;
  if( qasm_filename )
    include_strings[include_stack_ptr] = strdup( qasm_filename );
  else
    include_strings[include_stack_ptr] = NULL;
  include_lineno[include_stack_ptr]  = yylineno;
  include_stack_ptr ++;
}


void   include_file( char* filename ){
  FILE* ff;
  // Listo ... verifico si hay lugar en el stack
  if( include_stack_ptr == MAX_INCLUDE_DEPTH ){
    qasmerror( "Insuficiente espacio para seguir incluyendo archivos" );
    return;
  }

  ff = fopen( filename, "r" );
  if( !ff ){
    char err[1024];
    if( strlen( filename ) > 128 ) filename[100] = '.';
    sprintf( err, "Error %d (%s) al abrir \"%s\"\n", errno, strerror( errno ), filename );
    qasmerror(err)  ;
    return;
  }


  qasmprintf( "Incluyendo : %s", filename );
  qasm_scan_save( );
  yy_switch_to_buffer( yy_create_buffer( ff, YY_BUF_SIZE ) );
  qasm_filename = filename;
  yylineno = 1;
  
}

  


