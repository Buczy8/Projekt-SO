//
// Created by pawel on 1/12/25.
//
#include "resources.h"
key_t s_key, m_key, s2_key; // deklaracja kluczy do IPC
int shm_id, sem_id, m_sem_id; // deklaracja zmiennych do IPC
struct Stadium *stadium;

void initialize_resources();


int main() {
    int station = 0;
    srand(time(NULL));
    struct Fan fan; // deklaracja struktiry kibica
    fan.id = getpid();
    fan.dangerous_item = (rand() % 100) < 5 ? 1 : 0;
    fan.is_vip = (rand() % 100) < VIP ? 1 : 0;
    fan.team = (rand() % 2) == 0 ? TEAM_A : TEAM_B;
    initialize_resources();
    if (fan.is_vip) {
        station = 1;
    }
    while (station == 0) {
        // Petla dziala az do znalezienia dostępnego stanowiska
        for (int i = 0; i < NUM_STATIONS; i++) {
            if (value_semaphore(sem_id, i) == 0) continue; // sprawdzenie czy stanowisko jest wolne

            wait_semaphore(m_sem_id, 0, 0); // opuszczenie semafora zeby skorzystac z pamieci wspodzielonej
            if (stadium->station_status[i] == 0 || stadium->station_status[i] == fan.team) {
                stadium->station_status[i] = fan.team;
                signal_semaphore(m_sem_id, 0); // zwolnienie semafora po uzyciu pamieci wspodzielonej
                wait_semaphore(sem_id, i, 0); // Czekaj na dostęp do stanowiska
                wait_semaphore(m_sem_id, 0, 0); // opuszczenie semafora zeby skorzystac z pamieci wspodzielonej
                if (stadium->fans == K) {
                    detach_shared_memory(stadium);
                    signal_semaphore(m_sem_id, 0); // zwolnienie semafora po uzyciu pamieci wspodzielonej
                    exit(0);
                }
                signal_semaphore(m_sem_id, 0); // zwolnienie semafora po uzyciu pamieci wspodzielonej
                printf("Kibic %d: Drużyny: %d Rozpoczęcie kontroli na stanowisku %d.\n", fan.id, fan.team,i);
                if (fan.dangerous_item == 1) {
                    printf("Kibic %d: posiada niebezpieczny przedmiot, nie wchodzi na stadion.\n", fan.id);
                    signal_semaphore(sem_id, i); // Zwolnienie semafora
                    detach_shared_memory(stadium);
                    exit(0);
                }
                sleep(10); // Symulacja czasu kontroli
                signal_semaphore(sem_id, i); // Zwolnienie semafora
                wait_semaphore(m_sem_id, 0, 0); // opuszczenie semafora zeby skorzystac z pamieci wspodzielonej
                if (value_semaphore(sem_id,i) == 3) stadium->station_status[i]=0; // jezli stanowisko jest puste to zmieniamy mozliwośc wejsca kibicow innej druzyny
                signal_semaphore(m_sem_id, 0); // zwolnienie semafora po uzyciu pamieci wspodzielonej
                station = 1; // warunek otrzymania stanowiska
                break; // wyjście z pętli po otrzymaniu stanowiska
            } else {
                signal_semaphore(m_sem_id, 0); // zwolnienie semafora po uzyciu pamieci wspodzielonej
                continue;
            }
        }
        // Jeśli żadne stanowisko nie było dostępne, pętla while ponownie sprawdza stanowiska
    }

    wait_semaphore(m_sem_id, 0, 0); // opuszczenie semafora zeby skorzystac z pamieci wspodzielonej
    // sprawdzneie czy stadion nie jest juz zapelniony
    if (stadium->fans == K) {
        printf("Kibic %d: Nie wchodzi na stadion, ponieważ jest pełny\n", fan.id);
        detach_shared_memory(stadium); // odlaczenie pamieci wspodzielonej
        signal_semaphore(m_sem_id, 0); // zwolnienie semafora po uzyciu pamieci wspodzielonej
        exit(0); // koniec procesu kibica
    } else {
        //obsluga VIP
        if (fan.is_vip) {
            printf("Kibic %d: VIP status: %d Wchodzi na stadion wejściem VIP\n", fan.id, fan.is_vip);
        } else {
            printf("Kibic %d: Drużyny: %d VIP status: %d Pomyślnie przechodzi kontrole i wchodzi na stadion\n", fan.id, fan.team,fan.is_vip);
        }
        (stadium->fans)++;
        detach_shared_memory(stadium); // odlaczenie pamieci wspodzielonej
        signal_semaphore(m_sem_id, 0); // zwolnienie semafora po uzyciu pamieci wspodzielonej
    }

    return 0;
}

void initialize_resources() {
    s_key = initialize_key('A'); //inicjaliazacja klucza do semaforów
    m_key = initialize_key('B'); //inicjaliazacja klucza do pamieci wspodzielonej
    s2_key = initialize_key('C'); //inicjaliazacja klucza do semefora zarzadzajacego pamiecia wspoldzielona

    sem_id = allocate_semaphore(s_key, NUM_STATIONS); // alokowanie semaforow
    shm_id = initialize_shared_memory(m_key, sizeof(struct Stadium)); // inicjalizacja pamieci wspodzielonej
    m_sem_id = allocate_semaphore(s2_key, 1); // alokowanie semefora zarzadzajacego pamiecia wspoldzielona

    stadium = (struct Stadium *) shmat(shm_id, NULL, 0); // przylaczenie pamieci wspodzielonej
    if (stadium == (void *) -1) {
        perror("shmat");
        exit(1);
    }
}