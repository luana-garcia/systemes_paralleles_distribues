#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#if defined(_OPENMP)
#include <omp.h>
#endif

// gcc -fopenmp -o pi_omp compute_pi_omp.c
// OMP_NUM_THREADS=4 ./pi_omp

// Fonction pour générer un nombre aléatoire entre -1 et 1
double random_double() {
    return (double)rand() / RAND_MAX * 2.0 - 1.0;
}

int main(int argc, char** argv) {
    // Nombre total de points à générer (passé en argument)
    long long total_points = 1000000;
    if (argc > 1) {
        total_points = atoll(argv[1]);
    }

    // Initialisation du générateur de nombres aléatoires
    srand(time(NULL));

    // Compteur de points dans le cercle
    long long points_in_circle = 0;

    // Début du chronomètre
    double start_time = omp_get_wtime();

    #pragma omp parallel
    {
        // Chaque thread a son prope counteur local
        long long local_points_in_circle = 0;

        // num de threads
        int num_threads = omp_get_num_threads();

        // Génération des points et comptage
        #pragma omp for
        for (long long i = 0; i < total_points; i++) {
            double x = random_double();
            double y = random_double();
            if (x * x + y * y <= 1.0) {
                local_points_in_circle++;
            }
        }

        #pragma omp atomic
        points_in_circle += local_points_in_circle;
    }

    // Fin du chronomètre
    double end_time = omp_get_wtime();

    double pi_estimate = 4.0 * (double)points_in_circle / (double)total_points;
    printf("Estimation de pi : %.10f\n", pi_estimate);
    printf("Temps écoulé : %.6f secondes\n", end_time - start_time);

    return 0;
}