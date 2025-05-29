#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

// Broj studenata – mora biti veći od 3 (kako bi partibrejker imao šansu ući)
#define K 7

// Monitor varijable
pthread_mutex_t monitor;
pthread_cond_t red_studenti;        // uvjetna varijabla za studente
pthread_cond_t red_partibrejker;   // uvjetna varijabla za partibrejkera

// Dijeljene varijable
int stud_u_sobi = 0;
int broj_cekanje = K;
int partiB_u_sobi = 0;

// Funkcija za dretvu studenta
void* student(void* arg) {
    int id = *(int*)arg;
    free(arg); // Oslobađamo memoriju

    // Pocetno "spavanje" prije ulaska u sobu
    usleep((rand() % 401 + 100) * 1000); // 100-500ms

    for (int i = 0; i < 3; i++) {
        // STUDENT POKUŠAVA UĆI U SOBU
        pthread_mutex_lock(&monitor);
        // Čeka ako je partibrejker unutra
        while (partiB_u_sobi) {
            pthread_cond_wait(&red_studenti, &monitor);
        }

        stud_u_sobi = stud_u_sobi + 1;
        printf("Student %d je usao u sobu\n", id);
        pthread_mutex_unlock(&monitor);

        // STUDENT SE ZABAVLJA
        usleep((rand() % 1001 + 1000) * 1000); // 1000–2000ms

        // STUDENT IZLAZI IZ SOBE
        pthread_mutex_lock(&monitor);
        stud_u_sobi = stud_u_sobi - 1;
        printf("Student %d je izasao iz sobe\n", id);

        // Ako je zadnji izašao i partibrejker je unutra, signalizira mu
        if (stud_u_sobi == 0 && partiB_u_sobi) {
            pthread_cond_signal(&red_partibrejker);
        }
        pthread_mutex_unlock(&monitor);

        // ODMOR PRIJE SLJEDEĆEG KRUGA
        usleep((rand() % 1001 + 1000) * 1000); // 1000–2000ms
    }

    // STUDENT JE GOTOV
    pthread_mutex_lock(&monitor);
    broj_cekanje = broj_cekanje - 1;
    pthread_cond_signal(&red_partibrejker); // ako on čeka i nitko više neće doći
    pthread_mutex_unlock(&monitor);

    return NULL;
}

// Funkcija za dretvu partibrejkera
void* partibrejker(void* arg) {
    while (1) {
        // Povremeno dolazi i provjerava stanje
        usleep((rand() % 901 + 100) * 1000); // 100–1000ms

        pthread_mutex_lock(&monitor);

        // Ako su svi studenti završili, završava i partibrejker
        if (broj_cekanje == 0) {
            pthread_mutex_unlock(&monitor);
            break;
        }

        // Čeka dok nema barem 3 studenta u sobi
        while (stud_u_sobi < 3) {
            if (broj_cekanje == 0) {
                pthread_mutex_unlock(&monitor);
                return NULL;
            }
            pthread_cond_wait(&red_partibrejker, &monitor);
        }

        // Ulazak partibrejkera u sobu
        partiB_u_sobi = 1;
        printf("Partibrejker je usao u sobu\n");

        // Čeka dok svi studenti ne izađu
        while (stud_u_sobi > 0) {
            pthread_cond_wait(&red_partibrejker, &monitor);
        }

        // Izlazak partibrejkera iz sobe
        printf("Partibrejker je izasao iz sobe\n");
        partiB_u_sobi = 0;

        // Signalizira studentima da ponovno mogu ulaziti
        pthread_cond_broadcast(&red_studenti);

        pthread_mutex_unlock(&monitor);
    }

    return NULL;
}

// Glavni program
int main() {
    srand(time(NULL)); // Inicijalizacija generatora slučajnih brojeva

    // Inicijalizacija monitora i uvjetnih varijabli
    pthread_mutex_init(&monitor, NULL);
    pthread_cond_init(&red_studenti, NULL);
    pthread_cond_init(&red_partibrejker, NULL);

    // Kreiranje dretvi
    pthread_t studenti[K], partibrejker_tid;
    for (int i = 0; i < K; i++) {
        int* id = malloc(sizeof(int));
        *id = i + 1;
        pthread_create(&studenti[i], NULL, student, id);
    }
    pthread_create(&partibrejker_tid, NULL, partibrejker, NULL);

    // Čekanje da se dretve završe
    for (int i = 0; i < K; i++) {
        pthread_join(studenti[i], NULL);
    }
    pthread_join(partibrejker_tid, NULL);

    // Čišćenje
    pthread_mutex_destroy(&monitor);
    pthread_cond_destroy(&red_studenti);
    pthread_cond_destroy(&red_partibrejker);

    return 0;
}