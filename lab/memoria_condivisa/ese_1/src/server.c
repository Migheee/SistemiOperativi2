#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include "shared_memory.h"
#include "semaphore.h"
#include "errExit.h"

#define BUFFER_SZ 100

#define REQUEST      0
#define DATA_READY   1

int create_sem_set(key_t semkey) {
    // Create a semaphore set with 2 semaphores
    int semid = semget(semkey, 2, IPC_CREAT | S_IRUSR | S_IWUSR); 
    if (semid == -1)
        errExit("semget failed");

    // Initialize the semaphore set with semctl
    union semun arg;
    unsigned short values[] = {0, 0};
    arg.array = values;

    if(semctl(semid, 0, SETALL, arg) == -1) errExit("SETALL Failed");
    return semid;
}

void copy_file(const char *pathname, char *buffer) {
    // open in read only mode the file
    int file = open(pathname, O_RDONLY);
    if (file == -1) {
        printf("File %s does not exist\n", pathname);
        buffer[0] = -1;
        return;
    }

    // read a chunks of BUFFER_SZ - 1 characters
    ssize_t bR = read(file, buffer, BUFFER_SZ - 1);
    if (bR > 0)
        buffer[bR] = '\0'; // end the line with '\0'
    else
        printf("read failed\n");

    // close the file descriptor
    close(file);
}

int main (int argc, char *argv[]) {

    // check command line input arguments
    if (argc != 3) {
        printf("Usage: %s request_shared_memory_key semaphore_key\n", argv[0]);
        exit(1);
    }

    // read the request shared memory key defined by user
    key_t shmRequestKey = atoi(argv[1]);
    if (shmRequestKey <= 0) {
        printf("The request_shared_memory_key must be greater than zero!\n");
        exit(1);
    }

    // read the semaphore set key defined by user
    key_t semkey = atoi(argv[2]);
    if (semkey <= 0) {
        printf("The semaphore_key must be greater than zero!\n");
        exit(1);
    }

    // allocate the request shared memory segment
    printf("<Server> allocating the request shared memory segment...\n");
    int shmRequestId = alloc_shared_memory(shmRequestKey, sizeof(struct Request));

    // attach the shared memory segment
    printf("<Server> attaching request the shared memory segment...\n");
    struct Request *request = (struct Request*) get_shared_memory(shmRequestId, 0);

    // create a semaphore set
    printf("<Server> creating a semaphore set...\n");
    int semid = create_sem_set(semkey);

    // wait for a Request (REQUEST)
    printf("<Server> waiting for a request...\n");
    // ...
    semOp(semid, 0, -1);

    // get the response shared memory segment
    printf("<Server> getting the response shared memory segment...\n");
    int shmResponseId = alloc_shared_memory(request->shmResponseKey, BUFFER_SZ);

    // attach the shared memory segment
    printf("<Server> attaching the response shared memory segment...\n");
    char *buffer = (char *) get_shared_memory(shmResponseId, 0);

    // copy file into the shared memory
    printf("<Server> coping a file into the response shared memory...\n");
    copy_file(request->pathname, buffer);

    // notify that data was stored into the client's shared memory
    printf("<Server> notifing data is ready...\n");
    semOp(semid, 1, 1);

    // detach the response shared memory segment
    printf("<Client> detaching the response shared memory segment...\n");
    free_shared_memory((void *) buffer);

    // detach the request shared memory segment
    printf("<Server> detaching the request shared memory segment...\n");
    // ...
    free_shared_memory((void *) request);

    // remove the request shared memory segment
    printf("<Server> removing the request shared memory segment...\n");
    remove_shared_memory(shmRequestId);

    return 0;
}
