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

    def __contains__(self, c: complex) -> bool:
        return self.stability(c) == 1

    def convergence(self, c: complex, smooth=False, clamp=True) -> float:
        value = self.count_iterations(c, smooth)/self.max_iterations
        return max(0.0, min(value, 1.0)) if clamp else value

    def count_iterations(self, c: complex,  smooth=False) -> int | float:
        z:    complex
        iter: int

        # On vérifie dans un premier temps si le complexe
        # n'appartient pas à une zone de convergence connue :
        #   1. Appartenance aux disques  C0{(0,0),1/4} et C1{(-1,0),1/4}
        if c.real*c.real+c.imag*c.imag < 0.0625:
            return self.max_iterations
        if (c.real+1)*(c.real+1)+c.imag*c.imag < 0.0625:
            return self.max_iterations
        #  2.  Appartenance à la cardioïde {(1/4,0),1/2(1-cos(theta))}
        if (c.real > -0.75) and (c.real < 0.5):
            ct = c.real-0.25 + 1.j * c.imag
            ctnrm2 = abs(ct)
            if ctnrm2 < 0.5*(1-ct.real/max(ctnrm2, 1.E-14)):
                return self.max_iterations
        # Sinon on itère
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

rows_per_process = height // size  # Cada processo pega um bloco contínuo
extra_rows = height % size  # Processos extras pegam 1 linha a mais

# Definir limites para cada processo
if rank < extra_rows:
    start_row = rank * (rows_per_process + 1)
    end_row = start_row + rows_per_process + 1
else:
    start_row = rank * rows_per_process + extra_rows
    end_row = start_row + rows_per_process

num_rows_local = end_row - start_row  # Quantidade de linhas processadas localmente
local_convergence = np.empty((num_rows_local, width), dtype=np.double)

deb_total = MPI.Wtime()

# Computar Mandelbrot nas linhas atribuídas
for y_local, y in enumerate(range(start_row, end_row)):
    for x in range(width):
        c = complex(-2. + scaleX * x, -1.125 + scaleY * y)
        local_convergence[y_local, x] = mandelbrot_set.convergence(c, smooth=True)

# Processo 0 precisa receber os dados de todos os processos
if rank == 0:
    global_convergence = np.empty((height, width), dtype=np.double)
    
    # Criar arrays de contagem e deslocamento para `Gatherv`
    recvcounts = np.array([(rows_per_process + 1 if i < extra_rows else rows_per_process) * width for i in range(size)], dtype=int)
    displacements = np.array([sum(recvcounts[:i]) for i in range(size)], dtype=int)
else:
    global_convergence = None
    recvcounts = None
    displacements = None

# Usar `Gatherv` para juntar os dados corretamente
comm.Gatherv(local_convergence, 
             [global_convergence, recvcounts, displacements, MPI.DOUBLE] if rank == 0 else None, 
             root=0)

if rank == 0:
    fin = MPI.Wtime()
    print(f"Temps total : {fin - deb_total}")
    # Création de l'image
    image = Image.fromarray(np.uint8(matplotlib.cm.plasma(global_convergence) * 255))
    image.save('mandel_mpi_cyclique.png')
