
# TD1

`pandoc -s --toc README.md --css=./github-pandoc.css -o README.html`

## lscpu

*lscpu donne des infos utiles sur le processeur : nb core, taille de cache :*

```
Architecture:             x86_64
  CPU op-mode(s):         32-bit, 64-bit
  Address sizes:          48 bits physical, 48 bits virtual
  Byte Order:             Little Endian
CPU(s):                   16
  On-line CPU(s) list:    0-15

Vendor ID:                AuthenticAMD
  Model name:             AMD Ryzen 7 7730U with Radeon Graphics
    CPU family:           25
    Model:                80
    Thread(s) per core:   2
    Core(s) per socket:   8
    Socket(s):            1
    Stepping:             0
    CPU max MHz:          4546,0000
    CPU min MHz:          400,0000
    BogoMIPS:             3992.34
    Flags:                fpu vme de pse tsc msr pae mce cx8 apic sep mtrr pge mca cmov pat pse36 clflush mmx fxsr sse sse2 ht syscall nx mmxext fxsr_opt pdpe1gb rdtscp lm constant_tsc rep_good nopl nonst
                          op_tsc cpuid extd_apicid aperfmperf rapl pni pclmulqdq monitor ssse3 fma cx16 sse4_1 sse4_2 movbe popcnt aes xsave avx f16c rdrand lahf_lm cmp_legacy svm extapic cr8_legacy abm s
                          se4a misalignsse 3dnowprefetch osvw ibs skinit wdt tce topoext perfctr_core perfctr_nb bpext perfctr_llc mwaitx cpb cat_l3 cdp_l3 hw_pstate ssbd mba ibrs ibpb stibp vmmcall fsgsb
                          ase bmi1 avx2 smep bmi2 erms invpcid cqm rdt_a rdseed adx smap clflushopt clwb sha_ni xsaveopt xsavec xgetbv1 xsaves cqm_llc cqm_occup_llc cqm_mbm_total cqm_mbm_local user_shstk 
                          clzero irperf xsaveerptr rdpru wbnoinvd cppc arat npt lbrv svm_lock nrip_save tsc_scale vmcb_clean flushbyasid decodeassists pausefilter pfthreshold avic v_vmsave_vmload vgif v_s
                          pec_ctrl umip pku ospke vaes vpclmulqdq rdpid overflow_recov succor smca fsrm debug_swap
Virtualization features:  
  Virtualization:         AMD-V
Caches (sum of all):      
  L1d:                    256 KiB (8 instances)
  L1i:                    256 KiB (8 instances)
  L2:                     4 MiB (8 instances)
  L3:                     16 MiB (1 instance)
NUMA:                     
  NUMA node(s):           1
  NUMA node0 CPU(s):      0-15
Vulnerabilities:          
  Gather data sampling:   Not affected
  Itlb multihit:          Not affected
  L1tf:                   Not affected
  Mds:                    Not affected
  Meltdown:               Not affected
  Mmio stale data:        Not affected
  Reg file data sampling: Not affected
  Retbleed:               Not affected
  Spec rstack overflow:   Mitigation; Safe RET
  Spec store bypass:      Mitigation; Speculative Store Bypass disabled via prctl
  Spectre v1:             Mitigation; usercopy/swapgs barriers and __user pointer sanitization
  Spectre v2:             Mitigation; Retpolines; IBPB conditional; IBRS_FW; STIBP always-on; RSB filling; PBRSB-eIBRS Not affected; BHI Not affected
  Srbds:                  Not affected
  Tsx async abort:        Not affected
```


## Produit matrice-matrice

### Effet de la taille de la matrice

  n            | MFlops
---------------|--------
1018           | 99361.4
1023           | 119782
1024 (origine) | 117440
1025           | 111010
1028           | 103306

*Explication des résultats.*
Les performances de la multiplication matricielle sont très sensibles à la taille de la matrice en raison de l'alignement de la mémoire cache, des schémas d'accès à la mémoire et des optimisations du compilateur. Les meilleures performances observées à 1023 et 1024 sont probablement dues à un alignement plus efficace de la mémoire, tandis que la chute à 1018, 1025 et 1028 est due à un mauvais alignement et à une augmentation des erreurs de cache.


### Permutation des boucles

*Comment est compilé le code (ligne de make)*

Le code a probablement été compilé avec des optimisations activées.
L’optimisation -O3 et -march=native est cruciale pour la performance de la multiplication de matrices, car elle permet d’exploiter les instructions spécifiques du processeur et de maximiser l'utilisation du cache.

`make test_product_matrix.exe && ./test_product_matrix.exe 1024`


  ordre           | time      | MFlops  | MFlops(n=2048)
------------------|-----------|---------|----------------
i,j,k             | 0.017849  | 120314  | 167424
j,i,k             | 0.0179272 | 119789  | 165472
i,k,j  (origine)  | 0.0230443 | 93189.4 | 166961
k,i,j             | 0.0181382 | 118396  | 165473
j,k,i             | 0.0209532 | 102490  | 162599
k,j,i             | 0.0180216 | 119162  | 164204


*Discuter les résultats.*
L'ordre des boucles affecte la manière dont les données sont accédées en mémoire, influençant l'utilisation du cache et la performance globale.

Considérons la multiplication de matrices suivante :

**C(i,j)+=A(i,k)×B(k,j)**

Où :

- i parcourt les lignes de la matrice C.
- j parcourt les colonnes de la matrice C.
- k parcourt les colonnes de A et les lignes de B.

L’efficacité dépend de la localité du cache, c’est-à-dire si les données sont accessibles de manière contiguë en mémoire.

- L’ordre i, j, k est le meilleur, car il suit un parcours logique et efficace pour la mémoire cache.
- L’ordre original i, k, j est le pire en termes de performance, car il accède aux éléments de B(k, j) de manière dispersée, provoquant plus de cache misses.
- Éviter un parcours désorganisé des matrices permet d'améliorer significativement la performance de la multiplication matricielle.

La réorganisation des boucles a un impact majeur sur la rapidité d'exécution à cause de la manière dont les données sont stockées et accédées en mémoire.


### OMP sur la meilleure boucle

Avec le code suivante:

```
#pragma omp parallel
  {
    #pragma omp for
    for (int i = iRowBlkA; i < std::min(A.nbRows, iRowBlkA + szBlock); ++i)
      for (int k = iColBlkA; k < std::min(A.nbCols, iColBlkA + szBlock); k++)
        for (int j = iColBlkB; j < std::min(B.nbCols, iColBlkB + szBlock); j++)
          #pragma omp atomic
          C(i, j) += A(i, k) * B(k, j);
  }
```

`make test_product_matrix.exe && OMP_NUM_THREADS=8 ./test_product_matrix.exe 1024`

  OMP_NUM         | MFlops  | MFlops(n=2048) | MFlops(n=512)  | MFlops(n=4096)
------------------|---------|----------------|----------------|---------------
8  | 1096.77 | 933.994 | 1127.13 | 777.547
16 | 1511.07 | 1336.46 | 2129.21 | 632.42
32 | 2194.28 | 1436.65 | 2248.98 | 369.045
64 | 2315.75 | 1555.3 | 2145.81 | 196.164

*Tracer les courbes de speedup (pour chaque valeur de n), discuter les résultats.*

Le speedup S est défini comme :
**S(p)=  T(1) / T(p)**
​
 
où :
- T(1) est le temps d'exécution en monothread (OMP_NUM_THREADS=1).
- T(p) est le temps d'exécution avec p threads.

Puisque nous avons des performances en MFlops (et que plus de MFlops signifie un meilleur temps d'exécution), nous pouvons exprimer le speedup comme :

**S(p)= MFlops(p) / MFlops(1)**
​
 
avec MFlops(1) étant approximativement la performance pour OMP_NUM_THREADS = 8 (si nous ne disposons pas de la valeur pour 1 thread).

2. Calcul des Speedups
On utilise les MFlops pour estimer S(p). Prenons les valeurs de OMP_NUM_THREADS=8 comme base (𝑝 = 1).

Speedup estimé pour chaque taille de matrice :
OMP_NUM	| S(1024)	| S(2048)	| S(512)	| S(4096)
--------|---------|---------|---------|--------
8	  | 1.00	| 1.00	| 1.00	| 1.00
16	| 1.38	| 1.43	| 1.89	| 0.81
32	| 2.00	| 1.54	| 1.99	| 0.47
64	| 2.11	| 1.67	| 1.90	| 0.25


*Discussion des Résultats*
n = 1024 et n = 512 : Bon scaling jusqu'à 32 threads, mais saturation après.
n = 2048 : Amélioration plus progressive, mais même saturation après 32 threads.
n = 4096 : Dégradation au-delà de 16 threads, probablement due à :
Accès mémoire non optimisés (taille de cache insuffisante).
Coût élevé de la synchronisation avec #pragma omp atomic.
Faible charge de travail par thread, rendant l'overhead de gestion des threads trop important.




### Produit par blocs

`make test_product_matrix.exe && ./test_product_matrix.exe 1024`

  szBlock         | MFlops  | MFlops(n=2048) | MFlops(n=512)  | MFlops(n=4096)
------------------|---------|----------------|----------------|---------------
32                | 877.578 | 936.239 | 878.275 | 684
64                | 774.113 | 1006.11 | 928.822 | 720.434
128               | 782.871 | 971.769 | 1358.63 | 860.056
256               | 1770.25 | 1312.6  | 793.276 | 685.419
512               | 1911.04 | 1419.24 | 1065.24 | 773.709
1024              | 1705.76 | 1406.02 | 2438.8 | 678.086

*Discuter les résultats.*
- Le choix du bon szBlock a un impact majeur sur la performance.
- Un bloc trop petit entraîne des accès mémoire trop fréquents.
- Un bloc trop grand surcharge le cache, entraînant une perte de performance.
- Le meilleur compromis se situe entre 256 et 512 pour la plupart des tailles de matrices.
- L’optimisation mémoire et cache est clé pour la multiplication matricielle rapide.


### Bloc + OMP


  szBlock      | OMP_NUM | MFlops  | MFlops(n=2048) | MFlops(n=512)  | MFlops(n=4096)|
---------------|---------|---------|----------------|----------------|---------------|
1024           |  1      | 1705.76 | 1406.02 | 2438.8 | 678.086
1024           |  8      | 1334.26 | 1007.71 | 1496.37 | 739.379
512            |  1      | 1911.04 | 1419.24 | 1065.24 | 773.709
512            |  8      | 1338 | 1003.88 | 1413.15 | 804.429

*Discuter les résultats.*
- Pour des tailles de matrices petites à moyennes, le parallélisme OpenMP ne donne pas toujours un gain significatif, et dans certains cas, il peut même réduire les performances en raison de l'overhead de gestion des threads et des conflits d'accès mémoire.
- Le meilleur compromis pour obtenir de bonnes performances serait de choisir une taille de bloc appropriée (par exemple, szBlock = 512 ou 256), d'optimiser l'usage du cache, et d'utiliser OpenMP de manière ciblée pour des calculs suffisamment lourds pour que le parallélisme soit bénéfique.


### Comparaison avec BLAS, Eigen et numpy

*Comparer les performances avec un calcul similaire utilisant les bibliothèques d'algèbre linéaire BLAS, Eigen et/ou numpy.*

En general:
- Pour des projets en C++, Eigen est un bon choix intermédiaire si vous avez besoin d'une bibliothèque facile à utiliser et relativement rapide.
- Pour les petites matrices ou des calculs rapides et simples en Python, NumPy est suffisant, bien qu'il soit plus lent que les alternatives C++.
- Si vous avez des matrices très grandes, BLAS est la meilleure option pour des performances optimales.

On va tester avec BLAS les tailles de matrices suivants pour vérifier l'hipotèse:
`make test_product_matrice_blas.exe && ./test_product_matrice_blas.exe`

  n            | MFlops
---------------|--------
1018           | 90986
1023           | 107857
1024 (origine) | 124051
1025           | 131064
1028           | 118864


## Parallélisation MPI

### Circulation d’un jeton dans un anneau
`make jeton_omp.exe && OMP_NUM_THREADS=4 ./jeton_omp.exe`
`make jeton_mpi.exe && mpirun -np 4 ./jeton_mpi.exe`
`mpirun -np 4 python3 ./jeton/jeton.py`

Langage | Bibliothèque | Temps (p = 4) | Temps (p = 8)
--------|--------------|---------------|--------------
C       | OMP          | x             | x
C       | MPI          | 0.000101      | 0.000333
Python  | MPI          | 0.0000848     | 0.000207

### Calcul très approché de pi

`make calcul_pi_omp.exe && OMP_NUM_THREADS=4 ./calcul_pi_omp.exe`
`make calcul_pi_mpi.exe && mpirun -np 4 ./calcul_pi_mpi.exe`
`mpirun -np 4 python3 ./calcul_pi/compute_pi.py`

Langage | Bibliothèque | Temps (p = 4) | Temps (p = 8)
--------|--------------|---------------|--------------
C       | OMP          | 0.0951        | 0.1517
C       | MPI          | 0.0072        | 0.0048
Python  | MPI          | 0.6858        | 0.6930

### Diffusion d’un entier dans un réseau hypercube
`make hypercube_omp.exe && OMP_NUM_THREADS=4 ./hypercube_omp.exe`
`make hypercube_mpi.exe && mpirun -np 4 ./hypercube_mpi.exe`
`mpirun -np 4 python3 ./hypercube/compute_pi.py`


# Tips

```
	env
	OMP_NUM_THREADS=4 ./produitMatriceMatrice.exe
```

```
    $ for i in $(seq 1 4); do elap=$(OMP_NUM_THREADS=$i ./TestProductOmp.exe|grep "Temps CPU"|cut -d " " -f 7); echo -e "$i\t$elap"; done > timers.out
```
