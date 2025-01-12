//
// Created by pawel on 1/12/25.
//

#ifndef RESOURCES_H
#define RESOURCES_H
#include <stdio.h>
#include <stdlib.h>
#include <sys/sem.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

#define NUM_STATIONS 3
#define MAX_NUM_FANS 3
#define K 100

struct Fan {
    int id;
    int station;
};

//operacje semaforowe
int allocate_semaphore(key_t key, int number);
void initialize_semaphore(int sem_ID, int number, int val);
int signal_semaphore(int sem_ID, int number);
void wait_semaphore(int sem_ID, int number, int flags);
int value_semaphore(int sem_ID, int number);
int release_semaphore(int sem_ID, int number);

key_t initialize_key(int name);


//operaceje na kolejce komunikatów
struct bufor{
    long mtype;
    int mvalue;
};
int initialize_message_queue(key_t key_ID);
void send_message(int msg_ID, struct bufor* message);
void receive_message(int msg_ID, struct bufor* message, int mtype);
int release_message_queue(int msg_ID);

//operacje na pamięci dzielonej
int initialize_shered_memory(key_t key, int size);
int release_shered_memory(int shm_ID);
int detach_shered_memory(const void *addr);
#endif //RESOURCES_H
