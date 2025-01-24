//
// Created by pawel on 1/12/25.
//
#include "resources.h"

// Klucze IPC
key_t s_key, m_key, m_s_key, exit_s_key, vip_exit_key, q_key;

// Zmienne IPC
int shm_id, sem_id, m_sem_id, exit_sem_id, vip_exit_sem_id, msg_id;

sem_t child_semaphore; // Deklaracja semafora dla watków

// Struktury
struct Stadium *stadium;
struct Fan fan;
struct bufor message;

// wątek dziecka
pthread_t id_child;


// Deklaracje funkcji
void initialize_resources(); // Inicjalizacja zasobów IPC
void generate_fan_attributes(); // Generowanie losowych atrybutów fana
void try_to_find_station(int *station); // Przeszukiwanie dostępnych stanowisk
int try_to_enter_station(int i, int *station); // Próba zajęcia stanowiska
void check_stadium_full(); // Sprawdzenie, czy stadion jest pełny
void perform_control(int i); // Przeprowadzenie kontroli kibica na stanowisku
void finalize_station(int i, int *station); // Finalizacja zajmowania stanowiska
void handle_exit(); // Obsługa wyjścia fana ze stadionu
void *child(void *arg); // Obsługa dziecka w przypadku, gdy kibic jest z dzieckiem
void wait_for_entry(); // Oczekiwanie na odblokowanie wejścia na stadion
void check_passes (); // sprawdza czy kibic przepuścił już 5 kibiców
void enter_stadium(); // wpuszcza kibiców na stadion
// Funkcja główna
int main() {
    initialize_resources(); // Inicjalizacja zasobów IPC
    generate_fan_attributes(); // Generowanie losowych atrybutów fana
    check_stadium_full(); // sprawdzenie czy stadion jest pełny

    int station = 0; // flaga do znalezienia stanowiska

    // Jeśli kibic jest VIP, omija proces szukania stanowiska
    if (fan.is_vip) {
        station = 1;
    }
    // Szukanie wolnego stanowiska do kontroli
    while (station == 0) {
        check_passes(); // sprawdzenie czy kibic przepuścił już 5 innych
        try_to_find_station(&station); // proba znalezienia odpowiedniej stacji kontroli
    }

    wait_for_entry(); // Oczekiwanie na pozwolenie na wejście

    check_stadium_full(); // Sprawdzenie, czy stadion nie jest pełny

    enter_stadium(); // wpuszczenie kibiców na stadion

    handle_exit(); // Obsługa wyjścia kibica ze stadionu

    // odłączanie wątku dziecka jeżeli kibic je posiada
    if (fan.has_child) {
        if (pthread_detach(id_child) == -1) {
            printf( "Error detaching a thread\n");
            exit(1);
        }
    }
    exit(0);
}

// Funkcja generująca losowe atrybuty fana
void generate_fan_attributes() {
    srand(getpid());
    receive_message(msg_id, &message, FAN); // odbiór komunikatu z ID kibica
    fan.id = message.mvalue; // Ustawienie unikalnego ID fana
    fan.dangerous_item = (rand() % 100) < 5 ? 1 : 0; // 5% szansa na posiadanie niebezpiecznego przedmiotu
    fan.is_vip = (rand() % K < VIP) ? 1 : 0; // VIP z prawdopodobieństwem określonym przez stałą VIP
    fan.team = (rand() % 2) == 0 ? TEAM_A : TEAM_B; // Przypisanie fana do drużyny
    fan.age = (rand() % 100) + 1; // Losowy wiek fana
    fan.has_child = (rand() % 100) < 15 ? 1 : 0; // 15% szansa, że kibic ma dziecko

    wait_semaphore(m_sem_id, 0, 0);  // Zablokowanie dostępu do pamięci współdzielonej
    stadium->passing_counter[fan.id] = 0; // ustawienia licznika przepuszczeń na 0
    signal_semaphore(m_sem_id, 0); // Zwolnienie dostępu do pamięci współdzielonej

    // Jeśli kibic ma dziecko, uruchamiany jest osobny wątek
    if (fan.has_child) {
        sem_init(&child_semaphore, 0, 0);
        if (pthread_create(&id_child, NULL, child, NULL) == -1) {
            printf( "Error creating a thread\n");
            exit(1);
        }
    }
}

// Funkcja inicjalizująca zasoby IPC
void initialize_resources() {
    s_key = initialize_key('A'); // Klucz dla semaforów stanowisk
    m_key = initialize_key('B'); // Klucz dla pamięci współdzielonej
    m_s_key = initialize_key('C'); // Klucz dla semafora zarządzającego pamięcią współdzieloną
    q_key = initialize_key('D'); // klucz dla kolejki komunikatów
    exit_s_key = initialize_key('E'); // Klucz dla semafora wyjścia
    vip_exit_key = initialize_key('F'); // Klucz dla semafora wyjścia VIP

    sem_id = allocate_semaphore(s_key, NUM_STATIONS); // Alokacja semaforów stanowisk
    shm_id = initialize_shared_memory(m_key, sizeof(struct Stadium)); // Inicjalizacja pamięci współdzielonej
    m_sem_id = allocate_semaphore(m_s_key, 1); // Alokacja semafora pamięci współdzielonej
    msg_id = initialize_message_queue(q_key); // inicjalizacja kolejki komunikatów
    exit_sem_id = allocate_semaphore(exit_s_key, 1); // Alokacja semafora wyjścia
    vip_exit_sem_id = allocate_semaphore(vip_exit_key, 1); // Alokacja semafora wyjścia VIP

    stadium = (struct Stadium *) shmat(shm_id, NULL, 0); // Podłączenie pamięci współdzielonej
    if (stadium == (void *) -1) {
        perror("shmat");
        exit(1);
    }
}
void check_passes() {
    wait_semaphore(m_sem_id, 0, 0); // Zablokowanie dostępu do pamięci współdzielonej
    // sprawdzenie czy kibic przepuścił już 5 lub więcej kibiców
    if (!fan.has_child) {
        if (stadium->passing_counter[fan.id ] > 5) {
            printf("Kibic %d: drużyny: %d stał się agresywny, nie może wejść na stadion\n", fan.id, fan.team);
            signal_semaphore(m_sem_id, 0); // Zwolnienie dostępu do pamięci współdzielonej
            detach_shared_memory(stadium); // odłączenie pamięci współdzielonej
            exit(0);
        }
    } else {
        if (stadium->passing_counter[fan.id ] > 5) {
            printf("Kibic %d: drużyny: %d z dzieckiem stał się agresywny, nie mogą wejść na stadion\n", fan.id, fan.team);
            signal_semaphore(m_sem_id, 0); // Zwolnienie dostępu do pamięci współdzielonej
            detach_shared_memory(stadium); // odłączenie pamięci współdzielonej
            // odłączenie wątku dziecka
            if (pthread_detach(id_child) == -1) {
                printf("Error detaching a thread\n");
                exit(1);
            }
            exit(0);
        }
    }
    signal_semaphore(m_sem_id, 0); // Zwolnienie dostępu do pamięci współdzielonej
}
// Funkcja przeszukująca dostępne stanowiska
void try_to_find_station(int *station) {
    for (int i = 0; i < NUM_STATIONS; i++) {
        //jeżeli kibic ma dziecko to stanowisko musi miec dwa wolne miejsca
        if (!fan.has_child) {
            if (value_semaphore(sem_id, i) == 0) continue; // Jeśli stanowisko zajęte, pomiń
        } else {
            if (value_semaphore(sem_id, i) < 2) continue; // Jeśli stanowisko zajęte, pomiń
        }

        if (try_to_enter_station(i, station)) break; // Jeśli udało się zająć stanowisko, przerwij
    }
}

// Funkcja próbująca zająć stanowisko
int try_to_enter_station(int i, int *station) {
    wait_semaphore(m_sem_id, 0, 0); // Zablokowanie dostępu do pamięci współdzielonej

    // Jeśli stanowisko jest puste lub należy do tej samej drużyny, zarezerwuj je
    if (stadium->station_status[i] == 0 || stadium->station_status[i] == fan.team) {
        stadium->station_status[i] = fan.team; // Przypisanie drużyny do stanowiska

        // zwiększenie ilości przepuszczeń poprzednich 5 kibiców
        for (int i = 0; i <= fan.id; i++) {
            stadium->passing_counter[i]++;
        }
        signal_semaphore(m_sem_id, 0); // Zwolnienie dostępu do pamięci współdzielonej
        if (!fan.has_child) {
            wait_semaphore(sem_id, i, 0); // Zajęcie stanowiska
        } else {
            wait_semaphore(sem_id, i, 0); // Zajęcie stanowiska przez rodzica
            wait_semaphore(sem_id, i, 0); // Zajęcie stanowiska przez dziecko
        }
        signal_semaphore(m_sem_id, 0); // Zwolnienie dostępu do pamięci współdzielonej

        check_stadium_full(); // Sprawdzenie, czy stadion nie jest pełny

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
    wait_semaphore(m_sem_id, 0, 0); // Zablokowanie dostępu do pamięci współdzielonej
    if (!fan.has_child) {
        if (stadium->fans >= K) {
            // Sprawdzenie, czy liczba kibiców osiągnęła limit
            printf("Kibic %d: drużyny: %d nie może wejść. Stadion jest pełny.\n", fan.id, fan.team);
            detach_shared_memory(stadium); // Odłączenie pamięci współdzielonej
            signal_semaphore(m_sem_id, 0); // Zwolnienie dostępu do pamięci współdzielonej
            exit(0); // Wyjście z procesu
        }
    } else {
        if (stadium->fans >= K - 1) {
            // Sprawdzenie, czy liczba kibiców osiągnęła limit
            printf("Kibic %d drużyny: %d z dzieckiem nie może wejść. Stadion jest pełny.\n", fan.id, fan.team);
            detach_shared_memory(stadium); // Odłączenie pamięci współdzielonej
            signal_semaphore(m_sem_id, 0); // Zwolnienie dostępu do pamięci współdzielonej
            if (pthread_detach(id_child) == -1) {
                printf( "Error detaching a thread\n");
                exit(1);
            }
            exit(0); // Wyjście z procesu
        }
    }
    signal_semaphore(m_sem_id, 0); // Zwolnienie dostępu do pamięci współdzielonej
}

// Funkcja przeprowadzająca kontrolę
void perform_control(int i) {
    if (!fan.has_child) {
        printf("Kibic %d: Drużyny: %d Rozpoczęcie kontroli na stanowisku %d.\n", fan.id, fan.team, i);

        if (fan.dangerous_item == 1) {
            printf("Kibic %d: posiada niebezpieczny przedmiot. Nie wchodzi na stadion.\n", fan.id);
            signal_semaphore(sem_id, i); // zwolnienie stanowiska do kontroli
            detach_shared_memory(stadium);
            exit(0);
        }
    } else {
        printf("Kibic %d: Drużyny: %d Rozpoczęcie kontroli na stanowisku %d.\n", fan.id, fan.team, i);
        printf("Dziecko kibica: %d Rozpoczęcie kontroli na stanowisku %d.\n", fan.id, i);

        if (fan.dangerous_item == 1) {
            printf("Kibic %d: Drużyny: %d posiada niebezpieczny przedmiot. Nie wchodzi na stadion.\n", fan.id,fan.team);
            printf(
                "Dziecko kibica: %d Nie wchodzi na stadion. Rodzic posiada niebezpieczny przedmiot, nie może wejść na stadion samo\n",
                fan.id);
            signal_semaphore(sem_id, i); // zwolnienie stanowiska do kontroli przez rodzica
            signal_semaphore(sem_id, i); // zwolnienie stanowiska do kontroli przez dziecko
            if (pthread_detach(id_child) == -1) {
                printf( "Error detaching a thread\n");
                exit(1);
            }
            exit(0);
        }
    }
     sleep(rand()%10); // symulacja kontroli
}

// Funkcja finalizująca zajęcie stanowiska
void finalize_station(int i, int *station) {
    if (!fan.has_child) {
        signal_semaphore(sem_id, i); // zwolnienie miejsca na stanowisku
    } else {
        signal_semaphore(sem_id, i); // zwolnienie miejsca na stanowisku przez rodzica
        signal_semaphore(sem_id, i); // zwolnienie miejsca na stanowisku przez dziecko
    }

    signal_semaphore(m_sem_id, 0); // Zwolnienie dostępu do pamięci współdzielonej

    wait_semaphore(m_sem_id, 0, 0); // Zablokowanie dostępu do pamięci współdzielonej
    // jeżeli stanowisko jest puste to może wejść kibic dowolnej drużyny
    if (value_semaphore(sem_id, i) == 3) {
        stadium->station_status[i] = 0; // "czyszczenie stanowiska z statusu drużyny"
    }

    signal_semaphore(m_sem_id, 0); // Zwolnienie dostępu do pamięci współdzielonej
    *station = 1; // znalezienie stanowiska
}

// Funkcja oczekująca na pozwolenie wejścia
void wait_for_entry() {
    wait_semaphore(m_sem_id, 0, 0); // Zablokowanie dostępu do pamięci współdzielonej
    while (stadium->entry_status == 0) {
        // Czekaj, aż wejście zostanie odblokowane
        if (stadium->exit_status == 1) {
            // Jeśli stadion zostaje zamknięty, zakończ
            detach_shared_memory(stadium);
            signal_semaphore(m_sem_id, 0); // Zwolnienie dostępu do pamięci współdzielonej
            exit(0);
        }
    }
    signal_semaphore(m_sem_id, 0); // Zwolnienie dostępu do pamięci współdzielonej
}
void enter_stadium() {
    wait_semaphore(m_sem_id, 0, 0); // Zablokowanie dostępu do pamięci współdzielonej
    if (!fan.has_child) {
        // Kibic wchodzi na stadion
        printf("Kibic %d: Drużyny: %d VIP status: %d Pomyślnie wchodzi na stadion.\n", fan.id, fan.team, fan.is_vip);
        stadium->fans++; // Zwiększenie liczby kibiców na stadionie
    } else {
        // Kibic wchodzi na stadion
        printf("Kibic %d: Drużyny: %d VIP status: %d Pomyślnie wchodzi na stadion.\n", fan.id, fan.team, fan.is_vip);
        printf("Dziecko kibica: %d Pomyślnie wchodzi na stadion.\n", fan.id);
        stadium->fans += 2; // Zwiększenie liczby kibiców na stadionie o 2
    }
    signal_semaphore(m_sem_id, 0); // Zwolnienie dostępu do pamięci współdzielonej
}
// Funkcja obsługująca wyjście fana
void handle_exit() {
    if (fan.is_vip) {
        wait_semaphore(vip_exit_sem_id, 0, 0); // zablokowanie dostępu do wyjścia VIP ze stadionu
        printf("Kibic %d: Drużyny: %d VIP status: %d wychodzi ze stadionu wejściem VIP\n", fan.id, fan.team,
               fan.is_vip);
        // dziecko wychodzi razem z rodzicem
        if (fan.has_child) {
            sem_post(&child_semaphore); // zwolnienie semafora dla watków
            // dolaczenie watku dziecka
            if (pthread_join(id_child,NULL) == -1) {
                printf( "Error joining a thread\n");
                exit(1);
            }
        }
        signal_semaphore(vip_exit_sem_id, 0); // zwolnienie dostępu do wyjścia VIP ze stadionu
    } else {
        wait_semaphore(exit_sem_id, 0, 0); // zablokowanie dostępu do wyjścia ze stadionu
        printf("Kibic %d: Drużyny: %d VIP status: %d wychodzi ze stadionu\n", fan.id, fan.team, fan.is_vip);
        // dziecko wychodzi razem z rodzicem
        if (fan.has_child) {
            sem_post(&child_semaphore); // zwolnienie semafora dla watków
            if (pthread_join(id_child,NULL) == -1) {
                printf( "Error joining a thread\n");
                exit(1);
            }
        }
        signal_semaphore(exit_sem_id, 0); // zwolnienie dostępu do wyjścia ze stadionu
    }
}

// Funkcja obsługująca wątek dziecka
void *child(void *arg) {
    sem_wait(&child_semaphore); // podniesienia semafora dla watku dziecka
    printf("Dziecko Kibica: %d wychodzi ze stadionu\n", fan.id);
    pthread_exit(NULL); // zakonczenie watku
}
