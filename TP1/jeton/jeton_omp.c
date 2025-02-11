#include <stdio.h>
#include <time.h>
#if defined(_OPENMP)
#include <omp.h>
#endif

int main(int argc, char** argv) {
    int token = 1;
    int num_threads = omp_get_num_threads();
    int next_thread;

    // Début du chronomètre
    double start_time = omp_get_wtime();

    #pragma omp parallel
    {
        int rank = omp_get_thread_num(); // Rank de la thread actuel
        int next_thread = (rank + 1) % num_threads;
        int prev_thread = (rank - 1 + num_threads) % num_threads;

        if (rank == 0 && token == 1){
            // Thread 0 initialise le token
            #pragma omp critical
            {
                printf("Thread %d a initialisé le token avec le valeur %d\n", rank, token);
            }

            // Envoit le token à la prochaine thread
            #pragma omp critical
            {
                printf("Thread %d a envoyé le token %d à la thread %d\n", rank, token, next_thread);
            }
        }

        // Toutes les threads attendent ici pour synchroniser
        #pragma omp barrier

        // Reçoit le token de la thread anterieur
        
        #pragma omp critical
        {
            printf("Thread %d a reçu le token %d de la thread %d\n", rank, token, prev_thread);

            // Incrementa o token
            token++;
        }
        

        // Envoit le token à la prochaine thread
        next_thread = (rank + 1) % num_threads;

        #pragma omp critical
        {
            printf("Thread %d a envoyé le token %d à la thread %d\n", rank, token, next_thread);
        }

        // Thread 0 recebe o token da última thread
        if (rank == 0 && token != 1) {
            #pragma omp barrier // Sincroniza todas as threads
            #pragma omp critical
            {
                printf("Thread %d a reçu le token %d de la thread %d\n", rank, token, num_threads - 1);
            }

            // Fim do cronômetro
            double end_time = omp_get_wtime();
            printf("Tempo decorrido: %.6f segundos\n", end_time - start_time);
        }
    }

    return 0;
}