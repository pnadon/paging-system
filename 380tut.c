// Version 1
enum { READ = 0, WRITE = 1};
// First you must create the pipes for children and parent
int pipeToChild[3][2];
int pipeToParent[3][2];

int pager();

for( int i = 0; i < 3; i++) {
	pipe( pipeToChild[i]);
	pipe( pipeToParent[i]);

	pid = fork();
	if( pid == 0) {
		//bookkeeping
		// close unused pipends
		child(); // child function
	}	
}
pager();

// Version 2
/*
parent should close pipeToChild[ READ], pipeToParent[ WRITE]
child closes pipeToChild[ WRITE], pipeToParent[ READ]
*/

/*
call 3 child functions
*/

int pipe1[2];
int pipe2[2];

    for (int i = 0; i < 3; i++) {
        pipe(pipe1);    // Put this in an 'if' to check for error return.
        pipe(pipe2);     // Put this in an 'if' to check for error return. 
        // Here, make the read end of the pipe that the parent will use to get page numbers
        // from the current child non-blocking. 
        pid = fork();
        if (pid == -1)
            fatalsys("fork failed");
        else if (pid == 0) {
            // Do bookkeeping here to close unused pipe descriptors and to copy the ones that are to be used
            // into a more convenient data structure.
            childProcess();   // send page numbers to parent process (pager) and receive frame numbers in return
            exit( EXIT_SUCCESS ); // unless you do this at the end of childProcess()
        }
        else {
            // Do bookkeeping here to close pipe descriptors that the parent won't use and to copy the ones
            // that are to be used into a more convenient data structure (probably a 3 x 2 array).
        }
    }
    parentProcess(); // i.e., the pager
    exit( EXIT_SUCCESS ); // unless you do this at the end of parentProcess()

    // Version 3

O_NONBLOCK
retval = read( fd, ...);
switch( retval) {
	case -1:
		if( errno == EAGAIN)
			// empty pipe
		else
			fatalsys(" read failed");
		break;
	case 0;
		// pipe closed
	default:
		
}
