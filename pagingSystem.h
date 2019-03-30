//
// Created by phil on 3/28/19.
//

#ifndef PAGINSYSTEM_PAGINGSYSTEM_H
#define PAGINSYSTEM_PAGINGSYSTEM_H

typedef struct pageSystem {
    int pages[4];
    int ages[4];
    int oldest;
    int faults;
    int referenceCount;
    int returnVal;
    char *id;
} pageSystem;

void fatalsys( char *str );
void parentProcess(int pipes[3][2][2], int mode, char *fileArgs[3]);
void childProcess(int pipes[2][2], char *refArg );
void setFlags(int pipes[2][2]);
pageSystem initSystem(char *fileArg);
pageSystem processLRU(int val, pageSystem system);
pageSystem processFIFO(int val, pageSystem system);
int findPage( int pages[], int num);
int getOldest( pageSystem system);
pageSystem updateAges( pageSystem system, int newPage);
void printPageSystem( pageSystem system);
void printSystemStats( int references, int faults);
int countTotalReferences( pageSystem systems[], int lengthSystem);
int countTotalFaults( pageSystem systems[], int lengthSystem);

#endif //PAGINSYSTEM_PAGINGSYSTEM_H
