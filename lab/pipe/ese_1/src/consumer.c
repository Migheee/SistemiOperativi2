#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "consumer.h"
#include "errExit.h"

#define MSG_BYTES 100

void consumer (int *pipeFD) {
    // close pipe's write end
    if ((close(pipeFD[1])) == -1)
	errExit("chiuso consumatore  - scrittura");

    ssize_t rB = -1;
    char buffer[MSG_BYTES + 1];
    do {
        // read max MSG_BYTES chars from the pipe
        rB = read(pipeFD[0], buffer, MSG_BYTES);
        if (rB == -1)
            printf("<Consumer> it looks like the pipe is broken\n");
        else if (!rB)
            printf("<Consumer> it looks like all pipe's write ends were closed\n");
        else {
            buffer[rB] = '\0';
            printf("<Consumer> line: %s\n", buffer);
        }
    } while (rB > 0);

    // close pipe's read end
    if ((close(pipeFD[0])) == -1)
	errExit("chiuso consumatore - lettura");
}
