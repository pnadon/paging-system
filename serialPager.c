/* O_NONBLOCK example */
/* Based on Haviland & Salama, p. 151 */

#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "serialPager.h"

const size_t MSGSIZE = sizeof(int);
const int END_OF_TRANSMISSION = -1;
const int LRU_MODE = 0;
const int FIFO_MODE = 1;
const int NUM_PAGES = 4;

int main( int argc, char *argv[] ) {
    if( argc != 5) {
        fprintf( stderr, "Incorrect argument count\n");
    }

    pageSystem system1 = {
            .pages = {-1, -1, -1, -1},
            .ages = {0, 0, 0, 0},
            .oldest = 0,
            .faults = 0,
            .referenceCount = 0,
            .callRes = 0
    };

    pageSystem system2 = {
            .pages = {-1, -1, -1, -1},
            .ages = {0, 0, 0, 0},
            .oldest = 0,
            .faults = 0,
            .referenceCount = 0,
            .callRes = 0
    };

    pageSystem system3 = {
            .pages = {-1, -1, -1, -1},
            .ages = {0, 0, 0, 0},
            .oldest = 0,
            .faults = 0,
            .referenceCount = 0,
            .callRes = 0
    };
    int pfds[3][2];
    pageSystem systems[3] = {system1, system2, system3};
    int mode;

    if( strcmp( argv[1], "-l") == 0){
        mode = LRU_MODE;
    } else {
        mode = FIFO_MODE;
    }

    initPipe( pfds[0]);
    initPipe( pfds[1]);
    initPipe( pfds[2]);

    switch ( fork()) {
        case -1:	/* error */
            fatalsys("fork call");
        case  0:	/* child */
            child( pfds[0], argv[2]);
        default:
            break;
    }
    switch ( fork()) {
        case -1:	/* error */
            fatalsys("fork call");
        case  0:	/* child */
            child( pfds[1], argv[3]);
        default:
            break;
    }
    switch ( fork()) {
        case -1:	/* error */
            fatalsys("fork call");
        case  0:	/* child */
            child( pfds[2], argv[4]);
        default:
            break;
    }

    for( int i = 0; i < 3; i++) {
        parent(pfds[i], mode, systems[i]);
    }
}

void initPipe( int pfd[]) {
    int flags;

    /* open pipe */
    if ( pipe(pfd) < 0 )
        fatalsys("pipe call");

    /* set O_NONBLOCK flag for p[0] */
    /* first get current flags */
    if ( (flags = fcntl(pfd[0], F_GETFL )) < 0 )
        fatalsys("fcntl get flags call");
    /* add the nonblocking flag (using bitwise OR) */
    flags |= O_NONBLOCK;
    /* set the flags */
    if ( fcntl(pfd[0], F_SETFL, flags ) < 0 )
        fatalsys("fcntl set flags call");
}


void parent(int p[2], int mode, pageSystem system) {
    ssize_t nread;
    int val;

    close( p[1] );

    nread = read( p[0], &val, sizeof(val));
    while ( val != END_OF_TRANSMISSION) {
        switch ( nread ) {
            case -1:	/* pipe empty */
                //printf("< pipe empty >\n");
                sleep(0);
                break;
            case  0:	/* end of file */
                //printf("< end of file >\n");
                exit( EXIT_FAILURE );
            default:	/* characters read */
                if( mode == 0) {
                    printf("%d\n", val);
                    system = processLRU(val, system);
                } else {
                    system = processFIFO(val, system);
                }
                //printPageSystem( system);
                break;
        }
        nread = read( p[0], &val, sizeof(val));
    };

    printPageSystem( system);
}

pageSystem processLRU(int val, pageSystem system) {
    system.callRes = findPage( system.pages, val);

    if( system.callRes == -1) {
        int oldest = getOldest( system);
        system.pages[ oldest] = val;
        system = updateAges( system, oldest);
    } else {
        system = updateAges( system, system.callRes);
    }

    return system;
}

pageSystem processFIFO(int val, pageSystem system) {
    system.callRes = findPage( system.pages, val);

    if( system.callRes == -1) {
        system.pages[ system.oldest] = val;
        system.oldest = (system.oldest + 1) % NUM_PAGES;
        system.faults++;
    }
    system.referenceCount++;

    return system;
}

void child( int p[2], char *refArg ) {
    int pageNum;

    FILE *refFile;
    if ((refFile = fopen(refArg, "r")) == NULL) {
        fatalsys("Failed to open reference file\n");
    }

    close( p[0] );

    rewind(refFile);
    while ( fscanf(refFile, "%d", &pageNum) != EOF) {
        //printf("read from file:%d\n", pageNum);
        write( p[1], &pageNum, sizeof(pageNum) );
        sleep( 0);
    }

    /* send final message */
    /* If you comment out the next line, the parent will
       report end-of-file ('read' will return 0 bytes). */
    if (fclose(refFile) == EOF) {
        fatalsys("Failed to close transaction file");
    }
    write( p[1], &END_OF_TRANSMISSION, MSGSIZE );
    exit( EXIT_SUCCESS );
}


void fatalsys( char *errmsg ) {
    perror( errmsg );
    exit( EXIT_FAILURE );
}

void printPageSystem( pageSystem system) {

    for( int i = 0; i < NUM_PAGES; i++) {
        if( system.pages[i] < 0 ){
            printf( "|   ");
        } else {
            printf( "| %d ", system.pages[i]);
        }
    }
    printf("|\n_________________\n");
}

pageSystem updateAges( pageSystem system, int newPage) {

    for( int i = 0; i < NUM_PAGES; i++) {
        if( i != newPage) {
            system.ages[i]++;
        }
    }
    system.ages[ newPage] = 0;

    return system;
}

int findPage( int pages[], int num) {
    int res = -1;

    for( int i = 0; i < NUM_PAGES; i++) {
        if( pages[i] == num) {
            res = i;
        }
    }

    return res;
}

int getOldest( pageSystem system) {
    int oldest = 0;

    for( int i = 1; i < NUM_PAGES; i++) {
        if( system.ages[i] > system.ages[oldest]) {
            oldest = i;
        }
    }
    return oldest;
}
