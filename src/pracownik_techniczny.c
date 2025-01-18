//
// Created by pawel on 1/12/25.
//
#include "resources.h"

// Modyfikator volatile jest informacją dla kompilatora, że jej zawartość może się zmienić w nieznanych momentach
// sig_atomic_t - operacje odczytu i zapisu są nieprzerywalne i odbywają się w jednym kroku
volatile sig_atomic_t stop_letting_in = 0;
volatile sig_atomic_t evacuation = 0;

key_t s_key, m_key, s2_key; // deklaracja kluczy do IPC
int sem_id, shm_id, sem2_id; // deklaracja zmiennych do IPC

void send_PID_message(); // funkcja wysylajaca PID do kierownika stadionu
void send_end_message(); // funkcja wysylajaca wiadomosc koncowa do kierownika stadionu
void signal_handler(int sig); // funkcja obsługujaca poszczegolny sygnal
void signal_handling(); // funkcja obslugujaca sygnaly od kierownika
void creat_fan(); // funkcja tworzaca kibicow


int main() {
    send_PID_message(); // wyslanie PIDu procesu pracownika technicznego do kierownika stadionu
    signal_handling(); // // Rejestracja funkcji obsługi sygnałów

    s_key = initialize_key('A'); // inicjalizacja klucza do semaforow
    m_key = initialize_key('B'); // inicjalizacja klucza do pamieci wspodzielonej
    s2_key = initialize_key('C'); // inicjalizacja klucza do semafora zarzadzajcego pamiecia wspodzielona


    sem_id = allocate_semaphore(s_key, NUM_STATIONS); // alokowanie semaforow
    shm_id = initialize_shared_memory(m_key, sizeof(int)); // inicjalizacja pmieci wspodzielonej
    sem2_id = allocate_semaphore(s2_key, 1); // alokowanie semafora zarzadzajcego pamiecia wspodzielona

    for (int i = 0; i < NUM_STATIONS; i++) {
        initialize_semaphore(sem_id, i, MAX_NUM_FANS); // inicjalizcaja semaforow
    }
    initialize_semaphore(sem2_id, 0, 1); // inicjalizownie semafora zarzadzajcego pamiecia wspodzielona

    int *current_fans = (int *) shmat(shm_id, NULL, 0); // przylaczenie pamieci wspodzielonej
    if (current_fans == (void *) -1) {
        perror("Shared memory attach failed");
        exit(1);
    }

    *current_fans = 0; // ustawienie aktualnej ilosci kibiców na stadionie

    // Główna pętla nasłuchująca sygnałów
    while (!evacuation) {
        wait_semaphore(sem2_id, 0, 0); // opuszczenie semafora zeby skorzystac z pamieci wspodzielonej
        if (*current_fans == K) stop_letting_in = 1; // koniec wpuszczania kibicow po osiagnieciu pozadanej ilosci (K)
        printf("Pracownik techniczny %d: Liczba kibiców na stadionie %d\n", getpid(), *current_fans);
        signal_semaphore(sem2_id, 0); // zwolnienie semafora po uzyciu pamieci wspodzielonej
        if (stop_letting_in) {
            // Symulacja stanu "wstrzymano wpuszczanie"
            sleep(1); // Czeka na sygnał wznowienia
        } else {
            // Symulacja normalnej pracy
            creat_fan(); // wpuszczanie kibicow
            sleep(1); // Symulacja wykonywania obowiązków
        }
    }
    detach_shared_memory(current_fans); // odlaczenie pamieci wspsoldzielonej
    release_semaphore(sem_id, NUM_STATIONS); // zwolnienie semaforow
    release_shared_memory(shm_id); // zwolnienie semaforow
    release_semaphore(sem2_id, 0);
    send_end_message(); // wysylanie wiadomosci koncowj do kierownika
    return 0;
}

void send_PID_message() {
    // usuniecie FIFO jezi istnieje
    if (access(FIFO_NAME, F_OK) == 0) {
        unlink(FIFO_NAME); // Usuwa istniejący plik FIFO
    }
    // Tworzenie FIFO, jeśli nie istnieje
    if (mkfifo(FIFO_NAME, 0666) == -1) {
        perror("FIFO creation error");
        exit(1);
    }

    // Otwórz FIFO do zapisu
    int fifo_fd = open(FIFO_NAME, O_WRONLY);
    if (fifo_fd == -1) {
        perror("FIFO open error");
        exit(1);
    }

    // Przygotowanie PID jako int
    pid_t pid = getpid();
    printf("Technical worker PID: %d\n", pid);

    // Wysłanie PID przez FIFO
    if (write(fifo_fd, &pid, sizeof(int)) == -1) {
        perror("Write error");
        close(fifo_fd);
        exit(1);
    }
    close(fifo_fd);
}

void send_end_message() {
    int fifo_fd = open(FIFO_NAME, O_WRONLY);
    if (fifo_fd == -1) {
        perror("FIFO open error");
        exit(1);
    }

    char message[] = "Wszyscy kibice opuścili stadion\n";
    write(fifo_fd, message, strlen(message));
    close(fifo_fd);
    // Usunięcie potoku
    unlink(FIFO_NAME);
}

void signal_handler(int sig) {
    switch (sig) {
        case SIGUSR1:
            stop_letting_in = 1; // Wstrzymanie wpuszczania kibiców
            printf("Pracownik techniczny: Wstrzymano wpuszczanie kibiców.\n");
            break;
        case SIGUSR2:
            stop_letting_in = 0; // Wznowienie wpuszczania kibiców
            printf("Pracownik techniczny: Wznowiono wpuszczanie kibiców.\n");
            break;
        case SIGINT:
            evacuation = 1; // Ewakuacja stadionu
            printf("Pracownik techniczny: Rozpoczęto ewakuację stadionu.\n");
            break;
        default:
            printf("Pracownik techniczny: Otrzymano nieznany sygnał (%d).\n", sig);
    }
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
