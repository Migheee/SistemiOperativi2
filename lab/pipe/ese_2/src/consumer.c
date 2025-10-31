#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "consumer.h"
#include "errExit.h"

#define MSG_BYTES 100

void consumer (int *pipeFD) {
    // close pipe's write-end
    if (close(pipeFD[1]) == -1)
	errExit("chiuso - consumer");

    ssize_t rB = -1;
    char buffer[MSG_BYTES + 1];
    do {
        // first we read the 'size' field of an Item
        ssize_t size;
        rB = read(pipeFD[0], &size, sizeof(ssize_t));
        if (rB == -1)
            printf("<Consumer> it looks like the pipe is broken\n");
        else if (rB == 0)
            printf("<Consumer> it looks like all pipe's write ends were closed\n");
        else if (rB < sizeof(ssize_t))
            printf("<Consumer> it looks like there is not ssize_t\n");
        else {
            // then, we read the 'value' field of an Item
            rB = read(pipeFD[0], buffer, size);
            if (rB == -1)
                printf("<Consumer> it looks like the pipe is broken\n");
            else if (rB == 0)
                printf("<Consumer> it looks like all pipe's write ends were closed\n");
            else if (rB < size)
                printf("<Consumer> it looks like there is not enough data\n");
            else {
                buffer[size] = '\0';
                printf("<Consumer> line: %s\n", buffer);
            }
        }
    } while (rB > 0);

    // close pipe's read end
    if (close(pipeFD[0]) == -1)
	errExit("chiuso - consumer");
}
