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


void usage(char* prg){

   puts( "Uso:" );
   printf( "  %s [-d] [-v] [filename.qasm]\n", prg );
   exit( EXIT_FAILURE );
   
}


int  main(int argc, char** argv) {

        int  flags = 0;
        int  opt = 0;
        int  ret;
        char* filename;

        while ((opt = getopt(argc, argv, "dv")) != -1){
            switch(opt){
                case 'd':
                    flags |= QASM_DEBUG;
                    break;
                case 'v':
                    flags |= QASM_VERBOSE;
                    break;
                default:
                    usage( argv[0] );
            }
        }

        if( optind == argc ) 
            ret = qasm_parse( stdin, flags );
        else{
            filename = argv[optind];
            ret = qasm_parse_filename( filename, flags );
        }

        return ret ? EXIT_SUCCESS : EXIT_FAILURE;

} 
