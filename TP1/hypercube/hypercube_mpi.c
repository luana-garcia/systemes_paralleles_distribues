#include <mpi.h>
#include <stdio.h>

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Verifica se o número de processos é uma potência de 2
    int d = 0;
    while ((1 << d) < size) d++; // dimension de l'hypercube (2^d = size)
    if ((1 << d) != size) {
        if (rank == 0) {
            printf("Erro: O número de processos deve ser uma potência de 2.\n");
        }
        MPI_Finalize();
        return 1;
    }

    printf("dimensao %d\n", d);

    double start_time, end_time;
    if (rank == 0) start_time = MPI_Wtime();

    int token;

    if (rank == 0) {
        token = 42; // valeur initial du token
    }

    // Difusão do token
    for (int k = 0; k < d; k++) {
        int mask = 1 << k; // Máscara para o k-ésimo bit
        int partner = rank ^ mask; // Parceiro de comunicação

        if (rank < partner && partner < size) { // rank < partner && 
            if ((rank & mask) == 0) {
                printf("Tarefa %d enviou o token %d para a tarefa %d\n", rank, token, partner);
                MPI_Send(&token, 1, MPI_INT, partner, 0, MPI_COMM_WORLD);
            } else {
                MPI_Recv(&token, 1, MPI_INT, partner, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                printf("Tarefa %d recebeu o token %d da tarefa %d\n", rank, token, partner);
            }
        }
    }

    // if (rank == 0) {
    //     end_time = MPI_Wtime();
    //     printf("Tempo total: %f segundos\n", end_time - start_time);
    // }

    MPI_Finalize();
    return 0;
}