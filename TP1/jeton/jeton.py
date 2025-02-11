import time
from mpi4py import MPI

globCom = MPI.COMM_WORLD.Dup()
nbp     = globCom.size
rank    = globCom.rank

if rank==0:
    beg = time.time()

    jeton = 1
    globCom.send(jeton, dest=1)
    jeton = globCom.recv(source=nbp-1)
    jeton += 1

    end = time.time()

    print(f"Temps pour calculer pi : {end-beg} secondes\n")
else:
    jeton = globCom.recv(source=rank-1)
    jeton += 1
    globCom.send(jeton,dest=(rank+1)%nbp)

print(f"Valeur du jeton : {jeton}")