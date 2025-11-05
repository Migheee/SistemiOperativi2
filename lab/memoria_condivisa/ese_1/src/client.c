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

// Semaphore names
#define REQUEST      0
#define DATA_READY   1

int main (int argc, char *argv[]) {

    // check command line input arguments
    if (argc != 4) {
        printf("Usage: %s request_shared_memory_key semaphore_key response_shared_memory_key\n", argv[0]);
        exit(1);
    }

    // read the server's shared memory key
    key_t shmKeyServer = atoi(argv[1]);
    if (shmKeyServer <= 0) {
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

    // get the request shared memory segment
    printf("<Client> getting the request shared memory segment...\n");
    int shmidServer = alloc_shared_memory(shmKeyServer, sizeof(struct Request));

    // attach the shared memory segment
    printf("<Client> attaching the request shared memory segment...\n");
    struct Request *request = (struct Request *) get_shared_memory(shmidServer, 0);

    // read a pathname from user
    printf("<Client> Insert pathname: ");
    scanf("%s", request->pathname);

    // allocate the response shared memory segment
    printf("<Client> allocating the response shared memory segment...\n");
    int shmidClient = alloc_shared_memory(shmResponseKey, BUFFER_SZ);

    // attach the response shared memory segment
    printf("<Client> attaching the response shared memory segment...\n");
    char *buffer = (char *) get_shared_memory(shmidClient, 0);

    // copy shmResponseKey into the request shared memory segment
    request->shmResponseKey = shmResponseKey;

    // get the server's semaphore set
    
    int semid = semget(semkey, 2 , IPC_CREAT | S_IRUSR | S_IWUSR);

    if (semid > 0) {

        semOp(semid, 0, 1);
        // wait for data
        semOp(semid, 1, -1);

        printf("<Client> reading data from the response shared memory segment...\n");
        if (buffer[0] == -1)
            printf("File %s does not exist\n", request->pathname);
        else
            printf("%s\n", buffer);
    } else
        printf("semget failed");

    // detach the request shared memory segment
    printf("<Client> detaching the request shared memory segment...\n");
    free_shared_memory((void*) request);

    // detach the response shared memory segment
    printf("<Client> detaching the response shared memory segment...\n");
    free_shared_memory((void*) buffer);

    // remove the response shared memory segment
    printf("<Client> removing the response shared memory segment...\n");
    remove_shared_memory(shmidClient);

    // remove the created semaphore set
    printf("<Client> removing the semaphore set...\n");
    if (semctl(semid, 0, IPC_RMID, 0) == -1)
        errExit("semctl IPC_RMID failed");

    return 0;
}
