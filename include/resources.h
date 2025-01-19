//
// Created by pawel on 1/12/25.
//

#ifndef RESOURCES_H
#define RESOURCES_H

#include <stdio.h> // standardowa biblioteka wejscia/wyjscia
#include <stdlib.h> // standardowe funkcje pomocnicze
#include <sys/sem.h> // operacje na semaforach
#include <unistd.h> // funkcje systemowe i operacje na procesach
#include <sys/shm.h> // obsluga pamieci wspoldzielonej
#include <sys/msg.h> // obsluga kolejki komunikatow
#include <sys/ipc.h>  // Generowanie kluczy IPC
#include <sys/types.h> // Definicje typow danych (pid_t, key_t, ...)
#include <sys/wait.h> // Oczekiwanie na zakonczenie procesow potomnych (wait, ...)
#include <string.h> // Funkcje do operacji na ciagach znakow
#include <stdbool.h> // Definicje typow logicznych
#include <time.h> // Funkcje do obslugi czasu
#include <fcntl.h> // Operacje na deskryptorach plikow
#include <sys/stat.h> // Obsluga informacji o plikach, uprawnieniach i potoku nazwanego FIFO
#include <errno.h>
#include <signal.h>
#include <pthread.h>

#define FIFO_NAME "FIF0"
#define NUM_STATIONS 3  // ilosc stanowiski do kontroli kibicow
#define MAX_NUM_FANS 3 // maksymalna ilosc kibicow na jednym stanowisku do kontroli
#define K 10 // maksymalna ilosc kibicow ktorzy moga przebywac na stadionie
// podzial na druzyny
#define TEAM_A 2
#define TEAM_B 1

#define VIP (K * 0.005) // szansa na zostanie kibicem VIP

// struktura opisujaca kibica
struct Fan {
    int id; // id kibica
    int team; // 0 - drużyna A, 1 - druzyna B
    bool dangerous_item; // posiadanie niebezpiecznego przedmiotu
    bool is_vip; // status VIP
    int age; // wiek kibica
    bool has_child; // posiadanie diecka
};

struct Stadium {
    int fans; // ilosc kibicow na stadionie
    int station_status[NUM_STATIONS]; // Tablica statusow stanowisk (0 - wolne, TEAM_A/TEAM_B - zajęte)
    int entry_status; // status wchodzenia na stadion
    int exit_status; // status opuszcznia stadionu
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
