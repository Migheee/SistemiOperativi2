#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <sys/ipc.h>
#include <sys/stat.h>
#include <sys/sem.h>
#include <sys/shm.h>

#include "shared_memory.h"
#include "semaphore.h"
#include "errExit.h"

#define BUFFER_SZ 100

#define REQUEST      0
#define DATA_READY   1
#define CLIENT_READY 2

int main (int argc, char *argv[]) {

    // check command line input arguments
    if (argc != 4) {
        printf("Usage: %s request_shared_memory_key semaphore_key response_shared_memory_key\n", argv[0]);
        exit(1);
    }

    // read the request shared memory key

    key_t shmRequestKey = atoi(argv[1]);
    if (shmRequestKey <= 0) {
        printf("The request_shared_memory_key must be greater than zero!\n");
        exit(1);
    }




    // read the semaphore key defined by user
    key_t semkey = atoi(argv[2]);
    if (semkey <= 0) {
        printf("The semaphore_key must be greater than zero!\n");
        exit(1);
   }

    // read the response key defined by user
    key_t shmResponseKey = atoi(argv[3]);
    if (shmResponseKey <= 0) {
        printf("The response_shared_memory_key must be greater than zero!\n");
        exit(1);
    }

    // getting the request shared memory segment
    printf("<Client> getting the request shared memory segment...\n");
    int shmRequestId = alloc_shared_memory(shmRequestKey, sizeof(struct Request) );

    // attach the request shared memory segment
    printf("<Client> attaching the request shared memory segment...\n");
    struct Request *request = (struct Request *)get_shared_memory(shmRequestId, 0) ;

    // read a pathname from user
    printf("<Client> Insert pathname: ");
    scanf("%s", request->pathname);

    // allocate the response shared memory segment
    printf("<Client> allocating the response shared memory segment...\n");
    int shmResponseId = alloc_shared_memory(shmResponseKey, sizeof(struct Request) );

    // attach the response shared memory segment
    printf("<Client> attaching the response shared memory segment...\n");
    char *buffer = (char*)get_shared_memory(shmResponseId, 0) ;

    // copy shmResponseKey into the request shared memory segment
    request->shmResponseKey = shmResponseKey;

    // get the server's semaphore set
    int semid = semget(semkey, 3, IPC_CREAT | S_IRUSR | S_IWUSR);
    if (semid >= 0) {
        // unlock the server (REQUEST)
        semOp(semid, REQUEST, 1);
        int cond = 0;
        printf("<Client> reading data from the response shared memory segment...\n");
        do {
            // wait for data (DATA_READY)
            semOp(semid, DATA_READY, -1);

            // check server's response
            cond = (buffer[0] != 0 && buffer[0] != -1);
            // print data on terminal
            if (cond)
                printf("%s", buffer);

            // notify the server that data was acquired (CLIENT_READY)
            semOp(semid, CLIENT_READY, 1);
        } while (cond);
    } else
        printf("semget failed");

    // detach the request shared memory segment
    printf("<Client> detaching the request shared memory segment...\n");
    // ...
    free_shared_memory((void*) request);

    // detach the response shared memory segment
    printf("<Client> detaching the response shared memory segment...\n");
    // ...
    free_shared_memory((void *) buffer);

    // remove the response shared memory segment
    printf("<Client> removing the response shared memory segment...\n");
    // ...
    remove_shared_memory(shmResponseId) ;

    // remove the created semaphore set
    printf("<Client> removing the semaphore set...\n");
   
    if (semctl(semid, 0/*ignored*/, IPC_RMID, 0/*ignored*/) == -1) errExit("semctl failed");

    return 0;
}
