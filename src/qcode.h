/*
 *
 * Libreria máquina virtual de proposito general
 * Haga lo que quiera con ella, pero no hay garantias
 * Silvio Quadri (c) 2009
 *
 * */


#ifndef  QCODE_INCLUDED
#define  QCODE_INCLUDED

/* 
 * TODO: Describir claramente el funcionamiento de cada instrucción
 * TODO: Hacer un dump legible
 */

/*
 * NOP: No operacion
 * */
#define  QCNOP     0


/* 
 * STO: Mueve el valor del origen al destino
 * Ejemplo:
 *    STO  1, 3   (pone el valor del registro 3 en el registro 1)
 *    STO  1, 16  (pone el valor del temporal 16 en el registro 1)
 * */
#define  QCSTO     1


/*
 * STI: Guarda un numero entero dentro del detino
 * Ejemplo:
 *    STI  1, 3   (pone el valor 3 al registro 1)
 * */
#define  QCSTI     2

/*
 * STP: Pone el valor que esta en el registro o
 *      temporal que es el origen en el puntero destino
 * Ejemplo:
 *    STP  1, pointer  (pone el valor del registro 1 en
 *                      el puntero pointer)
 * */
#define  QCSTP     3


/*
 * STM: Pone el valor del puntero de memoria en el destino
 * Ejemplo:
 *    STM  16, pointer  (pone el valor entero que hay en el puntero
 *                       pointer en el temporal 16 )
 * */
#define  QCSTM     4

/* 
 * STD: Pone el valor del dato (puntero estatico) en el registro
 * */

#define  QCSTD     5


#define  QCADD     6
#define  QCSUB     7
#define  QCINC     8
#define  QCDEC     9
#define  QCMUL     10
#define  QCDIV     11
#define  QCEQU     12
#define  QCNEQ     13
#define  QCLTH     14
#define  QCLEQ     15
#define  QCGTH     16
#define  QCGEQ     17
#define  QCPSH     18
#define  QCPOP     19
#define  QCJMP     20
#define  QCJPZ     21
#define  QCJNZ     22
#define  QCCLL     23
#define  QCALT     24
#define  QCCLX     25
#define  QCRET     26

typedef  long(*qcode_extfunc)(void*);

// Instrucciones
typedef  struct StrQCodeIns {

    // Codigo de operacion
    unsigned  char   inst;

    // Operandos 1 y 2
    unsigned  char   op1;
    unsigned  char   op2;

    union {
        // Puntero a la posicion de codigo
        // para calls y jumps
        int    code;

        // Cosas exteriores ...
        long   extval;

        // Puntero a la data
        int    data;
    };

}  QCodeIns;


// Labels
// QCODE_LABFLAG_INS => Es una etiqueta de instruccion
#define  QCODE_LABFLAG_INS  1

// QCODE_LABFLAG_EXT => Es una etiqueta de funcion externa
#define  QCODE_LABFLAG_EXT  2

// QCODE_LABFLAG_DAT => Es un puntero a datos (entero en el array data)
#define  QCODE_LABFLAG_DAT  3

// QCODE_LABFLAG_REF => Hay referencias que apuntan a esa etiqueta
#define  QCODE_LABFLAG_REF  4

// QCODE_LABFLAG_SET => La etiqueta fue seteada. Esto es util para las 
//                      etiquetas de instruccion, las cuales pueden ser
//                      definidas antes de establecer a que instruccion
//                      apuntan
#define  QCODE_LABFLAG_SET  8

// Este define es util para determinar el tipo de la etiqueta,
// el cual se establece en los dos bits menos significativos del flag
#define  QCODE_LABFLAG_CHECK(f) (f & 3)
typedef  struct StrQCodeLab  {
    int    lab;
    char   flag;
    union {
        int            inst;
        int            data;
        qcode_extfunc  ext;
    };
}  QCodeLab;


// Las flags del qcode
// El debug no tiene ninguna explicación
#define  QCODE_FLAG_DEBUG    1

// LABRESOLVED es "readonly". No puede ser tocada
// externamente y sirve para determinar si el qcode ha
// resuelto las etiquetas
#define  QCODE_LABRESOLVED   2

// EXTRESOLVED es "readonly". No puede ser tocada
// externamente y sirve para determinar si el qcode ha
// resuelto las etiquetas externas
#define  QCODE_EXTRESOLVED   4

// MEMVOLATILE le indica al qcode si la "data" definida
// debe ser volatil o no. En el caso que sea volatil
// cada ejecucion de codigo tendra su propia data,
// que es igual a lo que esta definido. 
#define  QCODE_MEMVOLATILE   8


#define  QCODE_ISDEBUG(q)    ( q->flags & QCODE_FLAG_DEBUG )
#define  QCODE_ISVOLATILE(q) ( q->flags & QCODE_MEMVOLATILE )

// Codigo
typedef  struct StrQCode {

    // codigo
    QCodeIns*    inst_list;
    int          inst_count;
    int          inst_alloc;

    // Etiquetas
    QCodeLab*    lab_list;
    int          lab_count;
    int          lab_alloc;

    // Area de datos
    void*     data;
    int       data_size;
    int       data_alloc;

    // Flags
    char         flags;

}  QCode;




/*
 * Vamos a definir las cosas de la maquina virtual ...
 *
 **/
typedef   struct   {
    // Program counter!
    int       pc;
    
    // Stack comun
    long*     stackp;
    int       stackp_count;
    int       stackp_alloc;

    // Stack especial
    long*     stackt;
    int       stackt_count;
    int       stackt_alloc;

    // Stack de llamada
    long*     stackc;
    int       stackc_count;
    int       stackc_alloc;

    // Area de datos
    void*     data;
    int       data_size;
    int       data_alloc;

    // Los registros
    long      r[16];

}  QCodeVM;



#ifndef  NULL  
#define  NULL  (void*)0
#endif

#define   unnamed_label  (char*)-1

QCode*    qcode_new();
void      qcode_free  ( QCode* qcode );

void      qcode_op    ( QCode* qcode, unsigned char inst, unsigned char op1, long op2 );

// Todas estas funciones son para trabajar con etiquetas
//
// Crea una etiqueta nombrada que apunta a un cacho de codigo
int       qcode_crlab ( QCode* qcode, char* label_name );

// Crea una etiqueta nombrada que apunta a una funcion externa
int       qcode_xcrlab( QCode* qcode, char* label_name, qcode_extfunc f );

// Crea una etiqueta nombrada que apunta a un puntero de datos
// Se pasan los datos también
int       qcode_dcrlab( QCode* qcode, char* label_name, void*  data, int size );
int       qcode_dcrlab_long( QCode* qcode, char* label_name, long data );
int       qcode_dcrlab_str( QCode* qcode, char* label_name, char* data );

// Establece la etiqueta para el cacho de codigo en la posicion actual
// del programa
void      qcode_label ( QCode* qcode, int   label );

// Operacion con etiqueta (JMP, CLL, CLX, etc)
void      qcode_oplab ( QCode* qcode, char inst, int label );

// Operacion con etiqueta nombrada 
void      qcode_opnlab( QCode* qcode, char inst, char* label_name );

// Esta funcion devuelve el PC de la etiqueta.
int       qcode_label_getpc( QCode* qcode, int label );




// Estas son las funciones para interactuar con el programa
// ya creado
#define   QCODE_RUNNING  -1
#define   QCODE_RUNOK     0
#define   QCODE_RUNOUTOFP 1
#define   QCODE_RUNOUTOFS 2
#define   QCODE_RUNOUTOFT 3
#define   QCODE_RUNOUTOFM 4
#define   QCODE_RUNNOTIMP 5
#define   QCODE_RUNINVINS 6

int       qcode_run   ( QCode* qcode, long*  ret );
int       qcode_runargs( QCode* qcode, long*  ret, int pc, int argc, long* argv );
void      qcode_push  ( QCodeVM* vm, long l );
long      qcode_pop   ( QCodeVM* vm );
void*     qcode_pop_ptr( QCodeVM* vm );


// Dump de la maquina.
void*     qcode_dumpbin( QCode* qcode, int* size );
QCode*    qcode_loadbin( void*  bin );

#endif
