#include <asm-generic/errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#include <sys/msg.h>
#include <unistd.h>
#include <errno.h>

#include "order.h"
#include "errExit.h"

// the message queue identifier
int msqid = -1;

void signTermHandler(int sig) {
    // do we have a valid message queue identifier?
    if (msqid > 0) {
        if(msgctl(msqid, IPC_RMID, 0) == -1) errExit("MSGCTL Failed");
    }
    printf("Queue Removed\n");
    // terminate the server process
    exit(0);
}

int main (int argc, char *argv[]) {
    // check command line input arguments
    if (argc != 2) {
        printf("Usage: %s message_queue_key\n", argv[0]);
        exit(1);
    }

    // read the message queue key defined by user
    int msgKey = atoi(argv[1]);
    if (msgKey <= 0) {
        printf("The message queue key must be greater than zero!\n");
        exit(1);
    }

    // set the function sigHandler as handler for the signals SIGINT, SIGTERM and SIGHUP
    signal(SIGINT, signTermHandler);
    signal(SIGTERM, signTermHandler);
    signal(SIGHUP, signTermHandler);


    printf("<Server> Making MSG queue...\n");
    // get the message queue, or create a new one if it does not exist
    msqid = msgget(msgKey, IPC_CREAT | 0600);

    // check functionality
    printf("<Server> sleep...\n");
    sleep(60);
    // the process sleeps for 1 minutes. Try to send some orders
    // and check that prime users' orders are always read before
    // normal users' ones

    struct order order;
    // endless loop
    while (1) {
        // read a message from the message queue.
        errno = 0;
        if(msgrcv(msqid, (void *)&order, sizeof(order) - sizeof(order.mtype), -2, IPC_NOWAIT) == -1){
            if(errno == ENOMSG){
                printf("“Nessun ordine! Contattare ufficio marketing”\n");
                sleep(30);
                continue;
            }
            else errExit("MSGRCV Failed");
        }
        
        // print the order on standard output
        printOrder(&order);
    }

    return 0;
}
