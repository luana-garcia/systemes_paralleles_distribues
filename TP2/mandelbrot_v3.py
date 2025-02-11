from mpi4py import MPI
import numpy as np
from dataclasses import dataclass
from PIL import Image
from math import log
import matplotlib.cm

@dataclass
class MandelbrotSet:
    max_iterations: int
    escape_radius:  float = 2.0

    def convergence(self, c: complex, smooth=False) -> float:
        z = 0
        for iter in range(self.max_iterations):
            z = z*z + c
            if abs(z) > self.escape_radius:
                if smooth:
                    return iter + 1 - log(log(abs(z)))/log(2)
                return iter
        return self.max_iterations

# Initialisation du MPI
comm = MPI.COMM_WORLD
rank = comm.Get_rank()
size = comm.Get_size()

mandelbrot_set = MandelbrotSet(max_iterations=50, escape_radius=10)
width, height = 1024, 1024
scaleX, scaleY = 3./width, 2.25/height

if rank == 0:
    convergence = np.empty((width, height), dtype=np.double)
    deb_total = MPI.Wtime()

    # Enviar tarefas iniciais aos trabalhadores
    num_workers = min(size - 1, height)
    for i in range(1, num_workers + 1):
        comm.send(i - 1, dest=i, tag=1)  # Envia uma linha para cada worker

    # Distribuir novas linhas conforme os trabalhadores terminam
    for y in range(num_workers, height):
        status = MPI.Status()
        line, data = comm.recv(source=MPI.ANY_SOURCE, tag=2, status=status)
        convergence[:, line] = data
        comm.send(y, dest=status.Get_source(), tag=1)  # Envia nova linha para processamento

    # Coletar as últimas respostas
    for _ in range(num_workers):
        line, data = comm.recv(source=MPI.ANY_SOURCE, tag=2)
        convergence[:, line] = data
    
    fin = MPI.Wtime()
    print(f"Temps du calcul sur le processus {rank} : {fin-deb_total}")

    # Finalizar workers
    for i in range(1, size):
        comm.send(None, dest=i, tag=1)

    # Criar e salvar a imagem
    normalized = (convergence - np.min(convergence)) / (np.max(convergence) - np.min(convergence))
    image = Image.fromarray(np.uint8(matplotlib.cm.plasma(normalized.T) * 255))
    image.save('mandel_mpi_master.png')
    print(f"Temps de constitution de l'image : {MPI.Wtime() - fin}")
    print(f"Temps total : {MPI.Wtime() - deb_total}")

else:
    while True:
        y = comm.recv(source=0, tag=1)
        if y is None:
            break
        row = np.array([mandelbrot_set.convergence(complex(-2. + scaleX * x, -1.125 + scaleY * y), smooth=True) for x in range(width)])

        comm.send((y, row), dest=0, tag=2)
