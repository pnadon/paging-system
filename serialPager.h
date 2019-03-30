//
// Created by phil on 3/18/19.
//

#ifndef PIPEDPAGES_PARENTPAGESYSTEM_H
#define PIPEDPAGES_PARENTPAGESYSTEM_H

typedef struct pageSystem {
    int pages[4];
    int ages[4];
    int oldest;
    int faults;
    int referenceCount;
    int callRes;
} pageSystem;

void parent(int p[2], int mode, pageSystem system);
void child( int p[2], char *fileArg);
pageSystem processLRU(int val, pageSystem system);
pageSystem processFIFO(int val, pageSystem system);
void initPipe( int pfd[]);
void printPageSystem( pageSystem system);
int findPage( int pages[], int num);
int getOldest( pageSystem system);
pageSystem updateAges( pageSystem system, int newPage);
void fatalsys( char *str );

#endif //PIPEDPAGES_PARENTPAGESYSTEM_H