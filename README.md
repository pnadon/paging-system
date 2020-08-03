# paging-system
A program which simulates a paging system using pipes


Loops through each child's pipes and reads page numbers from them.
 * If the pipe is empty but not closed it is ignored.
 * If the pipe is empty and closed, the number of running processes is decremented, and the final results of that child process is printed.
 * The parentProcess then updates the paging system based on the replacement algorithm chosen via mode, and the frame within which the page was stored is returned to the child via pipes.
