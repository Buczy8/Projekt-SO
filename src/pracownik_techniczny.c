//
// Created by pawel on 1/12/25.
//
#include "resources.h"

// Modyfikator volatile jest informacją dla kompilatora, że jej zawartość może się zmienić w nieznanych momentach
// sig_atomic_t - operacje odczytu i zapisu są nieprzerywalne i odbywają się w jednym kroku
volatile sig_atomic_t stop_letting_in = 0;
volatile sig_atomic_t evacuation = 0;

key_t s_key; // deklaracja kluczy do IPC
int sem_id; // deklaracja zmiennych do IPC

void send_PID_message();
void send_end_message();
void signal_handler(int sig);
void signal_handling();
void creat_fan();


int main() {
    send_PID_message(); // wyslanie PIDu procesu pracownika technicznego do kierownika stadionu
    signal_handling(); // // Rejestracja funkcji obsługi sygnałów

    s_key = initialize_key('A'); // inicjalizacja klucza do semaforow

    sem_id = allocate_semaphore(s_key, NUM_STATIONS); // alokowanie semaforow

    for (int i = 0; i < NUM_STATIONS; i++) {
        initialize_semaphore(sem_id, i, MAX_NUM_FANS); // inicjalizcaja semaforow
    }

    // Główna pętla nasłuchująca sygnałów
    while (!evacuation) {
        if (stop_letting_in) {
            // Symulacja stanu "wstrzymano wpuszczanie"
            sleep(1); // Czeka na sygnał wznowienia
        } else {
            // Symulacja normalnej pracy
            creat_fan(); // wpuszczanie kibicow
            sleep(1); // Symulacja wykonywania obowiązków
        }
    }
    release_semaphore(sem_id, NUM_STATIONS); // zwolnienie semaforow
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
    switch (fork()) {
        case -1:
            perror("Fork error\n");
        exit(1);
        case 0:
            if (execl("./kibic", "./kibic", (char *)NULL) == -1) {
                perror("exec error\n");
                exit(1);
            }
        exit(0);
    }
}

