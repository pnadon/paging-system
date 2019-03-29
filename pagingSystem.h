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
} pageSystem;

void fatalsys( char *str );
void parentProcess(int ***pipes, int mode);
void childProcess(int pipes[2][2], char *refArg );
void initPipes( int pipes[2][2]);
pageSystem initSystem();
pageSystem processLRU(int val, pageSystem system);
pageSystem processFIFO(int val, pageSystem system);
int findPage( int pages[], int num);
int getOldest( pageSystem system);
pageSystem updateAges( pageSystem system, int newPage);

#endif //PAGINSYSTEM_PAGINGSYSTEM_H
