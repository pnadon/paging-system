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

const int TO_PARENT = 0;
const int TO_CHILD = 1;
const int WRITE_PIPE = 1;
const int READ_PIPE = 0;

const int FALSE = 0;
const int TRUE = 1;

int main( int argc, char *argv[]) {
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

    char *fileArgs[3] = {argv[2], argv[3], argv[4]};
    int pipes[3][2][2];

    for( int i = 0; i < 3; i++) {
        pipe( pipes[i][TO_PARENT]);
        pipe( pipes[i][TO_CHILD]);

        switch ( fork()) {
            case -1:    /* error */
                fatalsys("fork call");
            case  0:    /* childProcess */
                childProcess(pipes[i], fileArgs[i]);
            default:
                break;
        }
    }
    parentProcess(pipes, mode, fileArgs);
    return 0;
}

void parentProcess(int pipes[3][2][2], int mode, char *fileArgs[3]) {
    pageSystem systems[3];
    int returnPage;
    int processDone[3] = {FALSE, FALSE, FALSE};
    for (int i = 0; i < 3; i++) {
        systems[i] = initSystem( fileArgs[i]);
        close(pipes[i][TO_PARENT][WRITE_PIPE]);
        close(pipes[i][TO_CHILD][READ_PIPE]);
    }
    int val;
    int runningProcesses = 3;
    while( runningProcesses > 0) {
        //printf("|| start of while || \n");
        for( int i = 0; i < 3; i++) {
            //printf("|| start of for loop %d ||\n", i);
            int retval = read( pipes[i][TO_PARENT][READ_PIPE], &val, sizeof(val));
            //printf("|| retVal of read: %d\n", retval);
            switch( retval) {
                case -1:
                    if( errno == EAGAIN) {
                        //printf("\n||%d|| case -1 EAGAIN\n\n", i);
                        break;
                    }
                    else {
                        //printf("\n||%d|| case -1 no eagain\n\n", i);
                        if( processDone[i] == 0) {
                            printPageSystem( systems[i]);
                            processDone[i] = 1;
                            runningProcesses--;
                        }
                        break;
                    }
                case 0:
                    //printf("\n||%d|| case 0\n\n", i);
                    if( processDone[i] == 0) {
                        printPageSystem( systems[i]);
                        processDone[i] = 1;
                        runningProcesses--;
                    }
                    break;
                    //fatalsys(" read failed");
                default:
                    //printf("||%d|| value piped to parent %d\n", i, val);
                    if( mode == LRU_MODE) {
                        systems[i] = processLRU(val, systems[i]);
                    } else {
                        systems[i] = processFIFO(val, systems[i]);
                    }
                    returnPage = systems[i].returnVal;
                    //printf("||%d|| sending to child: %d\n", i, returnPage);
                    write( pipes[i][TO_CHILD][WRITE_PIPE], &returnPage, sizeof(returnPage));
            }
        }
    }
    printf("  /__________________\\\n");
    printf("  | PROCESSES FINISHED\n");
    printSystemStats( countTotalReferences( systems, NUM_FILES), countTotalFaults( systems, NUM_FILES));
}


void childProcess(int pipes[2][2], char *refArg ) {
    setFlags(pipes);
    close(pipes[TO_PARENT][READ_PIPE]);
    close(pipes[TO_CHILD][WRITE_PIPE]);
    int pageNum;
    int returnPages[1024];
    int numReturnPages = 0;

    FILE *refFile;
    if ((refFile = fopen(refArg, "r")) == NULL) {
        printf( "%s", refArg);
        fatalsys("Failed to open reference file\n");
    }

    while ( fscanf(refFile, "%d", &pageNum) != EOF) {
        printf("--read from file:%d:\n", pageNum);
        write(pipes[TO_PARENT][WRITE_PIPE], &pageNum, sizeof(pageNum));
        read( pipes[TO_CHILD][READ_PIPE], &returnPages[ numReturnPages], sizeof( int));
        printf("--received %d from parent\n", returnPages[ numReturnPages]);
    }

    if (fclose(refFile) == EOF) {
        fatalsys("Failed to close transaction file");
    }
    //printf("====done!====\n");
    close( pipes[TO_CHILD][READ_PIPE]);
    close( pipes[TO_PARENT][WRITE_PIPE]);
    printf("--child terminated successfully!\n");
    exit( EXIT_SUCCESS );
}

void printPageSystem( pageSystem system) {

    printf("\n   /_________________\\\n");
    printf("  | FILE: %s\n", system.id);
    printf("  |-------------------\n  ");
    for( int i = 0; i < NUM_PAGES; i++) {
        if( system.pages[i] < 0 ){
            printf( "| ");
        } else {
            printf( "| %d ", system.pages[i]);
        }
    }
    printf("|\n  |-------------------\n");
    printSystemStats( system.referenceCount, system.faults);
}

int countTotalFaults( pageSystem systems[], int lengthSystem) {
    int res = 0;
    for( int i = 0; i < lengthSystem; i++) {
        res += systems[i].faults;
    }
    return res;
}

int countTotalReferences( pageSystem systems[], int lengthSystem) {
    int res = 0;
    for( int i = 0; i < lengthSystem; i++) {
        res += systems[i].referenceCount;
    }
    return res;
}

void printSystemStats( int references, int faults) {
    printf("  | REFERENCES: %d\n", references);
    printf("  | FAULTS: %d\n", faults);
    printf("  | FAULT RATE: %d\%\n", (faults * 100)/references);
    printf("  \\__________________/\n\n");
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


    if ( (flags = fcntl(pipes[TO_PARENT][READ_PIPE], F_GETFL )) < 0 )
        fatalsys("fcntl get flags call");

    flags |= O_NONBLOCK;

    if ( fcntl(pipes[TO_PARENT][READ_PIPE], F_SETFL, O_NONBLOCK ) < 0 )
        fatalsys("fcntl set flags call");
}

pageSystem initSystem(char *fileArg) {
    pageSystem system = {
            .pages = {-1, -1, -1, -1},
            .ages = {0, 0, 0, 0},
            .oldest = 0,
            .faults = 0,
            .referenceCount = 0,
            .returnVal = 0,
            .id = fileArg
    };
    return system;
}


void fatalsys( char *errmsg ) {
    perror( errmsg );
    exit( EXIT_FAILURE );
}