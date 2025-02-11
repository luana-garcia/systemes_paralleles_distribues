# Produit matrice-vecteur v = A.u
import numpy as np
from mpi4py import MPI

# Initialisation du MPI
comm = MPI.COMM_WORLD
rank = comm.Get_rank()
nbp = comm.Get_size()

deb_total = MPI.Wtime()

# Dimension du problème (peut-être changé)
dim = 120

if dim % nbp != 0:
    print(f"Must have a number of processes which divides the dimension {Nloc} of the vectors")
    comm.Abort(-1)

# Division du travail entre les processus
Nloc = dim // nbp
start_column = rank * Nloc
end_column = (rank + 1) * Nloc if rank != nbp - 1 else dim

# Initialisation de la matrice locale
A_local = np.array([[(i + j) % dim + 1. for j in range(start_column, end_column)] for i in range(dim)])

# Initialisation du vecteur u
u = np.array([i+1. for i in range(dim)])

# Calcul de l'ensemble de mandelbrot :
deb = MPI.Wtime()
v_local = np.zeros(dim, dtype=np.double)
for j in range(Nloc):
    v_local += A_local[:, j] * u[start_column + j]
fin = MPI.Wtime()
print(f"Temps du calcul sur le processus {rank} : {fin-deb}")

# Rassemblement des résultats sur tous les processus
v_global = np.zeros(dim, dtype=np.double)
comm.Allreduce([v_local, MPI.DOUBLE], [v_global, MPI.DOUBLE], op=MPI.SUM)

# Constitution de l'image résultante sur le processus racine
if rank == 0:   
    print(f"Temps total : {MPI.Wtime() - deb_total}")
    # print(f"v = {v_global}")
