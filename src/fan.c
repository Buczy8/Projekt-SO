//
// Created by pawel on 1/12/25.
//
#include "resources.h"

int main(){
    key_t s_key, m_key;
    int shm_id, sem_id;
    s_key = initialize_key('A');
    m_key = initialize_key('B');
    shm_id = initialize_shered_memory(m_key, sizeof(struct Fan) * K);
    sem_id = allocate_semaphore(s_key, NUM_STATIONS);

    struct Fan *Fans = (struct Fan *)shmat(shm_id, NULL, 0);
    int id = getpid() %K;
    int stanowisko = Fans[id].station;

    wait_semaphore(sem_id, stanowisko, 0);
    printf("Kibic %d jest kontrolowany na stanowisku %d\n", Fans[id].id, stanowisko);
    sleep(1); // Symulacja kontroli
    printf("Kibic %d zakończył kontrolę na stanowisku %d\n", Fans[id].id, stanowisko);
    signal_semaphore(sem_id, stanowisko);

    detach_shered_memory(Fans);

};