/*
 *
 * Libreria máquina virtual de proposito general
 * Haga lo que quiera con ella, pero no hay garantias
 * Silvio Quadri (c) 2009
 *
 * */

/*
 * TODO: Implementar todas las instrucciones de la maquina virtual
 * TODO: Dump de la maquina
 * */



#include  <stdlib.h>
#include  <string.h>
#include  <assert.h>
#include  <stdio.h>
#include  "qcode.h"
#include  "md5.h"


// #define  DEBUG_ALLOC

#ifdef   DEBUG_ALLOC
static inline void* qcode_debug_alloc(size_t size){
    void* r = malloc(size);
    printf( "DEBUG_ALLOC: %p(%d)\n", r, (int)size );
    return r;
}
#define  ALLOC(s)  qcode_debug_alloc(s)
#define  REALLOC(p,s) ({  \
    void *r =  realloc(p, s);\
    printf( "DEBUG_RELOC: " __FILE__ ":%d %p = %p(%d)\n", __LINE__, p, r, (int)s );\
    r;} )
      
#define  FREE(p){\
        printf( "DEBUG_FREE : %p\n", p ); free(p ); \
    }
#else

#ifndef  ALLOC
#define  ALLOC(s)  malloc(s)
#endif

#ifndef  REALLOC
#define  REALLOC(p,s)  realloc(p,s)
#endif

#ifndef  FREE
#define  FREE(p)  free(p)
#endif

#endif

#define  ALLOC_K  16


static  QCodeLab*  qcode_search_label(QCode* qcode, int label );
static  QCodeLab*  qcode_new_label(QCode* qcode, char* label_name );

#define  NUMLABEL(numlab, labelname) \
  { numlab = md5_mem( label_name, strlen( label_name ) );\
        if( numlab > 0 ) numlab = -numlab; }

#define  DEFOP1   1
#define  DEFOP2   2
#define  DEFLBL   4
#define  DEFDAT   8
#define  DEFOPX   16

static   char   valid_inst [] = {
    0,               //  QCNOP 
    DEFOP1 | DEFOP2, //  QCSTO
    DEFOP1 | DEFOPX, //  QCSTI
    DEFOP1 | DEFDAT, //  QCSTP
    DEFOP1 | DEFDAT, //  QCSTM
    DEFOP1 | DEFDAT, //  QCSTD
    DEFOP1 | DEFOP2, //  QCADD
    DEFOP1 | DEFOP2, //  QCSUB
    DEFOP1         , //  QCINC
    DEFOP1         , //  QCDEC
    DEFOP1 | DEFOP2, //  QCMUL
    DEFOP1 | DEFOP2, //  QCDIV
    DEFOP1 | DEFOP2, //  QCEQU
    DEFOP1 | DEFOP2, //  QCNEQ
    DEFOP1 | DEFOP2, //  QCLTH
    DEFOP1 | DEFOP2, //  QCLEQ
    DEFOP1 | DEFOP2, //  QCGTH
    DEFOP1 | DEFOP2, //  QCGEQ
    DEFOP1         , //  QCPSH
    DEFOP1         , //  QCPOP
    DEFLBL         , //  QCJMP
    DEFLBL         , //  QCJPZ
    DEFLBL         , //  QCJNZ
    DEFLBL         , //  QCCLL
    DEFOP1         , //  QCALT
    DEFLBL         , //  QCCLX
    0              , //  QCRET
    0 };

static   char*  name_inst [] = {
    "QCNOP",
    "QCSTO",
    "QCSTI",
    "QCSTP",
    "QCSTM",
    "QCSTD",
    "QCADD",
    "QCSUB",
    "QCINC",
    "QCDEC",
    "QCMUL",
    "QCDIV",
    "QCEQU",
    "QCNEQ",
    "QCLTH",
    "QCLEQ",
    "QCGTH",
    "QCGEQ",
    "QCPSH",
    "QCPOP",
    "QCJMP",
    "QCJPZ",
    "QCJNZ",
    "QCCLL",
    "QCALT",
    "QCCLX",
    "QCRET",
    0 };
/*
 * Creacion del codigo
 * */

QCode*    qcode_new(){
    QCode* c = ALLOC( sizeof( QCode ) );
    memset( c, 0, sizeof( QCode ) );
    return  c;
}


/*
 * Libero mi codigo, para siempre ...
 * */
void      qcode_free  ( QCode* qcode ){
    if( qcode->lab_list ) FREE( qcode->lab_list );
    if( qcode->data ) FREE( qcode->data );
    FREE(qcode);
}

/* 
 * Agrego una etiqueta en la posicion actual
 * */
void      qcode_label ( QCode* qcode, int   label ){
    // busco mi querida etiqueta (ya la tuve que 
    // haber creado, de alguna u otra forma)
    QCodeLab* l;
    assert( l = qcode_search_label( qcode, label ) );
    l->inst = qcode->inst_count;
    l->flag |= QCODE_LABFLAG_SET;
}

/*
 * Seteo una funcion externa, asignandola a 
 * la etiqueta
 * */
int       qcode_xcrlab( QCode* qcode, char* label_name, qcode_extfunc f ){
    QCodeLab*  l = qcode_new_label( qcode, label_name ) ;
    l->flag = QCODE_LABFLAG_EXT | QCODE_LABFLAG_SET;
    l->ext = f;
    return l->lab;
}

/* 
 * Creo una etiqueta nueva, sin posicion, ni nada
 * */
int       qcode_crlab ( QCode* qcode, char* label_name ){
    QCodeLab*  l = qcode_new_label( qcode, label_name ) ;
    l->flag = QCODE_LABFLAG_INS;
    return  l->lab;
}

/* 
 * Creo una etiqueta nueva, que apunta a un cacho de memoria
 * */
int       qcode_dcrlab( QCode* qcode, char* label_name, void*  data, int size ){
    QCodeLab*  l = qcode_new_label( qcode, label_name ) ;
    if( qcode->data_alloc < qcode->data_size + size ){
        int alloc = qcode->data_alloc + size + ALLOC_K * 4;
        alloc = alloc - ( alloc % ( ALLOC_K * 4 ) );
        qcode->data_alloc = alloc;
        if( qcode->data )
            qcode->data = REALLOC( qcode->data, qcode->data_alloc );
        else
            qcode->data = ALLOC( qcode->data_alloc );
    }
    memcpy( qcode->data + qcode->data_size, data, size );

    l->flag = QCODE_LABFLAG_DAT | QCODE_LABFLAG_SET;
    l->data = qcode->data_size;

    qcode->data_size += size;
    return  l->lab;
}


int       qcode_dcrlab_long( QCode* qcode, char* label_name, long data ){
    return  qcode_dcrlab( qcode, label_name, &data, sizeof( long ) );
}
int       qcode_dcrlab_str( QCode* qcode, char* label_name, char* data ){
    return  qcode_dcrlab( qcode, label_name, data, strlen( data ) + 1 );
}

/*
 * Esta funcion retorna el numero de etiqueta nombrada.
 * Si no existe la etiqueta, retonra 0
 * */

int       qcode_slab  ( QCode* qcode, char* label_name ){
    int  numlab;
    assert( label_name != unnamed_label );
    NUMLABEL( numlab, label_name );
    QCodeLab* l = qcode_search_label( qcode, numlab );
    return( l ? numlab : 0 );
}

/*
 *
 * Esta funcion interna me permite crear una etiqueta 
 * en forma generica
 *
 * */
static  QCodeLab*  qcode_new_label(QCode* qcode, char* label_name ){
    int  numlab;
    QCodeLab*  l ;

    if( !qcode->lab_list ) {
        assert( qcode->lab_list  = ALLOC( sizeof( QCodeLab ) * ALLOC_K ) );
        qcode->lab_alloc = ALLOC_K ;
    } else if( qcode->lab_count ==  qcode->lab_alloc ) {
        qcode->lab_alloc += ALLOC_K;
        assert( qcode->lab_list  = REALLOC( qcode->lab_list, sizeof( QCodeLab ) * qcode->lab_alloc ) );
    }


    // Obtengo numero de etiqueta
    if( label_name == unnamed_label ){
        numlab = qcode->lab_count;
    } else {
        NUMLABEL( numlab, label_name );
    }
    l = &(qcode->lab_list[qcode->lab_count++]);

    memset( l, 0, sizeof( QCodeLab ) );
    l->lab  = numlab;
    return l;
}



/*
 * Busca una etiqueta a partir del numero ... si
 * no encuentra nada, retorna nulo
 * */
static  QCodeLab*  qcode_search_label(QCode* qcode, int label ){
    int i;
    for( i = 0; i < qcode->lab_count; i ++ ){
        register QCodeLab* l = &( qcode->lab_list[i] );
        if( l->lab == label ) return l;
    }
    return NULL;
}

QCodeIns*  qcode_new_inst( QCode* qcode ){
    QCodeIns*  i;
    // Bien, por ahora, podemos agrandar nuestro array
    if( !qcode->inst_list ){
        assert( qcode->inst_list  = ALLOC( sizeof( QCodeIns ) * ALLOC_K ) );
        qcode->inst_alloc = ALLOC_K;
    } else if( qcode->inst_count >= qcode->inst_alloc ){
        qcode->inst_alloc += ALLOC_K;
        assert( qcode->inst_list  = REALLOC( qcode->inst_list, sizeof( QCodeIns ) * qcode->inst_alloc ) );
    }

    i = &(qcode->inst_list[qcode->inst_count++]);
    memset( i, 0, sizeof( QCodeIns ) );
    return i;
}


/* 
 *
 * Agrego instruccion
 *
 *
 * */
void      qcode_op    ( QCode* qcode, unsigned char inst, unsigned char op1, long op2 ){
    QCodeIns*  i;
    // QRET es y será la última instrucción
    assert( inst <= QCRET );

    // Si la operacion requiere label, no deberia 
    // usar esta funcion
    assert( !( valid_inst[inst] & DEFLBL ) );

    // Listo, creo la instruccion
    i = qcode_new_inst( qcode );

    i->inst = inst;
    if( valid_inst[inst] & DEFOP1 ){
        i->op1 = op1;
    } 
    if( valid_inst[inst] & DEFOP2 ){
        i->op2 = (unsigned char)op2;
    }
    if( valid_inst[inst] & DEFOPX ){
        i->extval = op2;
    }
    if( valid_inst[inst] & DEFDAT ){
        i->data   = op2;
    }
    

}


/*
 * Agrega una instruccion de llamado
 * */
void      qcode_oplab ( QCode* qcode, char inst, int label ){
    QCodeIns*  i;
    // QRET es y será la última instrucción
    assert( inst <= QCRET );

    // La operacion deberia ser de etiqueta
    assert( ( valid_inst[(int)inst] & DEFLBL ) );

    // La operacion no deberia ser con operador
    assert( !( valid_inst[(int)inst] & DEFOP1 ) );

    // Listo, creo la instruccion
    i = qcode_new_inst( qcode );

    i->inst = inst;
    i->code = label;
}

void      qcode_opnlab( QCode* qcode, char inst, char* label_name ){
    int  numlabel;
    NUMLABEL( numlabel, label_name );
    qcode_oplab( qcode, inst, numlabel );
}

/*
 * Dada una etiqueta, devuelve el PC o la instruccion
 * a donde apunta.
 * Si la instruccion no ha sido establecida, o no 
 * existe la etiqueta, se devuelve un error
 * */

int       qcode_label_getpc( QCode* qcode, int label ){
    // busco mi querida etiqueta (ya la tuve que 
    // haber creado, de alguna u otra forma)
    QCodeLab* l;
    l = qcode_search_label( qcode, label );
    if( !l ) return -1;

    if( !(QCODE_LABFLAG_CHECK(l->flag) == QCODE_LABFLAG_INS ) ) return -1;
    if( !(l->flag & QCODE_LABFLAG_SET ) ) return -1;

    return l->inst;



}

/*
 * Esta es una función interna, que tiene como objetivo recorrer
 * todas las etiquetas y hacer el "resolvimiento" efectivo!
 * Lo que tengo en inst.code de cada instrucción, antes de
 * resolver las etiquetas es simplemente el id de la etiqueta
 * Lo que hago es buscarlo dentro de las etiquetas y
 * reemplazarlo por el puntero a la instrucción
 * a la que hay que saltar. Por otro lado, 
 * se puede tratar de una etiqueta externa.
 *
 * El parametro flags indica si se procesaran las etiquetas
 * internas o externas (QCODE_LABRESOLVED y QCODE_EXTRESOLVED
 * respectivamente)
 * */
void   qcode_resolvelabels( QCode* qcode, char flags ){
    int  i;
    for( i = 0; i < qcode->inst_count; i ++ ){
        char  valid;
        QCodeLab* qlab;
        valid = valid_inst[qcode->inst_list[i].inst];
        if( !(valid & ( DEFLBL | DEFDAT ) ) ) continue;
        // Es para resolver!

        if( (!( flags & QCODE_LABRESOLVED )) && ( qcode->inst_list[i].inst == QCCLX ) ) continue;
        if( (!( flags & QCODE_EXTRESOLVED )) && ( qcode->inst_list[i].inst != QCCLX ) ) continue;

        assert( qlab = qcode_search_label( qcode, qcode->inst_list[i].code ) );
        if( QCODE_LABFLAG_CHECK( qlab->flag ) == QCODE_LABFLAG_INS ){
            qcode->inst_list[i].code = qlab->inst;
        } else if ( QCODE_LABFLAG_CHECK( qlab->flag ) == QCODE_LABFLAG_EXT ){
            assert( qcode->inst_list[i].inst == QCCLX );
            qcode->inst_list[i].extval = (long)qlab->ext;
        } else if ( QCODE_LABFLAG_CHECK( qlab->flag ) == QCODE_LABFLAG_DAT ){
            assert( valid_inst[qcode->inst_list[i].inst] & DEFDAT );
            qcode->inst_list[i].data   = (long)qlab->data;
        } else {
            assert( !"Tipo de etiqueta invalida" );
        }
        qlab->flag |= QCODE_LABFLAG_REF;  // Hago referencia!
    }
    qcode->flags |= flags;
}


static inline int qcode_vm_sto( QCodeVM* vm, unsigned char op1, unsigned char op2 );
static inline int qcode_vm_sti( QCodeVM* vm, unsigned char op1, long op2 );
static inline int qcode_vm_stm( QCodeVM* vm, unsigned char op1, long op2 );
static inline int qcode_vm_std( QCodeVM* vm, unsigned char op1, long op2 );
static inline int qcode_vm_inc( QCodeVM* vm, unsigned char op1, int incdec );
static inline int qcode_vm_opp( QCodeVM* vm, unsigned char inst, unsigned char op1, unsigned char op2 );
static inline int qcode_vm_add( QCodeVM* vm, unsigned char inst, unsigned char op1, unsigned char op2 );
static inline int qcode_vm_ret( QCodeVM* vm );
static inline int qcode_vm_cll( QCodeVM* vm, int code );
static inline int qcode_vm_psh( QCodeVM* vm, unsigned char op1 );
static inline int qcode_vm_pop( QCodeVM* vm, unsigned char op1 );
static inline int qcode_vm_jmp( QCodeVM* vm, unsigned char inst, int pc );
static inline int qcode_vm_clx( QCodeVM* vm, qcode_extfunc f );


/*
 *
 * Esta parte es la mas diviertida, si es que alguna puede serlo
 * Es la corrida principal del programa !!!!
 *
 *
 * */
int    qcode_run( QCode* qcode, long*  ret ){
    return  qcode_runargs( qcode, ret, 0, 0, NULL );
}
int    qcode_runargs( QCode* qcode, long*  ret, int pc, int argc, long* argv ){
    int i;
    QCodeVM* vm;
    int   retrun = QCODE_RUNNING;

    // Si no esta resuelto el tema de los labels,
    // Obviamente, hay que resolverlo!
    if( ! (qcode->flags & QCODE_LABRESOLVED ) ) qcode_resolvelabels( qcode, QCODE_LABRESOLVED );
    if( ! (qcode->flags & QCODE_EXTRESOLVED ) ) qcode_resolvelabels( qcode, QCODE_EXTRESOLVED );

    // Voy a crear la maquina virtual
    vm = ALLOC( sizeof( QCodeVM ) );
    memset( vm, 0, sizeof( QCodeVM ) );

    vm->pc = pc;
    
    // Genero el stack
    vm->stackp = ALLOC( sizeof( long ) * ALLOC_K );
    vm->stackp_alloc = ALLOC_K;

    // Meto los argumentos en el stack
    for( i = 0; i < argc; i ++ ){ qcode_push( vm, argv[i] ); }

    // Pongo los punteros en la data
    if( qcode->data ){
        if( QCODE_ISVOLATILE( qcode ) ){
            vm->data = ALLOC( qcode->data_size );
            memcpy( vm->data, qcode->data, qcode->data_size );
        } else {
            vm->data = qcode->data;
        }
        vm->data_size = qcode->data_size;
    }

    // Arranco!
    while( retrun == QCODE_RUNNING ){
        QCodeIns* inst;
        
        // Controlo que no me vaya del programa
        if( vm->pc > qcode->inst_count ){
            retrun = QCODE_RUNOUTOFP;
            break;
        }

        inst = &( qcode->inst_list[vm->pc] );
        if( QCODE_ISDEBUG(qcode) ) printf( "Ejecutando %d (%d) %s\n", inst->inst, vm->pc, name_inst[inst->inst] );
        switch(inst->inst){
            case QCNOP:
                break;
            case QCSTO:
                retrun = qcode_vm_sto( vm, inst->op1, inst->op2 );
                break;
            case QCSTI:
                retrun = qcode_vm_sti( vm, inst->op1, inst->extval );
                break;
            case QCSTD:
                retrun = qcode_vm_std( vm, inst->op1, inst->data );
                break;
            case QCSTP:
                retrun = qcode_vm_sti( vm, inst->op1, inst->data );
                break;
            case QCSTM:
                retrun = qcode_vm_stm( vm, inst->op1, inst->data );
                break;
            case QCDEC:
                retrun = qcode_vm_inc( vm, inst->op1, -1 );
                break;
            case QCINC:
                retrun = qcode_vm_inc( vm, inst->op1, 1 );
                break;
            case QCLTH:
            case QCLEQ:
            case QCGTH:
            case QCGEQ:
            case QCEQU:
                retrun = qcode_vm_opp( vm, inst->inst, inst->op1, inst->op2 );
                break;
            case QCADD:
            case QCSUB:
            case QCMUL:
            case QCDIV:
                retrun = qcode_vm_add( vm, inst->inst, inst->op1, inst->op2 );
                break;
            case QCJMP:
            case QCJPZ:
            case QCJNZ:
                retrun = qcode_vm_jmp( vm, inst->inst, inst->code );
                break;
            case QCCLX:
                retrun = qcode_vm_clx( vm, (qcode_extfunc) inst->extval );
                break;
            case QCPSH:
                retrun = qcode_vm_psh( vm, inst->op1 );
                break;
            case QCPOP:
                retrun = qcode_vm_pop( vm, inst->op1 );
                break;
            case QCCLL:
                retrun = qcode_vm_cll( vm, inst->code );
                break;
            case QCRET:
                retrun = qcode_vm_ret( vm );
                break;
            default:
                if( inst->inst <= QCRET ){
                    retrun = QCODE_RUNNOTIMP;
                } else {
                    retrun = QCODE_RUNINVINS;
                }
    
        }

        if( QCODE_RUNNING ) vm->pc ++;
        
    }

    FREE( vm->stackp );
    if( vm->stackt ) FREE( vm->stackt );
    if( vm->stackc ) FREE( vm->stackc );
    if( vm->data && QCODE_ISVOLATILE( qcode ) ) FREE( vm->data );
    if( ret ) *ret = vm->r[0];
    FREE( vm );
    
    if( QCODE_ISDEBUG(qcode)) printf( "Saliendo con %d\n", retrun ); 
    return retrun;
}

/*
 * PUSH. Esto es para manejar en forma interna o externa
 * el stack de la maquina virtual.
 * */
void      qcode_push  ( QCodeVM* vm, long l ){
    if( vm->stackp_count + 1 > vm->stackp_alloc ){
        vm->stackp_alloc += ALLOC_K;
        vm->stackp = REALLOC( vm->stackp, vm->stackp_alloc  );
    }
    vm->stackp[vm->stackp_count++] = l;
}

long      qcode_pop   ( QCodeVM* vm ){
    if( vm->stackp_count == 0 ) return 0;
    return vm->stackp[--vm->stackp_count];
}

void*    qcode_pop_ptr( QCodeVM* vm ){
    void* ret;
    if( vm->stackp_count == 0 ) return NULL;
    assert(vm->data);
    long x = vm->stackp[--vm->stackp_count];
    ret = ((char*)vm->data + x);
    return ret;
}

#define  AJUSTA_T(opt)  {\
        if( opt >= vm->stackt_alloc ) {\
            int  alloc =  opt + ALLOC_K;  \
            alloc =(  alloc - ( alloc % ALLOC_K ) ) ; \
            vm->stackt = ( vm->stackt ? REALLOC( vm->stackt, alloc * sizeof( long ) ) : ALLOC( alloc * sizeof( long ) ) );\
            vm->stackt_alloc = alloc;\
        } \
        vm->stackt_count = opt + 1;\
    }

/*
 *
 *
 *
 * A partir de aqui, comienzan las funciones para trabajar
 * con las operaciones propias de la maquina virtual
 *
 *
 *
 *
 * */


/*
 * Instruccion QCSTO
 * Pasa el valor del operando 1 al operando 2
 * */
int qcode_vm_sto( QCodeVM* vm, unsigned char op1, unsigned char op2 ){
    long  val2;
    // Controlo si es temporal 2
    if( op2 >= 16 ){
        if( op2 - 16 >= vm->stackt_count ) return QCODE_RUNOUTOFT;
        val2 = vm->stackt[op2 - 16];
    } else {
        val2 = vm->r[(int)op2];
    }

    // Controlo si es temporal 1
    if( op1 >= 16 ){
        if( op1 - 16 >= vm->stackt_count ){ AJUSTA_T( op1 - 16 ); }
        vm->r[0] = ( vm->stackt[op1 - 16] = val2 );
    } else {
        vm->r[0] = ( vm->r[(int)op1] = val2 );
    }
    return  QCODE_RUNNING;
}

/* 
 * Instruccion QCSTI 
 * Instruccion QCSTP 
 * Toma el valor del operando 2 y lo mete dentro del operando 1
 * */

int qcode_vm_sti( QCodeVM* vm, unsigned char op1, long op2 ){
    // Controlo si es temporal 1
    if( op1 >= 16 ){
        if( op1 - 16 >= vm->stackt_count ){ AJUSTA_T( op1 - 16 ); }
        vm->r[0] = ( vm->stackt[op1 - 16] = op2 );
    } else {
        vm->r[0] = ( vm->r[(int)op1] = op2 );
    }
    return  QCODE_RUNNING;
}

/* 
 * Instruccion QCSTM
 * Toma el valor al que apunta la etiqueta dentro de la memoria
 * y lo mete dentro del operando 1
 * */

int qcode_vm_stm( QCodeVM* vm, unsigned char op1, long op2 ){
    // Controlo si el puntero es bueno ...
    if( op2 >= vm->data_size ){ return  QCODE_RUNOUTOFM; }

    // Controlo si es temporal 1
    if( op1 >= 16 ){
        if( op1 - 16 >= vm->stackt_count ){ AJUSTA_T( op1 - 16 ); }
        vm->r[0] = ( vm->stackt[op1 - 16] = ((long*)((char*)vm->data + op2))[0] );
    } else {
        vm->r[0] = ( vm->r[(int)op1]      = ((long*)((char*)vm->data + op2))[0] );
    }
    return  QCODE_RUNNING;

}

/* 
 * Instruccion QCSTD
 * Toma el valor al que apunta la etiqueta dentro de la memoria
 * y le mete dentro el valor del operando 1
 * */

int qcode_vm_std( QCodeVM* vm, unsigned char op1, long op2 ){
    // Controlo si el puntero es bueno ...
    if( op2 >= vm->data_size ){ return  QCODE_RUNOUTOFM; }

    // Controlo si es temporal 1
    if( op1 >= 16 ){
        if( op1 - 16 >= vm->stackt_count ){ AJUSTA_T( op1 - 16 ); }
        vm->r[0] = ((long*)((char*)vm->data + op2))[0] = vm->stackt[op1 - 16] ;
    } else {
        vm->r[0] = ((long*)((char*)vm->data + op2))[0] = vm->r[(int)op1] ;
    }
    return  QCODE_RUNNING;

}

/*
 * Instruccion QCLTH QCLEQ QGTH QLEQ
 *
 * */
int qcode_vm_opp( QCodeVM* vm, unsigned char inst, unsigned char op1, unsigned char op2 ){

    long  val2, val1;
    // Valor 1
    if( op1 >= 16 ){
        if( op1 - 16 >= vm->stackt_count ) return QCODE_RUNOUTOFT;
        // printf( "OPP1: %d (%d) \n", vm->stackt[op1 - 16], op1 - 16 );
        val1 = vm->stackt[op1 - 16];
    } else {
        val1 = vm->r[(int)op1];
    }

    // Valor 2
    if( op2 >= 16 ){
        if( op2 - 16 >= vm->stackt_count ) return QCODE_RUNOUTOFT;
        // printf( "OPP2: %d (%d) \n", vm->stackt[op2 - 16], op2 - 16 );
        val2 = vm->stackt[op2 - 16];
    } else {
        val2 = vm->r[(int)op2];
    }


    switch(inst){
        case QCLTH:
            vm->r[0] = ( val1 < val2 );
            break;
        case QCLEQ:
            vm->r[0] = ( val1 <= val2 );
            break;
        case QCGTH:
            vm->r[0] = ( val1 > val2 );
            break;
        case QCGEQ:
            vm->r[0] = ( val1 >= val2 );
            break;
        case QCEQU:
            vm->r[0] = ( val1 == val2 );
            break;
          
        default:
            return  QCODE_RUNINVINS;
    }
    return  QCODE_RUNNING;

}

/*
 * Instrucciones ...
 *             QCADD QCSUB
 *             QCMUL QCDIV 
 * */
#define  OPERAR(x) \
        switch(inst){ \
        case  QCADD: \
            vm->r[0] = ( x += val2 ); \
            break; \
        case  QCSUB: \
            vm->r[0] = ( x -= val2 ); \
            break; \
        case  QCMUL: \
            vm->r[0] = ( x *= val2 ); \
            break; \
        case  QCDIV: \
            vm->r[0] = ( x /= val2 ); \
            break; \
        default: \
            return  QCODE_RUNINVINS; \
        }

int qcode_vm_add( QCodeVM* vm, unsigned char inst, unsigned char op1, unsigned char op2 ){
    long  val2;
    // Controlo si es temporal 2
    if( op2 >= 16 ){
        if( op2 - 16 >= vm->stackt_count ) return QCODE_RUNOUTOFT;
        // printf( "VAL: %d (%d) \n", vm->stackt[op2 - 16], op2 - 16 );
        val2 = vm->stackt[op2 - 16];
    } else {
        val2 = vm->r[(int)op2];
    }

    // Valor 1
    if( op1 >= 16 ){
        if( op1 - 16 >= vm->stackt_count ) return QCODE_RUNOUTOFT;
        OPERAR(vm->stackt[op1 - 16]); 
    } else {
        OPERAR(vm->r[(int)op1]); 
    }
    return QCODE_RUNNING;
}

/*
 * Instrucciones QCINC y QCDEC
 * */
int qcode_vm_inc( QCodeVM* vm, unsigned char op1, int incdec ){
    // Valor 1
    if( op1 >= 16 ){
        if( op1 - 16 >= vm->stackt_count ) return QCODE_RUNOUTOFT;
        vm->r[0] = ( vm->stackt[op1 - 16] += incdec );
    } else {
        vm->r[0] = ( vm->r[(int)op1] += incdec );
    }
    return QCODE_RUNNING;

}


int qcode_vm_psh( QCodeVM* vm, unsigned char op1 ){
    // Controlo si es temporal 1
    if( op1 >= 16 ){
        if( op1 - 16 >= vm->stackt_count ) return QCODE_RUNOUTOFT;
        vm->r[0] = vm->stackt[op1 - 16];
        qcode_push( vm, vm->stackt[op1 - 16] );
    } else {
        vm->r[0] = vm->r[(int)op1];
        qcode_push( vm, vm->r[(int)op1] );
    }
    return  QCODE_RUNNING;

}

/*
 * Instruccion QCPOP
 * */
int qcode_vm_pop( QCodeVM* vm, unsigned char op1 ){
    if( vm->stackp_count == 0 ) return QCODE_RUNOUTOFS;
    // Controlo si es temporal 1
    if( op1 >= 16 ){
        if( op1 - 16 >= vm->stackt_count ){ AJUSTA_T( op1 - 16 ); }
        vm->r[0] = ( vm->stackt[op1 - 16] = qcode_pop( vm ) );
        // printf( "Lo que obtuve %d\n", (int)vm->r[0] );
    } else {
        vm->r[0] = ( vm->r[(int)op1] = qcode_pop( vm ) );
    }
    return  QCODE_RUNNING;

}


/* 
 * Instrucciones JMP JPZ y JNZ
 * */
int qcode_vm_jmp( QCodeVM* vm, unsigned char inst, int pc ){

    // Controlo la instruccion
    if( inst == QCJPZ && vm->r[0] != 0 ) return QCODE_RUNNING;
    if( inst == QCJNZ && vm->r[0] == 0 ) return QCODE_RUNNING;

    if( pc < 0 ) return QCODE_RUNINVINS;
    // Listo, salto. // el PC - 1 se debe a que en el loop
    // principal se le sumará uno de nuevo
    vm->pc = pc - 1;
    return QCODE_RUNNING;


}


int qcode_vm_clx( QCodeVM* vm, qcode_extfunc f ){
    vm->r[0] = f( vm );
    return  QCODE_RUNNING;
}

/*
 * Instruccion QCRET. Fundamental!
 * Lo que hace es verificar si hay cosas en el stack o
 * no.
 * En el caso que no haya nada, simplemente termina
 * el programa por OK.
 * En el caso que haya, lo que toma del stack es,
 * en este orden, stackt, stackt_count, stackt_alloc
 * y pc.
 **/

static inline int qcode_vm_ret( QCodeVM* vm ){

    // Limpio el stackt actual
    if( vm->stackc_count >= 4 ){
        if( vm->stackt ) FREE( vm->stackt );
        vm->stackt = (long*)vm->stackc[vm->stackc_count - 1];
        vm->stackt_count  = vm->stackc[vm->stackc_count - 2];
        vm->stackt_alloc  = vm->stackc[vm->stackc_count - 3];
        vm->pc            = vm->stackc[vm->stackc_count - 4] - 1; // El menos uno se debe a que luego el programa incrementara el pc
        vm->stackc_count -= 4;
        return  QCODE_RUNNING;
    } else {
        // La liberacion de stackt se hace al final 
        // de la ejecución, así que no es necesario
        // que lo hga acá
        return  QCODE_RUNOK;
    }
        
}

/*
 * Se hace el call, guardando info en el stack acerca
 * de los datos temporales y el PC
 * */
static inline int qcode_vm_cll( QCodeVM* vm, int code ){

    //
    if( vm->stackc_count + 4 > vm->stackc_alloc ){
        vm->stackc_alloc += ALLOC_K * 4 ;
        if( vm->stackc ){
            vm->stackc = REALLOC( vm->stackc, vm->stackc_alloc * sizeof(long*) );
        } else {
            vm->stackc = ALLOC( vm->stackc_alloc * sizeof(long*) );
        }
    }

    // Guardo el pc actual más 1, que es a donde va
    // a ir a parar el programa cuando vuelva
    vm->stackc[vm->stackc_count] = vm->pc + 1;
    
    // Guardo, en ese orden, los datos de alocacion, cantidad
    // y puntero del stackt
    vm->stackc[vm->stackc_count + 1] = vm->stackt_alloc;
    vm->stackc[vm->stackc_count + 2] = vm->stackt_count;
    vm->stackc[vm->stackc_count + 3] = (long) vm->stackt;

    vm->stackc_count += 4;

    vm->stackt_alloc = 0;
    vm->stackt_count = 0;
    vm->stackt = NULL;

    // Listo, ajusto el pc y sigo adelante
    vm->pc = code - 1;
    return  QCODE_RUNNING;
}




/*
 * Esta es la parte de dump y load de la maquina.
 * Lo que se hace es bajar las cosas tal cual como estan ...
 * Teniendo en cuenta
 * a. Las instrucciones
 * b. Las etiquetas (En realidad podrian pasar solo las externas)
 * c. La data.
 * */
typedef  struct   {
    int  inst;
    int  labs;
    int  data;
    char flag;
    char bin[1];
} qcodebin;


void*     qcode_dumpbin( QCode* qcode, int* size ){
    // Calculo el tamaño de lo que necesito.
    int alloc = sizeof( qcodebin ) + qcode->inst_count * sizeof( QCodeIns ) 
                                   + qcode->lab_count  * sizeof( QCodeLab )
                                   + qcode->data_size ;
    qcodebin* bin;
    bin = ALLOC(alloc);
    bin->inst =  qcode->inst_count ;
    bin->labs =  qcode->lab_count;
    bin->data =  qcode->data_size;
    bin->flag =  qcode->flags;

    // Primero las instrucciones
    if( bin->inst )
        memcpy( bin->bin, qcode->inst_list, bin->inst * sizeof( QCodeIns ) );
    if( bin->labs )
        memcpy( bin->bin +  bin->inst * sizeof( QCodeIns ), 
                      qcode->lab_list , bin->labs * sizeof( QCodeLab ) );
    if( bin->data )
        memcpy( bin->bin +  bin->inst * sizeof( QCodeIns ) +  qcode->lab_count  * sizeof( QCodeLab ),
                      qcode->data, bin->data );

    *size = alloc;
    return (void*)bin;
}


QCode*    qcode_loadbin( void*  binv ){
    qcodebin* bin = (qcodebin*)binv;
    QCode* ret = qcode_new();

    ret->flags = bin->flag;

    // Instrucciones.
    if( bin->inst ){
        ret->inst_list  = ALLOC( bin->inst * sizeof( QCodeIns ) );
        ret->inst_count = bin->inst;
        ret->inst_alloc = bin->inst;
        memcpy( ret->inst_list, bin->bin, bin->inst * sizeof( QCodeIns ) );
    }

    if( bin->labs ){
        ret->lab_list  = ALLOC( bin->labs * sizeof( QCodeLab ) );
        ret->lab_count = bin->labs;
        ret->lab_alloc = bin->labs;
        memcpy( ret->lab_list, bin->bin +  bin->inst * sizeof( QCodeIns ), 
                          bin->labs * sizeof( QCodeLab ) );
    }

    if( bin->data ){
        ret->data = ALLOC( bin->data );
        ret->data_size  = bin->data;
        ret->data_alloc = bin->data;
        memcpy( ret->data, bin->bin +  bin->inst * sizeof( QCodeIns ) + bin->labs * sizeof( QCodeLab ),
                          bin->data );
    }

    return  ret;

}



