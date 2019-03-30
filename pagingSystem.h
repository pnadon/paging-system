//
// Created by phil on 3/28/19.
//

#ifndef PAGINSYSTEM_PAGINGSYSTEM_H
#define PAGINSYSTEM_PAGINGSYSTEM_H

/*
 * a structure which maintains all of the "bookeeping"
 *   associated with the paging system.
 */
typedef struct pageSystem {
    int pages[4]; // the actual pages stored in the system
    int ages[4]; // the age of the pages
    int oldest; // the oldest page
    int faults; // the number of faults
    int referenceCount; // the number of references
    int returnVal; // the returned frame number, or otherwise
    char *source; // the file or other source of page numbers
} pageSystem;

void fatalsys( char *str );
void parentProcess(int pipes[3][2][2], int mode, char *fileArgs[3]);
void childProcess(int pipes[2][2], char *refArg );
void setNonBlocking(int pipes[2]);
pageSystem initSystem(char *fileArg);
pageSystem processLRU(int val, pageSystem system);
pageSystem processFIFO(int pageNum, pageSystem system);
int findPage( int pages[], int num);
int getOldest( pageSystem system);
pageSystem updateAges( pageSystem system, int newPage);
void printPageSystem( pageSystem system);
void printSystemStats( int references, int faults);
int countTotalReferences( pageSystem systems[], int lengthSystem);
int countTotalFaults( pageSystem systems[], int lengthSystem);
void printFrameNumbers( int frameNumbers[], int numFrames);

#endif //PAGINSYSTEM_PAGINGSYSTEM_H
