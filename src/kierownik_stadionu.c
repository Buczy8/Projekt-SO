//
// Created by pawel on 1/15/25.
//
#include "resources.h"

#define SIGNAL_STOP 1
#define SIGNAL_RESUME 2
#define SIGNAL_EVACUATE 3

int recive_PID_message();
void recive_end_message();
void print_menu();

int main() {
    int PID_worker = recive_PID_message();
    int signal = -1;

    print_menu();

    while (1) {
        printf("\nPodaj nr sygnału: ");
        if (scanf("%d", &signal) != 1 || signal < 0) // pobranie od uzytkownika sygnału
        {
            printf("Błąd: Wprowadź poprawny numer sygnału.\n");
            while (getchar() != '\n'); // Czyszczenie bufora wejściowego
            continue;
        }

        if (signal < 1 || signal > 3) {
            printf("Błąd: Niepoprawny numer sygnału.\n");
            continue;
        }

        int result = -1;
        switch (signal) {
            case SIGNAL_STOP:
                result = kill(PID_worker, SIGUSR1); // pracownik techniczny wstzymauje wpuszczenie kibicow na stadion(sygnal1)
            break;
            case SIGNAL_RESUME:
                result = kill(PID_worker, SIGUSR2); // pracownik techniczny wznawia wpuszczanie kibiców na stadion (sygnal2)
            break;
            case SIGNAL_EVACUATE:
                result = kill(PID_worker, SIGINT); // wszyscy kibice opszuczaja stadion (koniec symulacji) (sygnal3)
            break;
        }

        if (result == -1) {
            perror("Błąd: Nie udało się wysłać sygnału");
            exit(1);
        } else {
            printf("Sygnał %d został pomyślnie wysłany do procesu o PID %d.\n", signal, PID_worker);
        }

        if (signal == SIGNAL_EVACUATE) {
            printf("Sygnał do opuszczenia stadionu wysłany.\n");
            break;
        }
    }
    recive_end_message();
    return 0;
}
void print_menu() {
    printf("Program Kierownika Stadionu\n");
    printf("Dostępne sygnały:\n");
    printf(" 1 - Wstrzymanie wpuszczania kibiców (SIGUSR1)\n");
    printf(" 2 - Wznowienie wpuszczania kibiców (SIGUSR2)\n");
    printf(" 3 - Ewakuacja stadionu (SIGINT)\n");
}
int recive_PID_message() {
    // Otwórz FIFO do odczytu
    int fifo_fd = open(FIFO_NAME, O_RDONLY);
    if (fifo_fd == -1) {
        perror("FIFO open error");
        exit(1);
    }

    // Odczytanie PID jako int
    int pid;
    ssize_t n = read(fifo_fd, &pid, sizeof(int));
    if (n > 0) {
        printf("Kierownik dostał PID pracownika technicznego: %d\n", pid);
    } else {
        perror("Read error");
    }

    close(fifo_fd);
    return pid;
}
void recive_end_message() {
    // otweranie potoku nazwanego
    int fifo_fd = open(FIFO_NAME, O_RDONLY);
    if (fifo_fd == -1) {
        perror("FIFO open error");
        exit(1);
    }

    char buffer[128]; // bufor na dane do odczytu
    ssize_t n = read(fifo_fd, buffer, sizeof(buffer) - 1); // odczytujemy dane z potoku nazwanego
    if (n > 0) {
        buffer[n] = '\0'; // Dodanie null-terminatora zeby traktowac to jako string
        printf("kierownik stadionu dostał wiadomość: %s", buffer);
    } else {
        perror("Read error");
    }
    close(fifo_fd);
}