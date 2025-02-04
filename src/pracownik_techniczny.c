//
// Created by pawel on 1/12/25.
//
#include "resources.h"

// Modyfikator volatile jest informacją dla kompilatora, że jej zawartość może się zmienić w nieznanych momentach
// sig_atomic_t - operacje odczytu i zapisu są nieprzerywalne i odbywają się w jednym kroku
volatile sig_atomic_t stop_letting_in = 0;
volatile sig_atomic_t evacuation = 0;

int processes = 0; // licznik utworzonych procesów

volatile int stop_cleaner_thread = 0; // Flaga do zatrzymania wątku
pthread_t cleaner_thread; // wątek czyszczący procesy zombie

key_t s_key, m_key, m_s_key, q_key, exit_key, vip_exit_key; // deklaracja kluczy do IPC
int sem_id, shm_id, m_sem_id, msg_id, exit_sem_id, vip_exit_sem_id; // deklaracja zmiennych do IPC

void send_end_message(); // funkcja wysyłająca wiadomość końcową do kierownika stadionu
void signal_handler(int sig); // funkcja obsługująca poszczególny sygnał
void signal_handling(); // funkcja obsługująca sygnały od kierownika
void creat_fan(); // funkcja tworząca kibiców
void initialize_resources(); // funkcja do inicjalizacji mechanizmów IPC
void release_resources(); //funkcja do zwalniania mechanizmów IPC
void release_stadium(); // funkcja do zwalniania stadionu
void initialize_stadium(); // funkcja do inicjalizacji stadionu
void terminate_kibic_processes(); // funkcja do zabijania procesów po błędzie fork
void reap_zombie_processes(); // funkcja do zabijania procesu zombie
void* zombie_cleaner_thread(void* arg); // funkcja do czyszczenia procesów zombie

struct Stadium *stadium; // struktura stadionu przechowywana w pamięci współdzielonej
struct bufor message; // struktura komunikatu

FILE *fp; // plik do obsługi polecenia systemowego

int main() {
    int fan_created = 0; // licznik stworzonych kibiców
    signal_handling(); // // Rejestracja funkcji obsługi sygnałów
    initialize_resources(); // inicjalizacja mechanizmów IPC
    initialize_stadium(); // inicjalizacja stadionu

    // Główna pętla nasłuchująca sygnałów
    while (!evacuation) {
        wait_semaphore(m_sem_id, 0, 0); // opuszczenie semafora żeby skorzystać z pamięci współdzielonej
        if (stadium->fans == K) {
            stop_letting_in = 1; // koniec wpuszczania kibiców po osiągnięciu pożądanej ilości (K)
        }
        signal_semaphore(m_sem_id, 0); // zwolnienie semafora po użyciu pamięci współdzielonej
        if (stop_letting_in) {
            // Symulacja stanu "wstrzymano wpuszczanie"
            usleep(1);
        } else {
            // Symulacja normalnej pracy
            // wysłanie komunikatu do kibica którym jest z rzędu
            wait_semaphore(m_sem_id, 0, 0); // zablokowanie dostępu do pamięci współdzielonej
            printf("Pracownik techniczny %d: Liczba kibiców na stadionie %d\n", getpid(), stadium->fans); // wyświetlenie ilości kibiców na stadionie
            signal_semaphore(m_sem_id, 0); // Zwolnienie dostępu do pamięci współdzielonej
            message.mvalue = fan_created; // przypisanie licznika kibiców do wartości komunikatu
            send_message(msg_id, &message); // wysłanie komunikatu do kibica
            fan_created++; // zwiększenie licznika stworzonych kibiców
            creat_fan(); // tworzenie nowego procesu kibica
            sleep(1); // Symulacja wykonywania obowiązków
        }
    }
    wait_semaphore(m_sem_id, 0, 0); // zablokowanie dostępu do pamięci współdzielonej
    printf("Pracownik techniczny %d: Liczba kibiców na stadionie %d\n", getpid(), stadium->fans); // wyświetlenie ilości kibiców na stadionie
    signal_semaphore(m_sem_id, 0); // Zwolnienie dostępu do pamięci współdzielonej
    release_stadium(); // zwolnienie stadionu przez kibiców
    // czekanie aż wszystkie procesy potomne zakończą swoje działanie (opuszczą stadion)
    for (int i = 0; i <= fan_created; i++) {
        wait(NULL);
    }

    send_end_message(); // wysłanie wiadomości do kierownika stadionu o tym ze wszyscy kibice opuścili stadion
    release_resources(); // zwolnienie zasobów
    return 0;
}

void initialize_resources() {
    // jeżeli kibiców jest więcej niż maksymalna ilość procesów to program wysyła wiadomość i kończy pracę
    if (K > MAX_PROCESSES) {
        fprintf(stderr, "Nie może byc więcej kibiców niz maksymalna liczba procesów\n");
        exit(1);
    }
    s_key = initialize_key('A'); // inicjalizacja klucza do semaforów rządzących wejściem na stację kontroli
    m_key = initialize_key('B'); // inicjalizacja klucza do pamięci współdzielonej
    m_s_key = initialize_key('C'); // inicjalizacja klucza do semafora zarządzającego pamięcią współdzieloną
    q_key = initialize_key('D'); // inicjalizacja klucza do kolejki komunikatów
    exit_key = initialize_key('E'); // inicjalizacja klucza do semafora zarządzającego wyjściem ze stadionu zwykłych kibiców
    vip_exit_key = initialize_key('F'); // inicjalizacja klucza do semafora zarządzającego wyjściem ze stadionu kibiców VIP

    sem_id = allocate_semaphore(s_key, NUM_STATIONS); // alokowanie semaforów do zarządzania stanowiskami do kontroli
    shm_id = initialize_shared_memory(m_key, sizeof(struct Stadium)); // inicjalizacja pamięci współdzielonej
    m_sem_id = allocate_semaphore(m_s_key, 1); // alokowanie semafora zarządzającego pamięcią współdzieloną
    msg_id = initialize_message_queue(q_key); // inicjalizacja kolejki komunikatów
    exit_sem_id = allocate_semaphore(exit_key, 1); // alokowanie semafora zarządzającego wyjściem ze stadionu zwykłych kibiców
    vip_exit_sem_id = allocate_semaphore(vip_exit_key, 1); // alokowanie semafora zarządzającego wyjściem ze stadionu kibiców VIP

    // tworzenie wątku do czyszczenia procesów zombie
    if (pthread_create(&cleaner_thread, NULL, zombie_cleaner_thread, NULL) != 0) {
        perror("Błąd podczas tworzenia wątku czyszczącego");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < NUM_STATIONS; i++) {
        initialize_semaphore(sem_id, i, MAX_NUM_FANS); // inicjalizcaja semaforow
    }

    initialize_semaphore(m_sem_id, 0, 1); // inicjalizacja semafora zarządzającego pamięcią współdzieloną
    initialize_semaphore(exit_sem_id, 0, 0); // inicjalizacja semafora zarządzającego wyjściem ze stadionu zwykłych kibiców
    initialize_semaphore(vip_exit_sem_id, 0, 0); // inicjalizacja semafora zarządzającego wyjściem ze stadionu kibiców VIP

    // wysłanie wiadomości z PID-em pracownika technicznego do kierownika stadionu
    message.mtype = MANAGER; // zmiana typu komunikatu żeby kierownik mógł go odebrać
    message.mvalue = getpid(); // przypisanie PID-u pracownika do wartości komunikatu
    send_message(msg_id, &message); // wysłanie komunikatu

    message.mtype = FAN; // zmiana typu komunikatu żeby kibice mogli go odbierać
    stadium = (struct Stadium *) shmat(shm_id, NULL, 0); // Przyłączenie pamięci współdzielonej
    if (stadium == (void *) -1) {
        perror("shmat failed");
        exit(1);
    }
}
void send_end_message() {
    message.mtype = MANAGER; // zmiana typu komunikatu żeby kierownik mógł go odebrać
    send_message(msg_id, &message); // wysłanie komunikatu
}

void signal_handler(int sig) {
    switch (sig) {
        case SIGUSR1:
            stop_letting_in = 1; // Wstrzymanie wpuszczania kibiców
        wait_semaphore(m_sem_id, 0, 0); // Zablokowanie dostępu do pamięci współdzielonej
        stadium->entry_status = 0; // zmiana statusu wpuszczania kibiców na stadion
        printf("Pracownik techniczny %d: Liczba kibiców na stadionie %d\n", getpid(), stadium->fans); // wyświetlenie ilości kibiców na stadionie
        signal_semaphore(m_sem_id, 0); // Zwolnienie dostępu do pamięci współdzielonej
        printf("Pracownik techniczny: Wstrzymano wpuszczanie kibiców.\n");
        break;
        case SIGUSR2:
            stop_letting_in = 0; // Wznowienie wpuszczania kibiców
        wait_semaphore(m_sem_id, 0, 0); // Zablokowanie dostępu do pamięci współdzielonej
        stadium->entry_status = 1; // zmiana statusu wpuszczania kibiców na stadion
        signal_semaphore(m_sem_id, 0); // Zwolnienie dostępu do pamięci współdzielonej
        printf("Pracownik techniczny: Wznowiono wpuszczanie kibiców.\n");
        break;
        case SIGTERM:
            evacuation = 1; // Ewakuacja stadionu
        stop_letting_in = 1; // Wstrzymanie wpuszczania kibiców
        wait_semaphore(m_sem_id, 0, 0); // Zablokowanie dostępu do pamięci współdzielonej
        stadium->entry_status = 0; // zmiana statusu wpuszczania kibiców na stadion
        stadium->exit_status = 1; // zmiana statusu opuszczania stadionu
        signal_semaphore(m_sem_id, 0); // Zwolnienie dostępu do pamięci współdzielonej
        printf("Pracownik techniczny: Rozpoczęto ewakuację stadionu.\n");
        break;
        default:
            printf("Pracownik techniczny: Otrzymano nieznany sygnał (%d).\n", sig);
    }

    signal_handling(); // ponowne wywołanie funkcji do obsługi przechwytywania sygnałów
}

void signal_handling() {
    // Rejestracja obsługi sygnału SIGUSR1
    if (signal(SIGUSR1, signal_handler) == SIG_ERR) {
        perror("Błąd przy rejestracji SIGUSR1");
        exit(EXIT_FAILURE);
    }
    // Rejestracja obsługi sygnału SIGUSR2
    if (signal(SIGUSR2, signal_handler) == SIG_ERR) {
        perror("Błąd przy rejestracji SIGUSR2");
        exit(EXIT_FAILURE);
    }
    // Rejestracja obsługi sygnału SIGTERM
    if (signal(SIGTERM, signal_handler) == SIG_ERR) {
        perror("Błąd przy rejestracji SIGTERM");
        exit(EXIT_FAILURE);
    }
}

void creat_fan() {
    //jeżeli kierownik zatrzymał wpuszczanie na stadion to nie tworzymy nowych kibiców
    if (stop_letting_in) {
        printf("Pracownik techniczny: %d Wstrzymano wpuszczanie kibiców.\n", getpid());
        return; // Nie twórz nowego kibica
    }
    // sprawdzenie czy ilość procesów nie przekroczyła maksymalnej wartości
    if (processes > MAX_PROCESSES) {
        fprintf(stderr, "Osiągnięto maksymlną liczbe procesów\n");
            terminate_kibic_processes(); // zabicie wszystkich procesów kibica
            release_resources(); // zwolnienie zasobów
            exit(1);
    }
    switch (fork()) {
        case -1:
            stop_letting_in = 1; // Wstrzymanie wpuszczania kibiców w przypadku błędu
        perror("Fork error\n");
        release_resources(); // Zwolnienie zasobów
        terminate_kibic_processes(); // Zakończenie procesów kibiców
        exit(1);
        case 0:
        if (execl("./kibic", "./kibic", (char *) NULL) == -1) {
            perror("exec error\n");
            exit(1);
        }
        exit(0);
        default:
            processes++; // zwieszenie liczby procesów
    }
}
void terminate_kibic_processes() {
    int status;
    // Wykonanie polecenia systemowego pkill
    status = system("pkill -f ./kibic");

    // Sprawdzenie statusu wykonania polecenia
    if (status == -1) {
        perror("Failed to execute pkill command");
        exit(1);
    }
}

void release_resources() {
    stop_cleaner_thread = 1; // Zatrzymanie wątku sprzątającego
    pthread_join(cleaner_thread, NULL); // Oczekiwanie na zakończenie wątku sprzątającego
    detach_shared_memory(stadium); // Odłączenie pamięci współdzielonej

    // Zwolnienie semaforów
    release_semaphore(sem_id, NUM_STATIONS); // Zwolnienie semaforów zarządzających stanowiskami do kontroli
    release_semaphore(m_sem_id, 0); // Zwolnienie semafora zarządzającego pamięcią współdzieloną
    release_semaphore(exit_sem_id, 0); // Zwolnienie semafora zarządzającego wyjściem ze stadionu zwykłych kibiców
    release_semaphore(vip_exit_sem_id, 0); // Zwolnienie semafora zarządzającego wyjściem ze stadionu kibiców VIP

    release_shared_memory(shm_id); // Zwolnienie pamięci współdzielonej
    release_message_queue(msg_id); // Zwolnienie kolejki komunikatów
}

void initialize_stadium() {
    stadium->fans = 0; // ustawienie aktualnej liczby kibiców na stadionie
    stadium->entry_status = 1; // ustawienie flagi na wchodzenie na stadion
    stadium->exit_status = 0; // ustawienie flagi do opuszczania stadionu
    for (int i = 0; i < NUM_STATIONS; i++) {
        stadium->station_status[i] = 0; // ustawienie statusu stanowiska: 0 wolne; 1 team B; 2 team A
    }
    for (int i = 0; i < K; i++) {
        stadium->passing_counter[i] = 0; // wypełnienie zerami tablicy do przechowywania liczby przepuszczeń dla każdego kibica
    }
}
void* zombie_cleaner_thread(void* arg) {
    while (!stop_cleaner_thread) {
        int status;
        pid_t pid;

        // Sprawdza, czy jakiekolwiek dziecko zakończyło pracę
        while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
            //printf("Zombi proces wyczyszczony: PID = %d\n", pid);
        }
        // Krótkie opóźnienie, aby uniknąć ciągłego obciążania CPU
        usleep(100000);
    }
    return NULL;
}
void release_stadium() {
    signal_semaphore(vip_exit_sem_id, 0); // podniesie semafora do opuszczania stadionu przez kibiców VIP
    signal_semaphore(exit_sem_id, 0); // podniesie semafora do opuszczania stadionu przez zwykłych kibiców
}
