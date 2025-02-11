#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// mpicc -o pi compute_pi_mpi.c
// mpirun -np 4 ./pi

// Fonction pour générer un nombre aléatoire entre -1 et 1
double random_double() {
    return (double)rand() / RAND_MAX * 2.0 - 1.0;
}

int main(int argc, char** argv) {
    // Initialisation de MPI
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank); // Rank du processus
    MPI_Comm_size(MPI_COMM_WORLD, &size); // Nombre total de processus

    // Nombre total de points à générer (passé en argument)
    long long total_points = 1000000;
    if (argc > 1) {
        total_points = atoll(argv[1]);
    }

    // Nombre de points par processus
    long long points_per_process = total_points / size;

    // Initialisation du générateur de nombres aléatoires
    srand(time(NULL) + rank);

    // Compteur de points dans le cercle
    long long points_in_circle = 0;

    // Début du chronomètre
    double start_time = MPI_Wtime();

    // Génération des points et comptage
    for (long long i = 0; i < points_per_process; i++) {
        double x = random_double();
        double y = random_double();
        if (x * x + y * y <= 1.0) {
            points_in_circle++;
        }
    }

    // Communication des résultats au processus maître
    long long total_points_in_circle;
    MPI_Reduce(&points_in_circle, &total_points_in_circle, 1, MPI_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);

    // Fin du chronomètre
    double end_time = MPI_Wtime();

    // Calcul de pi par le processus maître
    if (rank == 0) {
        double pi_estimate = 4.0 * (double)total_points_in_circle / (double)total_points;
        printf("Estimation de pi : %.10f\n", pi_estimate);
        printf("Temps écoulé : %.6f secondes\n", end_time - start_time);
    }

    // Finalisation de MPI
    MPI_Finalize();
    return 0;
}