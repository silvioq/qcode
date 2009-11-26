#include   <stdio.h>
#include   <string.h>
#include  <stdlib.h>
#include   <assert.h>
#include   "qcode.h"



long  printf_de_i( QCodeVM* vm ){
    printf( "%d ", (int) qcode_pop( vm ) );
    return 0;
}



void  mi_primer_programa( int recoff ){
    QCode* code;
    int  label1, label2;
    long  ret;

    // Mi primer programa va a ser algo asi
    // int i = 0; while( i < 100 ){ printf( "%d\n", i ); i ++ }; 
    assert( code = qcode_new() );
    label1 = qcode_crlab( code, unnamed_label );
    label2 = qcode_crlab( code, unnamed_label );
    qcode_xcrlab( code, "printf_de_i", (qcode_extfunc) printf_de_i );
    printf( "." ); 

    qcode_op( code, QCSTI, 1 + recoff , 0 );
    qcode_op( code, QCSTI, 2 + recoff, 100 );
    qcode_label( code, label1 );
    qcode_op( code, QCLTH, 1 + recoff, 2 + recoff );
    qcode_oplab( code, QCJPZ, label2 );
    qcode_op( code, QCPSH, 1 + recoff, 0 );
    qcode_opnlab( code, QCCLX, "printf_de_i" );
    qcode_op( code, QCINC, 1 + recoff, 0 );
    qcode_oplab( code, QCJMP, label1 );
    qcode_label( code, label2 );
    qcode_op( code, QCRET, 0, 0 );

    assert( qcode_run( code, &ret ) == QCODE_RUNOK );
    qcode_free( code );
    printf( "." ); 
    

}

static  long fact;
long   setfactorial( QCodeVM* vm ){
    qcode_push( vm, fact );
    return fact;
}



QCode*  factorial( int con_push ){
    // factorial(n){ if( n == 1 ) return 1; else return n * factorial( n - 1 ) };
    //
    //
    int  label1, label2;
    QCode* code;

    assert( code = qcode_new() );
    label1 = qcode_crlab( code, unnamed_label );
    label2 = qcode_crlab( code, unnamed_label );
    qcode_xcrlab( code, "setfactorial", (qcode_extfunc) setfactorial );

    if( ! con_push ){
        qcode_opnlab( code, QCCLX, "setfactorial" ); 
                                            // CLX   setfactorial
    }
    qcode_label( code, label1 );            // label1:
    qcode_op( code, QCPOP, 16, 0 );         // POP  16
    qcode_op( code, QCSTI, 3, 1 ) ;         // STI   3,  1
    qcode_op( code, QCEQU, 16, 3 );         // EQU  16,  3
    qcode_oplab( code, QCJPZ, label2 );     // JPZ   label2
    qcode_op( code, QCSTI, 0, 1 );          // STI   0,  1
    qcode_op( code, QCRET, 0, 0 );          // RET
    qcode_label( code, label2 );            // label2:
    qcode_op( code, QCSTO, 1 , 16 );        // STO   1 , 16
    qcode_op( code, QCDEC, 1 , 0 );         // DEC   1 
    qcode_op( code, QCPSH, 1 , 0 );         // PSH   1 
    qcode_oplab( code, QCCLL, label1 );     // CLL   label1
    qcode_op( code, QCMUL, 0, 16 );         // MUL   0,  16
    qcode_op( code, QCRET, 0, 0 );          // RET

    return  code;

}

void   test_factorial( ){
    QCode* code = factorial( 0 );
    long ret;
    
    printf( "." );
    fact = 1;
    assert( qcode_run( code, &ret ) == QCODE_RUNOK );
    assert( ret == 1 ); 


    printf( "." );
    fact = 2;
    assert( qcode_run( code, &ret ) == QCODE_RUNOK );
    assert( ret == 2 );

    printf( "." );
    fact = 6;
    assert( qcode_run( code, &ret ) == QCODE_RUNOK );
    // printf( "El resultado es  %d\n", (int)ret );
    assert( ret == 2 * 3 * 4 * 5 * 6  ); 

    qcode_free( code );

}

void   test_factorial2( ){
    QCode* code = factorial( 1 );
    long ret;
    long argv[1];
    
    printf( "." );
    argv[0] = 1;
    assert( qcode_runargs( code, &ret, 0, 1, argv ) == QCODE_RUNOK );
    assert( ret == 1 ); 


    printf( "." );
    argv[0] = 2;
    assert( qcode_runargs( code, &ret, 0, 1, argv ) == QCODE_RUNOK );
    assert( ret == 2 );

    printf( "." );
    argv[0] = 10;
    assert( qcode_runargs( code, &ret, 0, 1, argv ) == QCODE_RUNOK );
    // printf( "El resultado es  %d\n", (int)ret );
    assert( ret == 2 * 3 * 4 * 5 * 6 * 7 * 8 * 9 * 10 ); 

    qcode_free( code );

}


long  strcmp_test( QCodeVM* vm ){
    char* data = "TEST_MEMORIA";
    char* cmp  ;
    printf( "." );
    assert( cmp = (char*)qcode_pop_ptr( vm ) );
    assert( strcmp( data, cmp ) == 0 );
    return 0;
}

void  test_memoria(){
    char* data = "TEST_MEMORIA";
    char* xxx;
    int  lab, cmp, lal;
    long  ret;
    QCode*  code;
    assert( code = qcode_new() );
    printf( "." );
    lab = qcode_dcrlab_str( code, unnamed_label, data );
    lal = qcode_dcrlab_long( code, unnamed_label, 134 );
    cmp = qcode_xcrlab( code, unnamed_label, (qcode_extfunc) strcmp_test );
    
    qcode_op( code, QCSTP, 0, lab );
    qcode_op( code, QCPSH, 0, 0   );
    qcode_oplab( code, QCCLX, cmp );
    qcode_op( code, QCSTM, 1, lal );
    qcode_op( code, QCINC, 1 , 0  );
    qcode_op( code, QCSTD, 1, lal );
    qcode_op( code, QCSTM, 0, lal );

    qcode_op( code, QCRET, 0, 0 ) ;
    printf( "." );
    assert( qcode_run( code, &ret ) == QCODE_RUNOK );
    assert( ret == 135 );
    xxx = (char*)code->data + strlen( data ) + 1;
    printf( "." );
    assert( ((long*)xxx)[0] == 135 );
    printf( "." );

    qcode_free( code );

    
}


void  test_dump(){
    
    QCode* code = factorial( 1 );
    int  tamanio;
    printf( "." );
    void* bin = qcode_dumpbin( code, &tamanio );
    long ret;
    long argv[1];

    QCode* code2;
    assert( code2 = qcode_loadbin( bin ) );
    printf( "." );

    argv[0] = 10;
    assert( qcode_runargs( code2, &ret, 0, 1, argv ) == QCODE_RUNOK );
    // printf( "El resultado es  %d\n", (int)ret );
    assert( ret == 2 * 3 * 4 * 5 * 6 * 7 * 8 * 9 * 10 ); 
    printf( "." );

    argv[0] = 11;
    assert( qcode_runargs( code2, &ret, 0, 1, argv ) == QCODE_RUNOK );
    // printf( "El resultado es  %d\n", (int)ret );
    assert( ret == 2 * 3 * 4 * 5 * 6 * 7 * 8 * 9 * 10 * 11 ); 
    printf( "." );
}



int  main(int argc, char** argv){
    QCode* code;
  
    printf( "." ); 
    assert( code = qcode_new() );
    qcode_free( code );

    mi_primer_programa( 0 );  // Hago el primer programa con registros
    mi_primer_programa( 20 ); // Hago el primer programa con temporales

    test_factorial( );
    test_factorial2( );

    test_memoria( );

    test_dump( );

    printf( "\n" );
    return 0;

}
