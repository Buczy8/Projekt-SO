#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "resources.h"

// Klucze IPC
key_t s_key, m_key, m_s_key, exit_s_key, vip_exit_key;

// Zmienne IPC
int shm_id, sem_id, m_sem_id, exit_sem_id, vip_exit_sem_id;

// Struktury
struct Stadium *stadium;
struct Fan fan;

// Deklaracje funkcji
void initialize_resources(); // Inicjalizacja zasobów IPC
void generate_fan_attributes(); // Generowanie losowych atrybutów fana
void try_to_find_station(int *station); // Przeszukiwanie dostępnych stanowisk
int try_to_enter_station(int i, int *station); // Próba zajęcia stanowiska
void check_stadium_full(); // Sprawdzenie, czy stadion jest pełny
void perform_control(int i); // Przeprowadzenie kontroli kibica na stanowisku
void finalize_station(int i, int *station); // Finalizacja zajmowania stanowiska
void handle_exit(); // Obsługa wyjścia fana ze stadionu
void* child(void* arg); // Obsługa dziecka w przypadku, gdy kibic jest z dzieckiem
void wait_for_entry(); // Oczekiwanie na odblokowanie wejścia na stadion

// Funkcja główna
int main() {
    generate_fan_attributes(); // Generowanie losowych atrybutów fana
    initialize_resources();    // Inicjalizacja zasobów IPC

    int station = 0;

    // Jeśli kibic jest VIP, omija proces szukania stanowiska
    if (fan.is_vip) {
        station = 1;
    }

    // Szukanie wolnego stanowiska do kontroli
    while (station == 0) {
        try_to_find_station(&station);
    }

    wait_semaphore(m_sem_id, 0, 0); // Zablokowanie dostępu do pamięci współdzielonej
    wait_for_entry(); // Oczekiwanie na pozwolenie na wejście
    check_stadium_full(); // Sprawdzenie, czy stadion nie jest pełny

    // Kibic wchodzi na stadion
    printf("Kibic %d: Drużyny: %d VIP status: %d Pomyślnie wchodzi na stadion.\n", fan.id, fan.team, fan.is_vip);
    stadium->fans++; // Zwiększenie liczby kibiców na stadionie
    signal_semaphore(m_sem_id, 0); // Zwolnienie dostępu do pamięci współdzielonej

    handle_exit(); // Obsługa wyjścia kibica ze stadionu
    return 0;
}

// Funkcja generująca losowe atrybuty fana
void generate_fan_attributes() {
    srand(getpid());
    fan.id = getpid();                               // Ustawienie unikalnego ID fana
    fan.dangerous_item = (rand() % 100) < 5 ? 1 : 0; // 5% szansa na posiadanie niebezpiecznego przedmiotu
    fan.is_vip = (rand() % 100) < VIP ? 1 : 0;       // VIP z prawdopodobieństwem określonym przez stałą VIP
    fan.team = (rand() % 2) == 0 ? TEAM_A : TEAM_B;  // Przypisanie fana do drużyny
    fan.age = (rand() % 100) + 1;                    // Losowy wiek fana
    fan.has_child = (rand() % 100) < 15 ? 1 : 0;     // 15% szansa, że kibic ma dziecko

    // Jeśli kibic ma dziecko, uruchamiany jest osobny wątek
    if (fan.has_child) {
        pthread_t id_child;
        if (pthread_create(&id_child, NULL, child, &id_child) == -1) {
            fprintf(stderr, "Error creating a thread\n");
            exit(1);
        }
    }
}

// Funkcja inicjalizująca zasoby IPC
void initialize_resources() {
    s_key = initialize_key('A'); // Klucz dla semaforów stanowisk
    m_key = initialize_key('B'); // Klucz dla pamięci współdzielonej
    m_s_key = initialize_key('C'); // Klucz dla semafora zarządzającego pamięcią współdzieloną
    exit_s_key = initialize_key('E'); // Klucz dla semafora wyjścia
    vip_exit_key = initialize_key('F'); // Klucz dla semafora wyjścia VIP

    sem_id = allocate_semaphore(s_key, NUM_STATIONS); // Alokacja semaforów stanowisk
    shm_id = initialize_shared_memory(m_key, sizeof(struct Stadium)); // Inicjalizacja pamięci współdzielonej
    m_sem_id = allocate_semaphore(m_s_key, 1); // Alokacja semafora pamięci współdzielonej
    exit_sem_id = allocate_semaphore(exit_s_key, 1); // Alokacja semafora wyjścia
    vip_exit_sem_id = allocate_semaphore(vip_exit_key, 1); // Alokacja semafora wyjścia VIP

    stadium = (struct Stadium *)shmat(shm_id, NULL, 0); // Podłączenie pamięci współdzielonej
    if (stadium == (void *)-1) {
        perror("shmat");
        exit(1);
    }
}

// Funkcja przeszukująca dostępne stanowiska
void try_to_find_station(int *station) {
    for (int i = 0; i < NUM_STATIONS; i++) {
        if (value_semaphore(sem_id, i) == 0) continue; // Jeśli stanowisko zajęte, pomiń

        if (try_to_enter_station(i, station)) break; // Jeśli udało się zająć stanowisko, przerwij
    }
}

// Funkcja próbująca zająć stanowisko
int try_to_enter_station(int i, int *station) {
    wait_semaphore(m_sem_id, 0, 0); // Zablokowanie dostępu do pamięci współdzielonej

    // Jeśli stanowisko jest puste lub należy do tej samej drużyny, zarezerwuj je
    if (stadium->station_status[i] == 0 || stadium->station_status[i] == fan.team) {
        stadium->station_status[i] = fan.team; // Przypisanie drużyny do stanowiska
        signal_semaphore(m_sem_id, 0); // Zwolnienie dostępu do pamięci współdzielonej
        wait_semaphore(sem_id, i, 0); // Zajęcie stanowiska
        wait_semaphore(m_sem_id, 0, 0); // zablokowanie pamięci współdzielonej

        check_stadium_full(); // Sprawdzenie, czy stadion nie jest pełny
        signal_semaphore(m_sem_id, 0); // Zwolnienie dostępu do pamięci współdzielonej

        perform_control(i); // Przeprowadzenie kontroli

        finalize_station(i, station); // Finalizacja zajmowania stanowiska
        return 1;
    } else {
        signal_semaphore(m_sem_id, 0); // Zwolnienie dostępu do pamięci współdzielonej
        return 0;
    }
}

// Funkcja sprawdzająca, czy stadion jest pełny
void check_stadium_full() {
    if (stadium->fans >= K) { // Sprawdzenie, czy liczba kibiców osiągnęła limit
        printf("Stadion jest pełny. Kibic %d nie może wejść.\n", fan.id);
        detach_shared_memory(stadium); // Odłączenie pamięci współdzielonej
        signal_semaphore(m_sem_id, 0); // Zwolnienie dostępu do pamięci współdzielonej
        exit(0); // Wyjście z procesu
    }
}

// Funkcja przeprowadzająca kontrolę
void perform_control(int i) {
    printf("Kibic %d: Drużyny: %d Rozpoczęcie kontroli na stanowisku %d.\n", fan.id, fan.team, i);

    if (fan.dangerous_item == 1) {
        printf("Kibic %d: posiada niebezpieczny przedmiot. Nie wchodzi na stadion.\n", fan.id);
        signal_semaphore(sem_id, i); // zwolnienie stanowiska do kontroli
        detach_shared_memory(stadium);
        exit(0);
    }

    sleep(rand() % 10);
}

// Funkcja finalizująca zajęcie stanowiska
void finalize_station(int i, int *station) {
    signal_semaphore(sem_id, i); // zwolnienie miejsca na stanowisku
    signal_semaphore(m_sem_id, 0); // Zwolnienie dostępu do pamięci współdzielonej

    // jeżli stanowisko jest puste to może wejść kibic dowolnej drużyny
    if (value_semaphore(sem_id, i) == 3) {
        stadium->station_status[i] = 0; // "czyszczenie stanowiska z statusu drużyny"
    }

    signal_semaphore(m_sem_id, 0); // Zwolnienie dostępu do pamięci współdzielonej
    *station = 1; // znalezienie stanowiska
}

// Funkcja oczekująca na pozwolenie wejścia
void wait_for_entry() {
    while (stadium->entry_status == 0) { // Czekaj, aż wejście zostanie odblokowane
        if (stadium->exit_status == 1) { // Jeśli stadion zostaje zamknięty, zakończ
            detach_shared_memory(stadium);
            signal_semaphore(m_sem_id, 0); // Zwolnienie dostępu do pamięci współdzielonej
            exit(0);
        } else {
            sleep(1); // Odczekaj 1 sekundę przed ponownym sprawdzeniem
        }
    }
}
// Funkcja obsługująca wyjście fana
void handle_exit() {
    if (fan.is_vip) {
        wait_semaphore(vip_exit_sem_id, 0, 0);
        printf("Kibic %d: Drużyny: %d VIP status: %d wychodzi ze stadionu wejściem VIP\n", fan.id, fan.team, fan.is_vip);
        signal_semaphore(vip_exit_sem_id, 0);
    } else {
        wait_semaphore(exit_sem_id, 0, 0);
        printf("Kibic %d: Drużyny: %d VIP status: %d wychodzi ze stadionu\n", fan.id, fan.team, fan.is_vip);
        signal_semaphore(exit_sem_id, 0);
    }
}
// Funkcja obsługująca wątek dziecka
void* child(void* arg) {
    // ????
}
