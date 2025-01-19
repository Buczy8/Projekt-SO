//
// Created by pawel on 1/12/25.
//
#include "resources.h"

int allocate_semaphore(key_t key, int number) {
    int semID;
    if ((semID = semget(key, number, IPC_CREAT | 0600)) == -1) {
        perror("semaphore initialization error (semget) ");
        exit(1);
    }
    return semID;
}

void initialize_semaphore(int sem_ID, int number, int val) {
    if (semctl(sem_ID, number, SETVAL, val) == -1) {
        perror("semphore initialization error (semctl) ");
        exit(1);
    }
}

int signal_semaphore(int sem_ID, int number) {
    struct sembuf operation[1];
    operation[0].sem_num = number;
    operation[0].sem_op = 1;
    operation[0].sem_flg = 0; //SEM_UNDO;

    if (semop(sem_ID, operation, 1) == -1)
        perror("semaphore signal error (semop) ");
    return 0;
}

void wait_semaphore(int sem_ID, int number, int flags) {
    struct sembuf operation[1];
    operation[0].sem_num = number;
    operation[0].sem_op = -1;
    operation[0].sem_flg = 0 | flags; //SEM_UNDO;

    while (semop(sem_ID, operation, 1) == -1) {
        if (errno == EINTR) {
            // Operacja została przerwana przez sygnał, ponów próbę
            continue;
        } else {
            // Inny błąd, zakończ działanie z komunikatem
            perror("Semaphore wait error (semop)");
            exit(1);
        }
    }
}
int value_semaphore(int sem_ID, int number) {
    int value;
    value = semctl(sem_ID, number, GETVAL, NULL);
    if (value == -1) {
        perror("geting semaphore value error (semctl)");
        exit(1);
    }
    return value;
}

int release_semaphore(int sem_ID, int number) {
    int id;
    id = semctl(sem_ID, number, IPC_RMID, NULL);
    if (id == -1) {
        perror("releasing shered memory error (shmctl)");
        exit(1);
    }
    return id;
}

key_t initialize_key(int name) {
    key_t key;
    if ((key = ftok(".", name)) == -1) {
        printf("Blad ftok \n");
        exit(2);
    };
    return key;
}

int initialize_message_queue(key_t key) {
    int msg_ID = msgget(key, IPC_CREAT | 0600);
    if (msg_ID == -1) {
        perror("Message queue initialization error (msgget)");
        exit(1);
    }
    return msg_ID;
}

void send_message(int msg_ID, struct bufor *message) {
    if (msgsnd(msg_ID, message, sizeof(message->mvalue), 0) == -1) {
        perror("sending message error (msgsnd)");
        exit(1);
    }
}

void receive_message(int msg_ID, struct bufor *message, int mtype) {
    if (msgrcv(msg_ID, message, sizeof(message->mvalue), mtype, 0) == -1) {
        perror("receiving message error");
        exit(1);
    }
}

int release_message_queue(int msg_ID) {
    int id = msgctl(msg_ID, IPC_RMID, NULL);
    if (id == -1) {
        perror("releasing shered memory error (shmctl)");
        exit(1);
    }
    return id;
}

int initialize_shared_memory(key_t key, int size) {
    int shm_ID = shmget(key, size, IPC_CREAT | 0600);
    if (shm_ID == -1) {
        perror("initialize shered memory error");
        exit(1);
    }
    return shm_ID;
}

int release_shared_memory(int shm_ID) {
    int id;
    id = shmctl(shm_ID, IPC_RMID, NULL);
    if (id == -1) {
        perror("releasing shered memory error (shmctl)");
        exit(1);
    }
    return id;
}

int detach_shared_memory(const void *addr) {
    int id;
    id = shmdt(addr);
    if (id == -1) {
        perror("detaching shered memory error (shmdt)");
        exit(1);
    }
    return id;
}
