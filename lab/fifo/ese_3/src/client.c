#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "request_response.h"
#include "errExit.h"

char *path2ServerFIFO = "/tmp/fifo_server";
char *baseClientFIFO = "/tmp/fifo_client.";

int Fd2ServerFIFO, Fd2ClientFIFO;

#define MAX 100

int main (int argc, char *argv[]) {

    // Step-1: The client makes a FIFO in /tmp
    char path2ClientFIFO [25];
    sprintf(path2ClientFIFO, "%s%d", baseClientFIFO, getpid());

    printf("<Client> making FIFO...\n");
    // make a FIFO with the following permissions:
    // user:  read, write
    // group: write
    // other: no permission
    if(mkfifo(path2ClientFIFO, 0640) == -1) errExit("<Client> 2ClientFIFO Creation Failed");

    printf("<Client> FIFO %s created!\n", path2ClientFIFO);

    // Step-2: The client opens the server's FIFO to send a Request
    printf("<Client> opening FIFO %s...\n", path2ServerFIFO);
    if((Fd2ServerFIFO = open(path2ServerFIFO, O_WRONLY)) == -1) errExit("<Client> 2ServerFIFO Open Failed");

    /* Intializes the random number generator */
    time_t t;
    srand((unsigned int) time(&t));

    // Prepare a request
    struct Request request;
    request.cPid = getpid();
    request.code = (int) ( ((double)rand() / RAND_MAX) * 10);

    // Step-3: The client sends a Request through the server's FIFO
    printf("<Client> sending %d\n", request.code);
    if(write(Fd2ServerFIFO, &request, sizeof(struct Request)) == -1) errExit("<Client> 2ServerFIFO Write Failed");

    // Step-4: The client opens its FIFO to get a Response
    if((Fd2ClientFIFO = open(path2ClientFIFO, O_RDONLY)) == -1) errExit("<Client> 2ClientFIFO Open Failed");

    // Step-5: The client reads a Response from the server
    struct Response response;
    if(read(Fd2ClientFIFO, &response, sizeof(struct Response)) == -1) errExit("<Client> 2ClientFIFO Read Failed");

    // Step-6: The client prints the result on terminal
    printf("<Client> The server sent the result: %d\n", response.result);

    // Step-7: The client closes its FIFO
    if(close(Fd2ClientFIFO) == -1) errExit("<Client> 2ClientFIFO Close Failed");

    // Step-8: The client removes its FIFO from the file system
    if(unlink(path2ClientFIFO) != 0) errExit("<Client> 2ClientFIFO Remove Failed"); 
}
