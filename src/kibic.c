//
// Created by pawel on 1/12/25.
//
#include "resources.h"
key_t s_key, m_key, s2_key; // deklaracja kluczy do IPC
int shm_id, sem_id, sem2_id; // deklaracja zmiennych do IPC

int main() {
    int *current_fans;
    int station = 0;
    srand(time(NULL));
    struct Fan fan; // deklaracja struktiry kibica
    fan.id = getpid();
    fan.dangerous_item = (rand() % 100) < 5 ? 1 : 0;

    s_key = initialize_key('A'); //inicjaliazacja klucza do semaforów
    m_key = initialize_key('B'); //inicjaliazacja klucza do pamieci wspodzielonej
    s2_key = initialize_key('C'); //inicjaliazacja klucza do semefora zarzadzajacego pamiecia wspoldzielona

    sem_id = allocate_semaphore(s_key, NUM_STATIONS); // alokowanie semaforow
    shm_id = initialize_shared_memory(m_key, sizeof(int)); // inicjalizacja pamieci wspodzielonej
    sem2_id = allocate_semaphore(s2_key, 1); // alokowanie semefora zarzadzajacego pamiecia wspoldzielona

    while (station == 0) {
        // Petla dziala az do znalezienia dostępnego stanowiska
        for (int i = 0; i < NUM_STATIONS; i++) {
            if (value_semaphore(sem_id, i) == 0) continue; // sprawdzenie czy stanowisko jest wolne

            wait_semaphore(sem_id, i, 0); // Czekaj na dostęp do stanowiska

            printf("Kibic %d: Rozpoczęcie kontroli na stanowisku %d.\n", fan.id, i);
            if (fan.dangerous_item == 1) {
                printf("Kibic %d: posiada niebezpieczny przedmiot nie wchodzi na stadion.\n", fan.id);
                signal_semaphore(sem_id, i); // Zwolnienie semafora
                exit(0);
            }
            sleep(2); // Symulacja czasu kontroli
            // Zwolnienie miejsca na stanowisku
            signal_semaphore(sem_id, i); // Zwolnienie semafora
            station = 1; // warunek otrzymania statowiska
        }
        // Jeśli żadne stanowisko nie było dostępne, pętla while ponownie sprawdza stanowiska
    }
    wait_semaphore(sem2_id, 0, 0); // opuszczenie semafora zeby skorzystac z pamieci wspodzielonej
    current_fans = (int *) shmat(shm_id, NULL, 0); // przylaczenie pamieci wspodzielonej
    // sprawdzneie czy stadion nie jest juz zapelniony
    if (*current_fans == K) {
        printf("Kibic %d: Nie wchodzi na stadion, ponieważ jest pełny\n", fan.id);
        detach_shared_memory(current_fans); // odlaczenie pamieci wspodzielonej
        signal_semaphore(sem2_id, 0); // zwolnienie semafora po uzyciu pamieci wspodzielonej
        exit(0); // koniec procesu kibica
    } else {
        printf("Kibic %d: Pomyślnie przechodzi kontrole i wchodzi na stadion\n", fan.id);
        (*current_fans)++;
        detach_shared_memory(current_fans); // odlaczenie pamieci wspodzielonej
        signal_semaphore(sem2_id, 0); // zwolnienie semafora po uzyciu pamieci wspodzielonej
    }

    return 0;
}
