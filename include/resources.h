//
// Created by pawel on 1/12/25.
//

#ifndef RESOURCES_H
#define RESOURCES_H

#include <stdio.h> // standardowa biblioteka wejscia/wyjscia
#include <stdlib.h> // standardowe funkcje pomocnicze
#include <sys/sem.h> // operacje na semaforach
#include <unistd.h> // funkcje systemowe i operacje na procesach
#include <sys/shm.h> // obsluga pamięci wspoldzielonej
#include <sys/msg.h> // obsluga kolejki komunikatow
#include <sys/ipc.h>  // Generowanie kluczy IPC
#include <sys/types.h> // Definicje typow danych (pid_t, key_t, ...)
#include <sys/wait.h> // Oczekiwanie na zakonczenie procesow potomnych (wait, ...)
#include <string.h> // Funkcje do operacji na ciągach znakow
#include <stdbool.h> // Definicje typow logicznych
#include <time.h> // Funkcje do obslugi czasu
#include <fcntl.h> // Operacje na deskryptorach plikow
#include <sys/stat.h> // Obsluga informacji o plikach, uprawnieniach i potoku nazwanego FIFO

#define FIFO_NAME "FIFO" // nazwa pliku do potoku nazwanego FIFO
#define NUM_STATIONS 3  // ilosc stanowiski do kontroli kibicow
#define MAX_NUM_FANS 3 // maksymalna ilosc kibicow na jednym stanowisku do kontroli
#define K 100 // maksymalna ilosc kibicow ktorzy moga przebywac na stadionie
// podzial na druzyny
#define TEAM_A 0
#define TEAM_B 1

#define IN 1
#define OUT 0

#define VIP (K * 0.005) // szansa na zostanie kibicem VIP

// struktura opisujaca kibica
struct Fan {
    int id; // id kibica
    int team; // 0 - drużyna A, 1 - druzyna B
    bool dangerous_item; // posiadanie niebezpiecznego przedmiotu
    int is_vip; // status VIP
};

struct Station {
    int fans_count; // Liczba kibiców na stanowisku
};

//operacje semaforowe
int allocate_semaphore(key_t key, int number);

void initialize_semaphore(int sem_ID, int number, int val);

int signal_semaphore(int sem_ID, int number);

int wait_semaphore(int sem_ID, int number, int flags);

int value_semaphore(int sem_ID, int number);

int release_semaphore(int sem_ID, int number);

key_t initialize_key(int name);


//operaceje na kolejce komunikatów
struct bufor {
    long mtype;
    int mvalue;
};

int initialize_message_queue(key_t key_ID);

void send_message(int msg_ID, struct bufor *message);

void receive_message(int msg_ID, struct bufor *message, int mtype);

int release_message_queue(int msg_ID);

//operacje na pamięci dzielonej
int initialize_shared_memory(key_t key, int size);

int release_shared_memory(int shm_ID);

int detach_shared_memory(const void *addr);
#endif //RESOURCES_H
