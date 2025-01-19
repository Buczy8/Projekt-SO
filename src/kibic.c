//
// Created by pawel on 1/12/25.
//
#include "resources.h"
key_t s_key, m_key, m_s_key, exit_s_key, vip_exit_key; // deklaracja kluczy do IPC
int shm_id, sem_id, m_sem_id, exit_sem_id, vip_exit_sem_id; // deklaracja zmiennych do IPC
struct Stadium *stadium; // deklaracja struktury stadionu
struct Fan fan; // deklaracja struktiry kibica
void initialize_resources();

void generate_fan_attributes();

int main() {
    int station = 0;
    generate_fan_attributes();
    initialize_resources();
    if (fan.is_vip) {
        station = 1;
    }
    while (station == 0) {
        // Petla dziala az do znalezienia dostępnego stanowiska
        for (int i = 0; i < NUM_STATIONS; i++) {
            if (value_semaphore(sem_id, i) == 0) continue; // sprawdzenie czy stanowisko jest wolne

            wait_semaphore(m_sem_id, 0, 0); // opuszczenie semafora zeby skorzystac z pamieci wspodzielonej

            // sprawdzenie czy na danym stanowisku jest juz kibic i czy jest z tej samej druzyny
            if (stadium->station_status[i] == 0 || stadium->station_status[i] == fan.team) {
                stadium->station_status[i] = fan.team; // przypisnie druzyny statusu stanowiska
                signal_semaphore(m_sem_id, 0); // zwolnienie semafora po uzyciu pamieci wspodzielonej
                wait_semaphore(sem_id, i, 0); // Czekaj na dostęp do stanowiska
                wait_semaphore(m_sem_id, 0, 0); // opuszczenie semafora zeby skorzystac z pamieci wspodzielonej

                //sprawdzenie przed kontrola czy stadion jest pelny
                if (stadium->fans == K) {
                    detach_shared_memory(stadium);
                    signal_semaphore(m_sem_id, 0); // zwolnienie semafora po uzyciu pamieci wspodzielonej
                    exit(0);
                }
                signal_semaphore(m_sem_id, 0); // zwolnienie semafora po uzyciu pamieci wspodzielonej

                // symulacja kontroli
                printf("Kibic %d: Drużyny: %d Rozpoczęcie kontroli na stanowisku %d.\n", fan.id, fan.team, i);
                if (fan.dangerous_item == 1) {
                    printf("Kibic %d: posiada niebezpieczny przedmiot, nie wchodzi na stadion.\n", fan.id);
                    signal_semaphore(sem_id, i); // Zwolnienie semafora
                    detach_shared_memory(stadium);
                    exit(0);
                    printf("Chuj\n");
                }
                sleep(rand() % 10); // Symulacja czasu kontroli

                signal_semaphore(sem_id, i); // Zwolnienie semafora stanowiska

                wait_semaphore(m_sem_id, 0, 0); // opuszczenie semafora zeby skorzystac z pamieci wspodzielonej
                if (value_semaphore(sem_id, i) == 3) stadium->station_status[i] = 0;
                // jezli stanowisko jest puste to zmieniamy mozliwośc wejsca kibicow innej druzyny
                signal_semaphore(m_sem_id, 0); // zwolnienie semafora po uzyciu pamieci wspodzielonej

                station = 1; // warunek otrzymania stanowiska
                break; // wyjście z pętli po otrzymaniu stanowiska
            }
            // kontynuacja szukania stanowiska
            else {
                signal_semaphore(m_sem_id, 0); // zwolnienie semafora po uzyciu pamieci wspodzielonej
                continue;
            }
        }
        // Jeśli żadne stanowisko nie było dostępne, pętla while ponownie sprawdza stanowiska
    }

    wait_semaphore(m_sem_id, 0, 0); // opuszczenie semafora zeby skorzystac z pamieci wspodzielonej
    // sprawdzneie czy stadion nie jest juz zapelniony
    if (stadium->fans == K) {
        printf("Kibic %d: Drużyny: %d VIP status: %d Nie wchodzi na stadion, ponieważ jest pełny\n", fan.id, fan.team,
               fan.is_vip);
        detach_shared_memory(stadium); // odlaczenie pamieci wspodzielonej
        signal_semaphore(m_sem_id, 0); // zwolnienie semafora po uzyciu pamieci wspodzielonej
        exit(0);
    } else {
        // sprawdzenie czy kierowniki stadionu wstrzymał wchodzenie na stadion
        while (stadium->entry_status == 0) {
            if (stadium->exit_status == 1) {
                detach_shared_memory(stadium);
                signal_semaphore(m_sem_id, 0);
                exit(0);
            } else {
                sleep(1);
            }
        }
        //obsluga VIP
        if (fan.is_vip) {
            printf("Kibic %d: Drużyny: %d VIP status: %d Wchodzi na stadion wejściem VIP\n", fan.id, fan.team,
                   fan.is_vip);
        } else {
            printf("Kibic %d: Drużyny: %d VIP status: %d Pomyślnie przechodzi kontrole i wchodzi na stadion\n", fan.id,
                   fan.team, fan.is_vip);
        }
        (stadium->fans)++;
        detach_shared_memory(stadium); // odlaczenie pamieci wspodzielonej
        signal_semaphore(m_sem_id, 0); // zwolnienie semafora po uzyciu pamieci wspodzielonej
    }

    // wyjscie ze stadionu
    if (fan.is_vip) {
        wait_semaphore(vip_exit_sem_id, 0, 0);
        printf("Kibic %d: Drużyny: %d VIP status: %d wychodzi ze stadionu wejściem VIP\n", fan.id, fan.team, fan.is_vip);
        signal_semaphore(vip_exit_sem_id, 0);
    } else {
        wait_semaphore(exit_sem_id, 0, 0);
        printf("Kibic %d: Drużyny: %d VIP status: %d wychodzi ze stadionu\n", fan.id, fan.team, fan.is_vip);
        signal_semaphore(exit_sem_id, 0);
    }
    return 0;
}

void initialize_resources() {
    s_key = initialize_key('A'); // inicjalizacja klucza do semaforow rzadajacych wejsciem na stacje kontroli
    m_key = initialize_key('B'); // inicjalizacja klucza do pamieci wspodzielonej
    m_s_key = initialize_key('C'); // inicjalizacja klucza do semafora zarzadzajcego pamiecia wspodzielona

    exit_s_key = initialize_key('E');
    vip_exit_key = initialize_key('F');


    sem_id = allocate_semaphore(s_key, NUM_STATIONS); // alokowanie semaforow
    shm_id = initialize_shared_memory(m_key, sizeof(struct Stadium)); // inicjalizacja pmieci wspodzielonej
    m_sem_id = allocate_semaphore(m_s_key, 1); // alokowanie semafora zarzadzajcego pamiecia wspodzielona

    exit_sem_id = allocate_semaphore(exit_s_key, 1);
    vip_exit_sem_id = allocate_semaphore(vip_exit_key, 1);

    stadium = (struct Stadium *) shmat(shm_id, NULL, 0); // przylaczenie pamieci wspodzielonej
    if (stadium == (void *) -1) {
        perror("shmat");
        exit(1);
    }
}

void generate_fan_attributes() {
    srand(getpid());
    fan.id = getpid();
    fan.dangerous_item = (rand() % 100) < 5 ? 1 : 0;
    fan.is_vip = (rand() % 100) < VIP ? 1 : 0;
    fan.team = (rand() % 2) == 0 ? TEAM_A : TEAM_B;
}
