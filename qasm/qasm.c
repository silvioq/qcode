/*
 *
 * Pseudo assembler para maquina virtual QCode
 * Haga lo que quiera con ella, pero no hay garantias
 * Silvio Quadri (c) 2009
 *
 * */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <qcode.h>
#include "qasm-parser.h"
#include "qasm.h"

QCode*  qasm = NULL;

void usage(char* prg){

   puts( "Uso:" );
   printf( "  %s [-d] [-v] [-r] [filename.qasm]\n", prg );
   exit( EXIT_FAILURE );
   
}


int  main(int argc, char** argv) {

        int  flags = 0;
        int  opt = 0;
        int  run_program  = 0;
        int  ret;
        char* filename;

        while ((opt = getopt(argc, argv, "rdv")) != -1){
            switch(opt){
                case 'd':
                    flags |= QASM_DEBUG;
                    break;
                case 'v':
                    flags |= QASM_VERBOSE;
                    break;
                case 'r':
                    run_program = 1;
                    break;
                default:
                    usage( argv[0] );
            }
        }

        if( optind == argc ) 
            ret = qasm_parse( stdin, flags );
        else{
            filename = argv[optind];
            if( flags & QASM_VERBOSE ) printf( "Abriendo %s\n", filename );
            ret = qasm_parse_filename( filename, flags );
        }

        if( run_program && ret && qasm ){
           qcode_run( qasm, &ret );
           if( flags & QASM_VERBOSE ) printf( "Retorno de ejecucion: %d\n", ret );
        }

        if( qasm ) qcode_free( qasm );

        return ret;


} 
