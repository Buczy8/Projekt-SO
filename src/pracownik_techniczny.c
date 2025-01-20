//
// Created by pawel on 1/12/25.
//
#include "resources.h"

// Modyfikator volatile jest informacją dla kompilatora, że jej zawartość może się zmienić w nieznanych momentach
// sig_atomic_t - operacje odczytu i zapisu są nieprzerywalne i odbywają się w jednym kroku
volatile sig_atomic_t stop_letting_in = 0;
volatile sig_atomic_t evacuation = 0;

key_t s_key, m_key, m_s_key, q_key, exit_key, vip_exit_key; // deklaracja kluczy do IPC
int sem_id, shm_id, m_sem_id, msg_id, exit_sem_id, vip_exit_sem_id; // deklaracja zmiennych do IPC

void send_end_message(); // funkcja wysylajaca wiadomosc koncowa do kierownika stadionu
void signal_handler(int sig); // funkcja obsługujaca poszczegolny sygnal
void signal_handling(); // funkcja obslugujaca sygnaly od kierownika
void creat_fan(); // funkcja tworzaca kibicow
void initialize_resources(); // funkcja do inicjalizacji mechanizmow IPC
void release_resources(); //funkcja do zwalnina mechanizmow IPC
void release_stadium(); // funkcja do zwalniania stadionu
void initialize_stadium(); // funkcja do inicjalizacji stadionu

struct Stadium *stadium;
struct bufor message;

int main() {
    int fan_created = 0;
    signal_handling(); // // Rejestracja funkcji obsługi sygnałów
    initialize_resources(); // inicjalizacja mechanizmow IPC
    initialize_stadium(); // inicjalizacja stadionu

    // Główna pętla nasłuchująca sygnałów
    while (!evacuation) {
        wait_semaphore(m_sem_id, 0, 0); // opuszczenie semafora zeby skorzystac z pamieci wspodzielonej
        if (stadium->fans == K) {
            stop_letting_in = 1; // koniec wpuszczania kibicow po osiagnieciu pozadanej ilosci (K)
        }
        signal_semaphore(m_sem_id, 0); // zwolnienie semafora po uzyciu pamieci wspodzielonej
        if (stop_letting_in) {
            // Symulacja stanu "wstrzymano wpuszczanie"
        } else {
            // Symulacja normalnej pracy
            // wysłanie komunikatu do kibica którym jest z rzędu
            message.mvalue = fan_created;
            send_message(msg_id, &message);
            fan_created++; // zwiekszenie stworzonych kibiców
            creat_fan(); // wpuszczanie kibicow
            sleep(1); // Symulacja wykonywania obowiązków
        }
    }
    printf("Pracownik techniczny %d: Liczba kibiców na stadionie %d\n", getpid(), stadium->fans); // wyswietlenie ilosci kibicow na stadionie
    release_stadium(); // wywołmnie funkcji do opuszania stadionu
    // czekanie az wszytkie procesy potomne zakończą swoje działanie (opuszczą stadion)
    for (int i = 0; i <= fan_created; i++) {
        wait(NULL);
    }

    send_end_message(); // wywolnie funkcji do wysłania widomosic do kierownika stadionu o tym ze wszycscy kibice opuscili stadion
    release_resources(); // wywołanie funkcji do zwalniania zasobów
    return 0;
}

void initialize_resources() {
    s_key = initialize_key('A'); // inicjalizacja klucza do semaforow rzadajacych wejsciem na stacje kontroli
    m_key = initialize_key('B'); // inicjalizacja klucza do pamieci wspodzielonej
    m_s_key = initialize_key('C'); // inicjalizacja klucza do semafora zarzadzajcego pamiecia wspodzielona
    q_key = initialize_key('D'); // inicjalizacja klucza do kolejki komunikatow
    exit_key = initialize_key('E'); //inicjalizcja klucza do semafora zarzadajacego wyjsciem ze stadionu zwyklych kibicow
    vip_exit_key = initialize_key('F'); //inicjalizcja klucza do semafora zarzadajacego wyjsciem ze stadionu kibicow VIP

    sem_id = allocate_semaphore(s_key, NUM_STATIONS); // alokowanie semaforow do zarzadania stanowiskami do kontroli
    shm_id = initialize_shared_memory(m_key, sizeof(struct Stadium)); // inicjalizacja pmieci wspodzielonej
    m_sem_id = allocate_semaphore(m_s_key, 1); // alokowanie semafora zarzadzajcego pamiecia wspodzielona
    msg_id = initialize_message_queue(q_key); // inicjalizacja kolejki komunikatow
    exit_sem_id = allocate_semaphore(exit_key, 1); //alokowanie semafora zarzadajacego wyjsciem ze stadionu zwyklych kibicow
    vip_exit_sem_id = allocate_semaphore(vip_exit_key, 1); //alokowanie semafora zarzadajacego wyjsciem ze stadionu  kibicow VIP

    for (int i = 0; i < NUM_STATIONS; i++) {
        initialize_semaphore(sem_id, i, MAX_NUM_FANS); // inicjalizcaja semaforow
    }

    initialize_semaphore(m_sem_id, 0, 1); // inicjalizacja semafora zarzadzajcego pamiecia wspodzielona
    initialize_semaphore(exit_sem_id, 0, 0); //inicjalizacja semafora zarzadajacego wyjsciem ze stadionu zwyklych kibicow
    initialize_semaphore(vip_exit_sem_id, 0, 0); //inicjalizacja semafora zarzadajacego wyjsciem ze stadionu  kibicow VIP

    // wyslanie wiadomosci z pidem pracownika technicznego do kierownika stadionu
    message.mtype = MANAGER;
    message.mvalue = getpid();
    send_message(msg_id, &message);

    message.mtype = FAN; // zmiana adresta komunikatow
    stadium = (struct Stadium *) shmat(shm_id, NULL, 0); // Przyłączenie pamięci współdzielonej
    if (stadium == (void *) -1) {
        perror("shmat failed");
        exit(1);
    }
}
// wysłanie wiadomosci koncowej do kierownika stadionu o tym ze wszyscy kibice opuscili stadion
void send_end_message() {
    message.mtype = MANAGER;
    message.mvalue = 2;
    send_message(msg_id, &message);
}

void signal_handler(int sig) {
    switch (sig) {
        case SIGUSR1:
            stop_letting_in = 1; // Wstrzymanie wpuszczania kibiców
            stadium->entry_status = 0; // zmiana statusu wpuszania kibicow na stadion
            printf("Pracownik techniczny: Wstrzymano wpuszczanie kibiców.\n");
            break;
        case SIGUSR2:
            stop_letting_in = 0; // Wznowienie wpuszczania kibiców
            stadium->entry_status = 1; // zmiana statusu wpuszania kibicow na stadion
            printf("Pracownik techniczny: Wznowiono wpuszczanie kibiców.\n");
            break;
        case SIGINT:
            evacuation = 1; // Ewakuacja stadionu
            stop_letting_in = 1; // Wstrzymanie wpuszczania kibiców
            stadium->entry_status = 0; // zmiana statusu wpuszania kibicow na stadion
            stadium->exit_status = 1; // zmiana statusu opuszania stadionu
            printf("Pracownik techniczny: Rozpoczęto ewakuację stadionu.\n");
            break;
        default:
            printf("Pracownik techniczny: Otrzymano nieznany sygnał (%d).\n", sig);
    }

    signal_handling(); // ponowne wywołnie funkcji do obsługi przechwytywania sygnałów (w Clion nie trzeba tylko przy komilacji za pomoca GCC)
}

void signal_handling() {
    if (signal(SIGUSR1, signal_handler) == SIG_ERR) {
        perror("Błąd przy rejestracji SIGUSR1");
        exit(EXIT_FAILURE);
    }
    if (signal(SIGUSR2, signal_handler) == SIG_ERR) {
        perror("Błąd przy rejestracji SIGUSR2");
        exit(EXIT_FAILURE);
    }
    if (signal(SIGINT, signal_handler) == SIG_ERR) {
        perror("Błąd przy rejestracji SIGINT");
        exit(EXIT_FAILURE);
    }
}

void creat_fan() {
    //jezeli kierownik zatrzymał wpuszanie na stadion to nie tworzymy nowych kibiców
    if (stop_letting_in) {
        printf("Wstrzymano wpuszczanie kibiców. Stadion jest pełny.\n");
        return; // Nie twórz nowego kibica
    }
    switch (fork()) {
        case -1:
            perror("Fork error\n");
            exit(1);
        case 0:
            if (execl("./kibic", "./kibic", (char *) NULL) == -1) {
                perror("exec error\n");
                exit(1);
            }
            exit(0);
    }
}

void release_resources() {
    detach_shared_memory(stadium); // odlaczenie pamieci wspsoldzielonej
    // zwolnienie semaforow
    release_semaphore(sem_id, NUM_STATIONS);
    release_semaphore(m_sem_id, 0);
    release_semaphore(exit_sem_id, 0);
    release_semaphore(vip_exit_sem_id, 0);
    release_shared_memory(shm_id); // zwolnienie pamieci wspsoldzielonej
    release_message_queue(msg_id); // zwolnieni kolejki komunikatow
}

void initialize_stadium() {
    stadium->fans = 0; // ustawienie aktualnej ilosci kibiców na stadionie
    stadium->entry_status = 1; // ustawienie flagi na wchodzenie na stadion
    stadium->exit_status = 0; // ustawienie flagi do opuszania stadionu
    for (int i = 0; i < NUM_STATIONS; i++) {
        stadium->station_status[i] = 0; // ustawienie statusu stanowiska 0 wolne; 1 team B; 2 team A
    }
    for (int i = 0; i < K; i++) {
        stadium->passing_counter[i] = 0; // wypelnienie zermai tablicy do przechowywania liczby przepuszczen dla kazdego kibica
    }
}

void release_stadium() {
    signal_semaphore(vip_exit_sem_id, 0); // podniesie semafora do opuszczania stadionu przez kibiców VIP
    signal_semaphore(exit_sem_id, 0); // podniesie semafora do opuszczania stadionu przez zwykłych kibiców
}
