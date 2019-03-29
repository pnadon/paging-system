#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "pagingSystem.h"

const int LRU_MODE = 0;
const int FIFO_MODE = 1;
const int NUM_PAGES = 4;
const int NUM_FILES = 3;

const int FROM_PARENT = 0;
const int FROM_CHILD = 1;
const int TO_CHILD = 0;
const int TO_PARENT = 1;

int main( int argc, char** argv) {
    if( argc != (2 + NUM_FILES)) {
        fprintf( stderr, "Incorrect argument count\n");
        exit( EXIT_FAILURE);
    }

    int mode;
    if( strcmp( argv[1], "-l") == 0){
        mode = LRU_MODE;
    } else {
        mode = FIFO_MODE;
    }

    int pipes[3][2][2];

    for( int i = 0; i < 3; i++) {
        pipe( pipes[i][FROM_PARENT]);
        pipe( pipes[i][FROM_CHILD]);

        switch ( fork()) {
            case -1:    /* error */
                fatalsys("fork call");
            case  0:    /* childProcess */
                childProcess(pipes[i], argv[i + 2]);
            default:
                break;
        }
    }
    parentProcess(pipes, mode);
    return 0;
}

void parentProcess(int pipes[3][2][2], int mode) {
    pageSystem systems[3];
    for (int i = 0; i < 3; i++) {
        systems[i] = initSystem( mode);
    }
    int val;
    int runningProcesses = 3;
    while( runningProcesses > 0) {
        runningProcesses = 3;
        for( int i = 0; i < 3; i++) {
            int retval = read( pipes[i][FROM_CHILD][TO_PARENT], &val, sizeof(val));
            switch( retval) {
                case -1:
                    if( errno == EAGAIN) {
                        break;
                    }
                    else {
                        printf("no eagain\n");
                        fatalsys(" read failed");
                        exit( EXIT_FAILURE);
                    }
                case 0:
                    printf("case 0\n");
                    runningProcesses--;
                    break;
                default:
                    printf("value piped to parent %d\n", val);
                    if( mode == LRU_MODE) {
                        systems[i] = processLRU(val, systems[i]);
                    } else {
                        systems[i] = processFIFO(val, systems[i]);
                    }
                    write( pipes[i][FROM_PARENT][TO_CHILD], &systems[i].returnVal, sizeof(systems[i].returnVal));
            }
        }
    }
}


void childProcess(int pipes[2][2], char *refArg ) {
    setFlags(pipes);
    int pageNum;
    int returnPage;

    FILE *refFile;
    if ((refFile = fopen(refArg, "r")) == NULL) {
        printf( "%s", refArg);
        fatalsys("Failed to open reference file\n");
    }

    while ( fscanf(refFile, "%d", &pageNum) != EOF) {
        printf("read from file:%d:\n", pageNum);
        write(pipes[FROM_CHILD][TO_PARENT], &pageNum, sizeof(pageNum));
        read( pipes[FROM_PARENT][TO_CHILD], &returnPage, sizeof( returnPage));
        printf("received %d from parent\n", returnPage);
    }

    if (fclose(refFile) == EOF) {
        fatalsys("Failed to close transaction file");
    }
    printf("====done!====\n");
    close( pipes[FROM_CHILD][TO_PARENT]);
    close( pipes[FROM_PARENT][TO_CHILD]);
    exit( EXIT_SUCCESS );
}

pageSystem processLRU(int val, pageSystem system) {
    system.returnVal = findPage( system.pages, val);

    if( system.returnVal == -1) {
        int oldest = getOldest( system);
        system.returnVal = oldest;
        system.pages[ oldest] = val;
        system = updateAges( system, oldest);
        system.faults++;
    } else {
        system = updateAges( system, system.returnVal);
    }
    system.referenceCount++;

    return system;
}

pageSystem processFIFO(int val, pageSystem system) {
    system.returnVal = findPage( system.pages, val);

    if( system.returnVal == -1) {
        system.returnVal = system.oldest;
        system.pages[ system.oldest] = val;
        system.oldest = (system.oldest + 1) % NUM_PAGES;
        system.faults++;
    }
    system.referenceCount++;

    return system;
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

void setFlags(int pipes[2][2]) {
    unsigned int flags;

    if ( (flags = fcntl(pipes[FROM_CHILD][TO_PARENT], F_GETFL )) < 0 )
        fatalsys("fcntl get flags call");
    /* add the nonblocking flag (using bitwise OR) */
    flags |= O_NONBLOCK;
    /* set the flags */
    if ( fcntl(pipes[FROM_CHILD][TO_PARENT], F_SETFL, flags ) < 0 )
        fatalsys("fcntl set flags call");
}

pageSystem initSystem() {
    pageSystem system = {
            .pages = {-1, -1, -1, -1},
            .ages = {0, 0, 0, 0},
            .oldest = 0,
            .faults = 0,
            .referenceCount = 0,
            .returnVal = 0
    };
    return system;
}


void fatalsys( char *errmsg ) {
    perror( errmsg );
    exit( EXIT_FAILURE );
}