include Make_linux.inc

CXXFLAGS = -std=c++14
ifdef DEBUG
CXXFLAGS += -g -O0 -Wall -fbounds-check -pedantic -D_GLIBCXX_DEBUG
CXXFLAGS2 = CXXFLAGS
else
CXXFLAGS2 = ${CXXFLAGS} -O2 -march=native -Wall 
CXXFLAGS += -O3 -march=native -Wall
endif

ALL= calcul_pi_mpi.exe calcul_pi_omp.exe jeton.exe test_product_matrix.exe test_product_matrice_blas.exe 

default:	help

all: $(ALL)

clean:
	@find . -name "*.o" -delete
	@rm -fr *.exe *~

.cpp.o:
	$(CXX) $(CXXFLAGS2) -c $^ -o $@	

calcul_pi_omp.exe: ./calcul_pi/compute_pi_omp.c
	$(CC) $^ -o $@ $(LIB)

calcul_pi_mpi.exe: ./calcul_pi/compute_pi_mpi.c
	$(MPIC) $^ -o $@

jeton_omp.exe: ./jeton/jeton_omp.c
	$(CC) $^ -o $@ $(LIB)

jeton_mpi.exe: ./jeton/jeton_mpi.c
	$(MPIC) $^ -o $@

hypercube_omp.exe: ./hypercube/hypercube_omp.c
	$(CC) $^ -o $@ $(LIB)

hypercube_mpi.exe: ./hypercube/hypercube_mpi.c
	$(MPIC) $^ -o $@

test_product_matrix.exe : ./prod_mat_mat/test_product_matrix.o ./prod_mat_mat/Matrix.hpp ./prod_mat_mat/Matrix.o ./prod_mat_mat/ProdMatMat.o
	$(CXX) $(CXXFLAGS2) $^ -o $@ $(LIB)	

test_product_matrice_blas.exe : ./prod_mat_mat/test_product_matrice_blas.o ./prod_mat_mat/Matrix.hpp ./prod_mat_mat/Matrix.o
	$(CXX) $(CXXFLAGS2) $^ -o $@ $(LIB)	$(BLAS)

help:
	@echo "Available targets : "
	@echo "    all            : compile all executables"
	@echo "Add DEBUG=yes to compile in debug"
	@echo "Configuration :"
	@echo "    CXX      :    $(CXX)"
	@echo "    CXXFLAGS :    $(CXXFLAGS)"

%.html: %.md
	pandoc -s --toc $< --css=./github-pandoc.css --metadata pagetitle="OS202 - TD1" -o $@
