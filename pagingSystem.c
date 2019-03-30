/*
pagingSystem.c
Created by Philippe Nadon
Last modified March 30, 2019

Simulates a paging system using pipes between a parent and child process.
*/

#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "pagingSystem.h"

#define MODE_ARG_INDEX 1
#define LRU_MODE 0
#define FIFO_MODE 1
#define NUM_PAGES 4
#define NUM_FILES 3
#define NUM_OTHER_ARGS 2
#define FRAMES_PRINTED_PER_LINE 14

#define TO_PARENT 0
#define TO_CHILD 1
#define WRITE_PIPE 1
#define READ_PIPE 0

#define CHILD_PROCESS 0

#define FALSE 0
#define TRUE 1

#define MAX_BUFFER_SIZE 1024

int main( int argc, char *argv[]) {
    if( argc != (NUM_OTHER_ARGS + NUM_FILES)) {
        fprintf( stderr, "Incorrect argument count\n");
        exit( EXIT_FAILURE);
    }

    int mode;
    if( strcmp( argv[ MODE_ARG_INDEX], "-l") == 0){
        mode = LRU_MODE;
    } else {
        mode = FIFO_MODE;
    }

    char *fileArgs[NUM_FILES] = {argv[2], argv[3], argv[4]};
    int pipes[NUM_FILES][2][2];

    for( int i = 0; i < NUM_FILES; i++) {
        pipe( pipes[i][TO_PARENT]);
        pipe( pipes[i][TO_CHILD]);

        switch ( fork()) {
            case -1:
                fatalsys("fork call");
            case  CHILD_PROCESS:
                childProcess(pipes[i], fileArgs[i]);
            default:
                break;
        }
    }
    parentProcess(pipes, mode, fileArgs);
    return 0;
}

/*
 * Loops through each child's pipes and reads page numbers from them.
 * If the pipe is empty but not closed it is ignored.
 * If the pipe is empty and closed, the number of running processes
 *   is decremented, and the final results of that child process is printed.
 * The parentProcess then updates the paging system based on the replacement algorithm chosen via mode,
 *   and the frame within which the page was stored is returned to the child via pipes.
 */
void parentProcess(int pipes[NUM_FILES][2][2], int mode, char *fileArgs[NUM_FILES]) {
    pageSystem systems[NUM_FILES];
    int returnPage;
    int processDone[NUM_FILES] = {FALSE, FALSE, FALSE};

    for (int i = 0; i < NUM_FILES; i++) {
        systems[i] = initSystem( fileArgs[i]);
        close(pipes[i][TO_PARENT][WRITE_PIPE]);
        close(pipes[i][TO_CHILD][READ_PIPE]);
    }
    int val;
    int runningProcesses = NUM_FILES;

    while( runningProcesses > 0) {
        for( int i = 0; i < NUM_FILES; i++) {
            int retval = read( pipes[i][TO_PARENT][READ_PIPE], &val, sizeof(val));
            switch( retval) {
                case -1:
                    if( errno == EAGAIN) {
                        break;
                    }
                    else {
                        if( processDone[i] == FALSE) {
                            printPageSystem( systems[i]);
                            processDone[i] = TRUE;
                            runningProcesses--;
                        }
                        break;
                    }
                case 0:
                    if( processDone[i] == FALSE) {
                        printPageSystem( systems[i]);
                        processDone[i] = TRUE;
                        runningProcesses--;
                    }
                    break;
                default:
                    if( mode == LRU_MODE) {
                        systems[i] = processLRU(val, systems[i]);
                    } else {
                        systems[i] = processFIFO(val, systems[i]);
                    }
                    returnPage = systems[i].returnVal;
                    write( pipes[i][TO_CHILD][WRITE_PIPE], &returnPage, sizeof(returnPage));
            }
        }
    }
    printf("  /__________________\\\n");
    printf("  | PROCESSES FINISHED\n");
    printSystemStats( countTotalReferences( systems, NUM_FILES), countTotalFaults( systems, NUM_FILES));
}

/*
 * Reads the file specified in fileArg,
 *   and pipes the page numbers found in the file to the parent via the array of pipes.
 * It then reads from the pipe the parent wrote to the frame in which the page was found in,
 *   or loaded into if it wasnt found.
 */
void childProcess(int pipes[2][2], char *fileArg ) {
    setNonBlocking(pipes[TO_PARENT]);
    close(pipes[TO_PARENT][READ_PIPE]);
    close(pipes[TO_CHILD][WRITE_PIPE]);
    int pageNum;
    int frameNumbers[MAX_BUFFER_SIZE];
    int numFrames = 0;

    FILE *refFile;
    if ((refFile = fopen(fileArg, "r")) == NULL) {
        printf( "%s", fileArg);
        fatalsys("Failed to open reference file\n");
    }

    while ( fscanf(refFile, "%d", &pageNum) != EOF) {
        write(pipes[TO_PARENT][WRITE_PIPE], &pageNum, sizeof(pageNum));
        read( pipes[TO_CHILD][READ_PIPE], &frameNumbers[ numFrames], sizeof( int));
        numFrames++;
    }

    if (fclose(refFile) == EOF) {
        fatalsys("Failed to close transaction file");
    }

    close( pipes[TO_CHILD][READ_PIPE]);
    close( pipes[TO_PARENT][WRITE_PIPE]);

    printf("\\--------------------------------/\n");
    printf("--file %s processed successfully!\n", fileArg);
    printf("--frame numbers received:\n");
    printFrameNumbers( frameNumbers, numFrames);
    printf("/--------------------------------\\\n");

    //sleep(1); // optional, to prevent print collisions
    exit( EXIT_SUCCESS );
}

/*
 * Prints the frame numbers which the child process received.
 */
void printFrameNumbers( int frameNumbers[], int numFrames) {
    for( int i = 0; i*FRAMES_PRINTED_PER_LINE < numFrames; i++) {
        printf("-- ");
        for( int j = 0; (j < FRAMES_PRINTED_PER_LINE) && (i * FRAMES_PRINTED_PER_LINE + j) < numFrames; j++) {
            printf("%d ", frameNumbers[ i * FRAMES_PRINTED_PER_LINE + j]);
        }
        printf("\n");
    }
}

/*
 * Prints the page system,
 *   along with the file it was fed from.
 */
void printPageSystem( pageSystem system) {

    printf("\n   /_________________\\\n");
    printf("  | FILE: %s\n", system.source);
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

/*
 * Counts the total number of faults in all systems.
 */
int countTotalFaults( pageSystem systems[], int lengthSystem) {
    int res = 0;
    for( int i = 0; i < lengthSystem; i++) {
        res += systems[i].faults;
    }
    return res;
}

/*
 * Counts the total number of references in all systems.
 */
int countTotalReferences( pageSystem systems[], int lengthSystem) {
    int res = 0;
    for( int i = 0; i < lengthSystem; i++) {
        res += systems[i].referenceCount;
    }
    return res;
}

/*
 * Prints the fault, reference count, and fault rate of the system.
 */
void printSystemStats( int references, int faults) {
    printf("  | REFERENCES: %d\n", references);
    printf("  | FAULTS: %d\n", faults);
    printf("  | FAULT RATE: %d\%\n", (faults * 100)/references);
    printf("  \\__________________/\n\n");
}

/*
 * Replacement algorithm for pageSystem system's pages, based on LRU.
 * Also keeps track of the number of references so far, and faults.
 * pageNum: the page requested.
 */
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

/*
 * Replacement algorithm for pageSystem system's pages, based on FIFO.
 * Also keeps track of the number of references so far, and faults.
 * pageNum: the page requested.
 */
pageSystem processFIFO(int pageNum, pageSystem system) {
    system.returnVal = findPage( system.pages, pageNum);

    if( system.returnVal == -1) {
        system.returnVal = system.oldest;
        system.pages[ system.oldest] = pageNum;
        system.oldest = (system.oldest + 1) % NUM_PAGES;
        system.faults++;
    }
    system.referenceCount++;

    return system;
}

/*
 * Updates the ages of the pages, based on which page was last accessed or replaced.
 * int newPage is the page which was just accessed / replaced.
 * pageSystem system is the system containing the paging system.
 */
pageSystem updateAges( pageSystem system, int newPage) {

    for( int i = 0; i < NUM_PAGES; i++) {
        if( i != newPage) {
            system.ages[i]++;
        }
    }
    system.ages[ newPage] = 0;

    return system;
}

/*
 * Returns the index at which the page is found.
 * Don't have to worry about duplicate pages,
 *   as only one instance of a page can be found in a system.
 */
int findPage( int pages[], int num) {
    int res = -1;

    for( int i = 0; i < NUM_PAGES; i++) {
        if( pages[i] == num) {
            res = i;
        }
    }

    return res;
}

/*
 * Returns the index of the oldest page in the pageSystem system.
 */
int getOldest( pageSystem system) {
    int oldest = 0;

    for( int i = 1; i < NUM_PAGES; i++) {
        if( system.ages[i] > system.ages[oldest]) {
            oldest = i;
        }
    }
    return oldest;
}

/*
 * Adds the O_NONBLOCK flags for the read-end of the pipe.
 */
void setNonBlocking(int pipes[]) {
    unsigned int flags;


    if ( (flags = fcntl(pipes[READ_PIPE], F_GETFL )) < 0 )
        fatalsys("fcntl get flags call");

    flags |= O_NONBLOCK;

    if ( fcntl(pipes[READ_PIPE], F_SETFL, flags ) < 0 )
        fatalsys("fcntl set flags call");
}

/*
 * Initializer for default pageSystem.
 * fileArg: the file being fed into the pageSystem,
 *   assigned to source.
 */
pageSystem initSystem(char *fileArg) {
    pageSystem system = {
            .pages = {-1, -1, -1, -1},
            .ages = {0, 0, 0, 0},
            .oldest = 0,
            .faults = 0,
            .referenceCount = 0,
            .returnVal = 0,
            .source = fileArg
    };
    return system;
}

/*
 * fatalsys call for fatal system errors
 * errmsg: the error message to print
 */
void fatalsys( char *errmsg ) {
    perror( errmsg );
    exit( EXIT_FAILURE );
}
