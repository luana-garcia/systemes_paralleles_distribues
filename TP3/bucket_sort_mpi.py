import numpy as np
from mpi4py import MPI

# Inicialize MPI
comm = MPI.COMM_WORLD
rank = comm.Get_rank()
nbp = comm.Get_size()

deb_total = MPI.Wtime()

# Total quantity of number at the list
N = 120

# Number should divide nbp
if N%nbp != 0:
    print("Le nombre de processeur doit diviser le nombre d'éléments à trier.")
    comm.Abort(-1)

deb = MPI.Wtime()

# Create local array with the Nloc and sort it
NLoc  = N//nbp

arr_local = np.random.randint(0, 500, size=NLoc,dtype=np.int64)

loc_sorted_arr = np.sort(arr_local)

print(f"Array ordenado {rank}:", loc_sorted_arr)

# Calculate the distribution of data into quatiles
loc_quartiles = np.quantile(loc_sorted_arr, np.linspace(0, 1, nbp+1))

# Colect quantiles from all processes and unifiy them
all_quartiles = np.zeros(nbp * (nbp + 1), dtype=np.float64)

comm.Allgather([loc_quartiles, MPI.DOUBLE], [all_quartiles, MPI.DOUBLE])

# Sort all quartiles and separate them for each process
global_quartiles = np.sort(all_quartiles)

interval_start = global_quartiles[rank * (nbp + 1) - 1] if rank != 0 else global_quartiles[0]

interval_end = global_quartiles[(rank + 1) * (nbp + 1) - 1]

# Filter local array to include only elements within the interval, and verify if it's the last process to include the last interval
if rank == nbp - 1:  # Last process
    filtered_arr = loc_sorted_arr[(loc_sorted_arr >= interval_start) & (loc_sorted_arr <= interval_end)]
else:
    filtered_arr = loc_sorted_arr[(loc_sorted_arr >= interval_start) & (loc_sorted_arr < interval_end)]

# Count the number of elements to send to each process
send_counts = np.zeros(nbp, dtype=np.int64)
for i in range(nbp):
    if i == nbp - 1:  # Last process
        send_counts[i] = np.sum((loc_sorted_arr >= global_quartiles[i * (nbp + 1) - 1]) & 
                         (loc_sorted_arr <= global_quartiles[-1]))
    elif i == 0:
        send_counts[i] = np.sum((loc_sorted_arr >= global_quartiles[0]) & 
                         (loc_sorted_arr < global_quartiles[(i + 1) * (nbp + 1) - 1]))
    else:
        send_counts[i] = np.sum((loc_sorted_arr >= global_quartiles[i * (nbp + 1) - 1]) & 
                         (loc_sorted_arr < global_quartiles[(i + 1) * (nbp + 1) - 1]))

# Use Alltoall to exchange the counts of elements to be sent/received
recv_counts = np.zeros(nbp, dtype=np.int64)

comm.Alltoall([send_counts, MPI.INT64_T], [recv_counts, MPI.INT64_T])

# Prepare send and receive buffers
send_buf = np.zeros(np.sum(send_counts), dtype=np.int64)
recv_buf = np.zeros(np.sum(recv_counts), dtype=np.int64)

# Fill the send buffer with elements to be sent to each process
start_idx = 0
for i in range(nbp):
    end_idx = start_idx + send_counts[i]
    if i == nbp - 1:  # Last process
        send_buf[start_idx:end_idx] = loc_sorted_arr[(loc_sorted_arr >= global_quartiles[i * (nbp + 1) - 1]) & 
                                      (loc_sorted_arr <= global_quartiles[-1])]
    elif i == 0:
        send_buf[start_idx:end_idx] = loc_sorted_arr[(loc_sorted_arr >= global_quartiles[0]) & 
                         (loc_sorted_arr < global_quartiles[(i + 1) * (nbp + 1) - 1])]
    else:
        send_buf[start_idx:end_idx] = loc_sorted_arr[(loc_sorted_arr >= global_quartiles[i * (nbp + 1) - 1]) & 
                                      (loc_sorted_arr < global_quartiles[(i + 1) * (nbp + 1) - 1])]
    start_idx = end_idx

# Use Alltoall to distribute the elements
comm.Alltoallv([send_buf, send_counts, MPI.INT64_T], 
               [recv_buf, recv_counts, MPI.INT64_T])

# Sort the received elements
sorted_complete_arr = np.sort(recv_buf)

# Gather the sizes of the sorted_complete_arr from all processes
recv_sizes = np.zeros(nbp, dtype=np.int64)
recv_sizes[rank] = len(sorted_complete_arr)
comm.Allgather([recv_sizes[rank], MPI.INT64_T], [recv_sizes, MPI.INT64_T])

# Prepare the final sorted array
final_sorted_arr = np.zeros(N, dtype=np.int64)

# Use Allgatherv to collect the sorted elements from all processes
comm.Allgatherv([sorted_complete_arr, MPI.INT64_T], 
                [final_sorted_arr, recv_sizes, MPI.INT64_T])

if rank == 0:
    print(final_sorted_arr)
    print(f"Tempo de calculo: {MPI.Wtime() - deb}")
