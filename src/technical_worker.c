//
// Created by pawel on 1/12/25.
//
#include "resources.h"

int main() {
    key_t s_key, m_key;
    int shm_id, sem_id;

    s_key = initialize_key('A');
    m_key = initialize_key('B');

    shm_id = initialize_shered_memory(m_key, sizeof(struct Fan) * K);
    sem_id = allocate_semaphore(s_key, NUM_STATIONS);
    for (int i = 0; i < NUM_STATIONS; i++) {initialize_semaphore(sem_id, i,MAX_NUM_FANS );}

    struct Fan *Fans = (struct Fan *)shmat(shm_id, NULL, 0);
    for (int i = 0; i < K; i++) {
        Fans[i].id = i + 1;
        Fans[i].station = i % NUM_STATIONS;
    }
    detach_shered_memory(Fans);

    for (int i = 0; i < K; i++) {
        if (fork() == 0) {
            execl("./fan", "./fan", (char *)NULL);
            exit(0);
        }
    }

    for (int i = 0; i < K; i++) {
        wait(NULL);
    }

    release_semaphore(sem_id, NUM_STATIONS);
    release_shered_memory(shm_id);
    return 0;
}