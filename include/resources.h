//
// Created by pawel on 1/12/25.
//

#ifndef RESOURCES_H
#define RESOURCES_H

#include <stdio.h> // standardowa biblioteka wejścia/wyjścia
#include <stdlib.h> // standardowe funkcje pomocnicze
#include <sys/sem.h> // operacje na semaforach
#include <unistd.h> // funkcje systemowe i operacje na procesach
#include <sys/shm.h> // obsługa pamięci współdzielonej
#include <sys/msg.h> // obsługa kolejki komunikatów
#include <sys/ipc.h> // generowanie kluczy IPC
#include <sys/types.h> // definicje typów danych (pid_t, key_t, ...)
#include <sys/wait.h> // oczekiwanie na zakończenie procesów potomnych (wait, ...)
#include <string.h> // funkcje do operacji na ciągach znaków
#include <stdbool.h> // definicje typów logicznych
#include <time.h> // funkcje do obsługi czasu
#include <fcntl.h> // operacje na deskryptorach plików
#include <sys/stat.h> // obsługa informacji o plikach, uprawnieniach i potoku nazwanego FIFO
#include <errno.h> // obsługa kodów błędów
#include <signal.h> // obsługa sygnałów
#include <pthread.h> // obsługa wątków
#include <semaphore.h> // obsługa semaforów dla wątków
#include <stdarg.h> // obsługa zmiennej liczby argumentów
#include <sys/time.h> // funkcje do operacji na czasie (gettimeofday, ...)

#define MAX_PROCESSES 4000 // maksymalna ilość procesów którą użytkownik może stworzyć
#define NUM_STATIONS 3 // ilość stanowisk do kontroli kibiców
#define MAX_NUM_FANS 3 // maksymalna ilość kibiców na jednym stanowisku do kontroli
#define K 100 // maksymalna ilość kibiców, którzy mogą przebywać na stadionie
// podział na drużyny
#define TEAM_A 2
#define TEAM_B 1
#define FAN 1 // typ komunikatu dla kibica
#define MANAGER 2 // typ komunikatu dla kierownika
#define VIP (K * 0.005) // szansa na zostanie kibicem VIP

// struktura opisująca kibica
struct Fan {
    int id; // id kibica
    int team; // 2 - drużyna A, 1 - drużyna B
    bool dangerous_item; // posiadanie niebezpiecznego przedmiotu
    bool is_vip; // status VIP
    bool has_child; // posiadanie dziecka
};

struct Stadium {
    int fans; // ilość kibiców na stadionie
    int station_status[NUM_STATIONS]; // Tablica statusów stanowisk (0 - wolne, TEAM_A/TEAM_B - zajęte)
    int entry_status; // status wchodzenia na stadion
    int exit_status; // status opuszczania stadionu
    int passing_counter[MAX_PROCESSES]; // licznik przepuszczeń przez kibica
};

//operacje semaforowe

int allocate_semaphore(key_t key, int number);// Alokuje semafor z podanym kluczem i liczbą semaforów


void initialize_semaphore(int sem_ID, int number, int val);// Inicjalizuje semafor z podanym ID, numerem i wartością początkową


void signal_semaphore(int sem_ID, int number);// Ustawia semafor na sygnał (podnosi semafor)


void wait_semaphore(int sem_ID, int number, int flags);// Oczekuje na semafor (opuszcza semafor)


int value_semaphore(int sem_ID, int number);// Zwraca wartość semafora


void release_semaphore(int sem_ID, int number);// Zwolnienie semafora z podanym ID i numerem


key_t initialize_key(int name);// Inicjalizuje klucz IPC z podaną nazwą

//operacje na kolejce komunikatów
struct bufor {
    long mtype;
    int mvalue;
};


int initialize_message_queue(key_t key_ID);// Inicjalizuje kolejkę komunikatów z podanym kluczem


void send_message(int msg_ID, struct bufor *message);// Wysyła wiadomość do kolejki komunikatów


void receive_message(int msg_ID, struct bufor *message, int mtype);// Odbiera wiadomość z kolejki komunikatów o podanym typie


void release_message_queue(int msg_ID);// Zwolnienie kolejki komunikatów

// Operacje na pamięci współdzielonej


int initialize_shared_memory(key_t key, int size);// Inicjalizuje pamięć współdzieloną z podanym kluczem i rozmiarem


void release_shared_memory(int shm_ID);// Zwolnienie pamięci współdzielonej


void detach_shared_memory(const void *addr);// Odłączenie pamięci współdzielonej

#endif //RESOURCES_H
