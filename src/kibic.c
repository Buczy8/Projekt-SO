//
// Created by pawel on 1/12/25.
//
#include "resources.h"
key_t s_key, m_key; // deklaracja kluczy do IPC
int shm_id, sem_id; // deklaracja zmiennych do IPC

void initialize_IPC_mechanisms();

int main() {
    printf("kibic: %d\n", getpid());

    s_key = initialize_key('A'); //inicjaliazacja klucza do semaforów

    sem_id = allocate_semaphore(s_key, NUM_STATIONS); // alokowanie semaforow

    // Szukanie wolnego stanowiska
    while (1) { // Petla dziala az do znalezienia dostępnego stanowiska
        for (int i = 0; i < NUM_STATIONS; i++) {
            if (value_semaphore(sem_id, i) == 0) continue; // sprawdzenie czy stanowisko jest wolne

            wait_semaphore(sem_id, i, 0); // Czekaj na dostęp do stanowiska

            printf("Kibic %d: Rozpoczęcie kontroli na stanowisku %d.\n", getpid(), i);
            sleep(10); // Symulacja czasu kontroli
            printf("Kibic %d: Zakończenie kontroli na stanowisku %d.\n", getpid(), i);

            // Zwolnienie miejsca na stanowisku
            signal_semaphore(sem_id, i); // Zwolnienie semafora
            return; // Wyjdź z funkcji po zakończeniu kontroli
        }

        // Jeśli żadne stanowisko nie było dostępne, pętla while ponownie sprawdza stanowiska
    }

    return 0;
}
