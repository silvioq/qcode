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

{WORD}          { qasmlval = (long)strdup(yytext); 
                  return  TOK_WRD; 
                }

\"              { BEGIN(string); }
<string>[^\"]*  { qasmlval = (long)strdup(yytext); 
                  return TOK_STR;
                }
<string>\"      { BEGIN(INITIAL); }

[ \n\t\r]+      /* ignore whitespace */
\#[^\n]*        /* ignora comentario */
;[^\n]*         /* ignora comentario */
.               { return (int) yytext[0]; }


%%
