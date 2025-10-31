#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "errExit.h"

int main (int argc, char *argv[]) {
    // Check command line input arguments
    // The program only wants a FIFO pathname
    if (argc != 2) {
        printf("Usage: %s fifo_pathname\n", argv[0]);
        return 0;
    }

    // read the FIFO's pathname
    char *path2ServerFIFO = argv[1];

    printf("<Client> opening FIFO %s...\n", path2ServerFIFO);
    // Open the FIFO in write-only mode
    int fifoDes = open(path2ServerFIFO, O_WRONLY);
    if(fifoDes == -1)
        errExit("<Client> Open FIFO Failed");

    int v [] = {0, 0};
    printf("<Client> Give me two numbers: ");
    scanf("%d %d", &v[0], &v[1]);

    printf("<Client> sending %d %d\n", v[0], v[1]);
    // Write two integers to the opened FIFO
    if(write(fifoDes, v, sizeof(v)) == -1) errExit("<Client> Write FIFO Failed");

    // Close the FIFO
    if(close(fifoDes) == -1) errExit("<Client> Close FIFO Failed");
}
