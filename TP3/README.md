# TD n°3 - parallélisation du Bucket Sort

*Ce TD peut être réalisé au choix, en C++ ou en Python*

Implémenter l'algorithme "bucket sort" tel que décrit sur les deux dernières planches du cours n°3 :

- le process 0 génère un tableau de nombres arbitraires,
- il les dispatch aux autres process,
- tous les process participent au tri en parallèle,
- le tableau trié est rassemblé sur le process 0.

J'utilise la tecnique indiqué par le professeur avec les quartiles pour trier la liste.

No numéros liste | Temps (NBP = 1) | Temps (NBP = 2) | Temps (NBP = 4) | Temps (NBP = 8)
-------------|---------------|-----------------|-----------|------------
120 | 0.0010 | 0.0010 | 0.0014 | 0.0036
120.000 | 0.0100 | 0.0081 | 0.0067 | 0.0056
1.200.000 | 0.0619 | 0.0527 | 0.0391 | 0.0366
12.000.000 | 0.5420 | 0.5725 | 0.5450 | 0.6088