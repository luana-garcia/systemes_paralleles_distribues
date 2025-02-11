# TD n° 2 - 27 Janvier 2025

Dependences:
```
sudo apt install python3-mpi4py
```

##  1. Parallélisation ensemble de Mandelbrot

L'ensensemble de Mandebrot est un ensemble fractal inventé par Benoit Mandelbrot permettant d'étudier la convergence ou la rapidité de divergence dans le plan complexe de la suite récursive suivante :
$$
\left\{
\begin{array}{l}
    c\,\,\textrm{valeurs\,\,complexe\,\,donnée}\\
    z_{0} = 0 \\
    z_{n+1} = z_{n}^{2} + c
\end{array}
\right.
$$
dépendant du paramètre $c$.

Il est facile de montrer que si il existe un $N$ tel que $\mid z_{N} \mid > 2$, alors la suite $z_{n}$ diverge. Cette propriété est très utile pour arrêter le calcul de la suite puisqu'on aura détecter que la suite a divergé. La rapidité de divergence est le plus petit $N$ trouvé pour la suite tel que $\mid z_{N} \mid > 2$.

On fixe un nombre d'itérations maximal $N_{\textrm{max}}$. Si jusqu'à cette itération, aucune valeur de $z_{N}$ ne dépasse en module 2, on considère que la suite converge.

L'ensemble de Mandelbrot sur le plan complexe est l'ensemble des valeurs de $c$ pour lesquels la suite converge.

Pour l'affichage de cette suite, on calcule une image de $W\times H$ pixels telle qu'à chaque pixel $(p_{i},p_{j})$, de l'espace image, on associe une valeur complexe  $c = x_{min} + p_{i}.\frac{x_{\textrm{max}}-x_{\textrm{min}}}{W} + i.\left(y_{\textrm{min}} + p_{j}.\frac{y_{\textrm{max}}-y_{\textrm{min}}}{H}\right)$. Pour chacune des valeurs $c$ associées à chaque pixel, on teste si la suite converge ou diverge.

- Si la suite converge, on affiche le pixel correspondant en noir
- Si la suite diverge, on affiche le pixel avec une couleur correspondant à la rapidité de divergence.

1. À partir du code séquentiel `mandelbrot.py`, faire une partition équitable par bloc suivant les lignes de l'image pour distribuer le calcul sur `nbp` processus  puis rassembler l'image sur le processus zéro pour la sauvegarder. Calculer le temps d'exécution pour différents nombre de tâches et calculer le speedup. Comment interpréter les résultats obtenus ?

NBP            | Temps (secondes)
---------------|-----------------
1 (séquentiel) | 2.0647
2              | 1.1729
4              | 0.6733
8              | 0.4992

$S(2) = \frac{2.0647}{1.1729} = 1.76$

$S(4) = \frac{2.0647}{0.6733} = 3.06$

$S(8) = \frac{2.0647}{0.4992} = 4.13$

Le speedup augmente avec le nombre de processus, mais de manière non linéaire.

Le temps de coordination entre les processus et la collecte des résultats ajoutent un coût qui ne diminue pas avec n. Et qui contrairement aumente un peu. On peut voir avec les valeurs d'efficace:

$E(2) = \frac{1.76}{2} = 88\%$

$E(4) = \frac{3.06}{4} = 76.5\%$

$E(8) = \frac{4.13}{8} = 51.6\%$

que la Loi d'Amdahl se comprobe en disant qu'une partie du code reste séquentielle, ce qui limite le speedup maximal possible.


2. Réfléchissez à une meilleur répartition statique des lignes au vu de l'ensemble obtenu sur notre exemple et mettez la en œuvre. Calculer le temps d'exécution pour différents nombre de tâches et calculer le speedup et comparez avec l'ancienne répartition. Quel problème pourrait se poser avec une telle stratégie ?

NBP            | Temps (secondes)
---------------|-----------------
1 (séquentiel) | 3.2279
2              | 1.6043
4              | 0.8652
8              | 0.4704

$S(2) = \frac{2.0647}{1.6043} = 2.01$

$S(4) = \frac{2.0647}{0.8652} = 3.73$

$S(8) = \frac{2.0647}{0.4704} = 6.86$


3. Mettre en œuvre une stratégie maître-esclave pour distribuer les différentes lignes de l'image à calculer. Calculer le speedup avec cette approche et comparez  avec les solutions différentes. Qu'en concluez-vous ?

NBP            | Temps (secondes)
---------------|-----------------
1 (séquentiel) | 3.2210
2              | 3.5029
4              | 1.2294
8              | 0.6520

$S(2) = \frac{3.2210}{3.5029} = 0.91$

$S(4) = \frac{3.2210}{1.2294} = 2.62$

$S(8) = \frac{3.2210}{0.6520} = 4.94$


## 2. Produit matrice-vecteur

On considère le produit d'une matrice carrée $A$ de dimension $N$ par un vecteur $u$ de même dimension dans $\mathbb{R}$. La matrice est constituée des cœfficients définis par $A_{ij} = (i+j) \mod N$. 

Par soucis de simplification, on supposera $N$ divisible par le nombre de tâches `nbp` exécutées.

### a - Produit parallèle matrice-vecteur par colonne

Afin de paralléliser le produit matrice–vecteur, on décide dans un premier temps de partitionner la matrice par un découpage par bloc de colonnes. Chaque tâche contiendra $N_{\textrm{loc}}$ colonnes de la matrice. 

- Calculer en fonction du nombre de tâches la valeur de Nloc
- Paralléliser le code séquentiel `matvec.py` en veillant à ce que chaque tâche n’assemble que la partie de la matrice utile à sa somme partielle du produit matrice-vecteur. On s’assurera que toutes les tâches à la fin du programme contiennent le vecteur résultat complet.
- Calculer le speed-up obtenu avec une telle approche

NBP            | Temps (secondes)
---------------|-----------------
1 (séquentiel) | 0.00837
2              | 0.00155
4              | 0.00092
8              | 0.00087

$S(2) = \frac{0.00837}{0.00155} = 5.40$

$S(4) = \frac{0.00837}{0.00092} = 9.09$

$S(8) = \frac{0.00837}{0.00087} = 9.62$

### b - Produit parallèle matrice-vecteur par ligne

Afin de paralléliser le produit matrice–vecteur, on décide dans un deuxième temps de partitionner la matrice par un découpage par bloc de lignes. Chaque tâche contiendra $N_{\textrm{loc}}$ lignes de la matrice.

- Calculer en fonction du nombre de tâches la valeur de Nloc
- paralléliser le code séquentiel `matvec.py` en veillant à ce que chaque tâche n’assemble que la partie de la matrice utile à son produit matrice-vecteur partiel. On s’assurera que toutes les tâches à la fin du programme contiennent le vecteur résultat complet.
- Calculer le speed-up obtenu avec une telle approche

NBP            | Temps (secondes)
---------------|-----------------
1 (séquentiel) | 0.00837
2              | 0.00130
4              | 0.00117
8              | 0.00071

$S(2) = \frac{0.00837}{0.00130} = 6.43$

$S(4) = \frac{0.00837}{0.00117} = 7.15$

$S(8) = \frac{0.00837}{0.00071} = 11.78$

## 3. Entraînement pour l'examen écrit

Alice a parallélisé en partie un code sur machine à mémoire distribuée. Pour un jeu de données spécifiques, elle remarque que la partie qu’elle exécute en parallèle représente en temps de traitement 90% du temps d’exécution du programme en séquentiel.

En utilisant la loi d’Amdhal, pouvez-vous prédire l’accélération maximale que pourra obtenir Alice avec son code (en considérant n ≫ 1) ?

La loi d'Amdahl est utilisée pour prédire l'accélération théorique maximale d'un programme lors de la parallélisation, en fonction de la proportion du programme qui peut être parallélisée. La formule est :

\[
S = \frac{1}{(1 - P) + \frac{P}{N}}
\]

Où :
- \( S \) est l'accélération maximale.
- \( P \) est la proportion du programme qui peut être parallélisée.
- \( N \) est le nombre de processeurs ou de nœuds de calcul.

**Application :**
Alice a parallélisé 90% de son code. Donc, \( P = 0.9 \). On cherche \( S \) quand \( N \) est très grand (\( N \gg 1 \)).

\[
S = \frac{1}{(1 - 0.9) + \frac{0.9}{N}}
\]

Quand \( N \) est très grand, \( \frac{0.9}{N} \) tend vers 0.

\[
S \approx \frac{1}{0.1} = 10
\]

**Conclusion :** L'accélération maximale théorique est de **10**.


À votre avis, pour ce jeu de donné spécifique, quel nombre de nœuds de calcul semble-t-il raisonnable de prendre pour ne pas trop gaspiller de ressources CPU ?

Pour éviter le gaspillage de ressources CPU, il faut trouver un équilibre entre l'accélération et le nombre de nœuds utilisés. Une approche courante est de chercher un point où l'augmentation du nombre de nœuds n'apporte plus d'amélioration significative de l'accélération.

**Étapes :**
- Calculer \( S \) pour différentes valeurs de \( N \).
- Identifier le point où l'augmentation de \( N \) n'augmente plus significativement \( S \).

**Exemple de calcul :**

| \( N \) | \( S \) |
|---------|---------|
| 1       | 1       |
| 2       | 1.82    |
| 4       | 3.08    |
| 8       | 4.71    |
| 16      | 6.40    |
| 32      | 7.27    |
| 64      | 8.00    |
| 128     | 8.57    |

**Observation :**
L'accélération augmente rapidement jusqu'à environ 8 nœuds, puis ralentit. Ainsi, utiliser entre 8 et 16 nœuds semble raisonnable pour maximiser l'efficacité sans gaspiller trop de ressources.

En effectuant son cacul sur son calculateur, Alice s’aperçoit qu’elle obtient une accélération maximale de quatre en augmentant le nombre de nœuds de calcul pour son jeu spécifique de données.

En doublant la quantité de donnée à traiter, et en supposant la complexité de l’algorithme parallèle linéaire, quelle accélération maximale peut espérer Alice en utilisant la loi de Gustafson ?


La loi de Gustafson propose une vision différente de la parallélisation, en supposant que la taille du problème augmente avec le nombre de processeurs. La formule est :

\[
S = N + (1 - N) \times s
\]

Où :
- \( S \) est l'accélération.
- \( N \) est le nombre de processeurs.
- \( s \) est la proportion séquentielle du programme.

**Application :**
Alice obtient une accélération maximale de 4 avec son jeu de données initial. Supposons que cela correspond à un certain nombre de nœuds \( N \). En doublant la quantité de données, et avec une complexité linéaire, la proportion parallélisable \( P \) reste à 90%, donc \( s = 1 - P = 0.1 \).

\[
S = N + (1 - N) \times 0.1
\]

Mais cette formule semble incorrecte dans ce contexte. En réalité, la loi de Gustafson est souvent exprimée comme :

\[
S = N \times (1 - s) + s
\]

Avec \( s = 0.1 \) (partie séquentielle), et en supposant que \( N \) est suffisamment grand pour que la partie parallèle domine :

\[
S \approx N \times 0.9
\]

Cependant, comme Alice a déjà une accélération de 4 avec un certain \( N \), et en doublant la quantité de données, l'accélération peut augmenter proportionnellement si la complexité reste linéaire.

**Conclusion :** En doublant la quantité de données, et avec une complexité linéaire, l'accélération maximale pourrait doubler, passant de 4 à 8.
