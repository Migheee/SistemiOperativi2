#include <sys/sem.h>

#include "semaphore.h"
#include "errExit.h"

void semOp (int semid, unsigned short sem_num, short sem_op) {
    struct sembuf sop = { 
        .sem_num = sem_num,
        .sem_flg = 0,
        .sem_op = sem_op
    };

    if (semop(semid, &sop, 1) == -1)
        errExit("semop failed");
}
