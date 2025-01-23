//
// Created by pawel on 1/15/25.
//
#include "resources.h"

#define SIGNAL_STOP 1 // sygnał do wstrzymania wpuszczania kibiców
#define SIGNAL_RESUME 2 // sygnał do wznowienia wpuszczania kibiców
#define SIGNAL_EVACUATE 3 // sygnał do ewakuacji stadionu

key_t q_key; // deklaracja klucza do kolejki komunikatów
int msg_id; // deklaracja zmiennej do kolejki komunikatów

struct bufor message;

void recive_end_message(); // odbiera wiadomość końcową od pracownika technicznego po opuszczeniu stadionu przez kibiców
void print_menu(); // wyświetla menu w terminalu
int initialize_resources(); // inicjalizuje zasoby

int main() {
    int PID_worker = initialize_resources(); // pobranie PID-u pracownika technicznego do wysłania sygnałów
    int signal = -1;

    print_menu(); // wyświetlenie menu

    while (1) {
        printf("\nPodaj nr sygnału: ");
        if (scanf("%d", &signal) != 1 || signal < 0) // pobranie od użytkownika sygnału
        {
            printf("Błąd: Wprowadź poprawny numer sygnału.\n");
            while (getchar() != '\n'); // Czyszczenie bufora wejściowego
            continue;
        }
        // obsługa błędu przy podaniu złego sygnału
        if (signal < 1 || signal > 3) {
            printf("Błąd: Niepoprawny numer sygnału.\n");
            continue;
        }

        int result = -1;
        switch (signal) {
            case SIGNAL_STOP:
                result = kill(PID_worker, SIGUSR1); // pracownik techniczny wstrzymuje wpuszczenie kibiców na stadion(sygnał1)
                break;
            case SIGNAL_RESUME:
                result = kill(PID_worker, SIGUSR2); // pracownik techniczny wznawia wpuszczanie kibiców na stadion (sygnał2)
                break;
            case SIGNAL_EVACUATE:
                result = kill(PID_worker, SIGTERM); // wszyscy kibice opuszczają stadion (koniec symulacji) (sygnał3)
                break;
        }
        // obsługa błędu sygnału
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
    recive_end_message(); // otrzymanie wiadomości kończącej
    return 0;
}

void print_menu() {
    printf("Program Kierownika Stadionu\n");
    printf("Dostępne sygnały:\n");
    printf(" 1 - Wstrzymanie wpuszczania kibiców (SIGUSR1)\n");
    printf(" 2 - Wznowienie wpuszczania kibiców (SIGUSR2)\n");
    printf(" 3 - Ewakuacja stadionu (SIGTERM)\n");
}

void recive_end_message() {
    receive_message(msg_id,&message,MANAGER); // odebranie komunikatu od pracownika technicznego o zakończeniu wypuszczania kibiców
    printf("kierownik stadionu dostał wiadomość o opuszczeniu stdaionu\n");
}
int initialize_resources() {
    q_key = initialize_key('D'); // inicjalizacja klucza do kolejki komunikatów
    msg_id = initialize_message_queue(q_key); // inicjalizacja kolejki komunikatów do odbierania wiadomości
    receive_message(msg_id, &message, MANAGER); // odebranie wiadomości z PID-em pracownika technicznego
    return message.mvalue; // zwrócenie PID-u pracownika technicznego
}