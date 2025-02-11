#include <mpi.h>
#include <stdio.h>

int main(int argc, char** argv) {
    // initialize the MPI
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank); // get the process rank
    MPI_Comm_size(MPI_COMM_WORLD, &size); // get the number of total processes

    int token;

    // Début du chronomètre
    double start_time = MPI_Wtime();

    if (rank == 0) {
        // Process 0 initialize token with value 1
        token = 1;
        // Send token to process 1
        MPI_Send(&token, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
        printf("Processus %d a envoyé le token %d au processus 1\n", rank, token);

        // Receive the token from the last token
        MPI_Recv(&token, 1, MPI_INT, size - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        
        // Fin du chronomètre
        double end_time = MPI_Wtime();

        printf("Processus %d a reçu le token %d du processus %d\n", rank, token, size - 1);
        printf("Temps écoulé : %.6f secondes\n", end_time - start_time);
    } else {
        // Other processes receive the token from the previous token
        MPI_Recv(&token, 1, MPI_INT, rank - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        printf("Processus %d a reçu le token %d du processus %d\n", rank, token, rank - 1);

        // Increment the token
        token++;

        // Send token to the next process
        int next_rank = (rank + 1) % size; // Calcula o próximo processo no anel
        MPI_Send(&token, 1, MPI_INT, next_rank, 0, MPI_COMM_WORLD);
        printf("Processus %d a envoyé le token %d au processus %d\n", rank, token, next_rank);
    }

    // Finaliza o MPI
    MPI_Finalize();
    return 0;
}