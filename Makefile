CXX     = g++
MPICXX  = mpic++

CXXFLAGS = -std=c++17 -Wall -Werror -lm -lstdc++fs\
		   -I include/eigen-lib -I include/ -I include/spectra/include

OMPFLAGS      = -fopenmp
PTHREADSFLAGS = -lpthread

# Source files
MAIN_SRC          = main.cpp
MPI_SRC           = main_mpi.cpp
MPI_HELP_SRC      = src/mpi_pca.cpp
SEQ_SPECTRA_SRC   = src/sequential_spectra.cpp
SEQ_SRC           = src/sequential.cpp
OMP_SRC           = src/omp_pca.cpp
PTHREADS_SRC      = src/pthreads_pca.cpp
PTHREADS_HELP_SRC = src/pthreads_funcs.cpp src/threadpool.cpp

COMMON_SRC = src/compressed_pca.cpp src/ppm_image.cpp

# Binary names
SEQ_SPECTRA_BIN      = seq_spectra_pca
SEQ_BIN              = seq_pca
OMP_BIN              = omp_pca
PTHREADS_BIN         = pthreads_pca
MPI_BIN              = mpi_pca
MPI_FEP_BIN          = mpi_fep_pca
HYBRID_OMP_BIN       = hybrid_omp_pca
HYBRID_FEP_OMP_BIN   = hybrid_fep_omp_pca
HYBRID_PTHR_BIN      = hybrid_pthr_pca
HYBRID_FEP_PTHR_BIN  = hybrid_fep_pthr_pca

all: $(SEQ_SPECTRA_BIN) $(OMP_BIN) $(SEQ_BIN) $(MPI_BIN) $(HYBRID_OMP_BIN) $()

# Sequential Eigen PCA
$(SEQ_SPECTRA_BIN): $(MAIN_SRC) $(SEQ_SPECTRA_SRC) $(COMMON_SRC)
	$(CXX) $(CXXFLAGS) $^ -o $@ -DVARIANT_NAME=\"seq_spectra\" $(LDFLAGS)

# Sequential PCA using Lanczos
$(SEQ_BIN): $(MAIN_SRC) $(SEQ_SRC) $(COMMON_SRC)
	$(CXX) $(CXXFLAGS) $^ -o $@ -DVARIANT_NAME=\"seq\" $(LDFLAGS)

# OpenMP PCA
$(OMP_BIN): $(MAIN_SRC) $(OMP_SRC) $(COMMON_SRC)
	$(CXX) $(CXXFLAGS) $(OMPFLAGS) $^ -o $@ -DVARIANT_NAME=\"omp\" $(LDFLAGS)

# Pthreads PCA
$(PTHREADS_BIN): $(MAIN_SRC) $(PTHREADS_SRC) $(COMMON_SRC) $(PTHREADS_HELP_SRC)
	$(CXX) $(CXXFLAGS) $(PTHREADSFLAGS) $^ -o $@ -DVARIANT_NAME=\"pthreads\"

# MPI PCA
$(MPI_BIN): $(MPI_SRC) $(SEQ_SRC) $(COMMON_SRC) $(MPI_HELP_SRC)
	$(MPICXX) $(CXXFLAGS) $^ -o $@ -DVARIANT_NAME=\"mpi\" $(LDFLAGS)

# MPI FEP PCA
$(MPI_FEP_BIN): $(MPI_SRC) $(SEQ_SRC) $(COMMON_SRC) $(MPI_HELP_SRC)
	srun bash -lc "module load mpi/openmpi-x86_64 && $(MPICXX) $(OMPFLAGS) $(CXXFLAGS) $(LDFLAGS) $^ -o $@ -DVARIANT_NAME='\"mpi_fep\"'"

# Hybrid PCA using MPI-OMP
$(HYBRID_OMP_BIN): $(MPI_SRC) $(OMP_SRC) $(COMMON_SRC) $(MPI_HELP_SRC)
	$(MPICXX) $(CXXFLAGS) $(OMPFLAGS) $^ -o $@ -DVARIANT_NAME=\"hybrid_omp\"

# Hybrid FEP MPI-OMP PCA
$(HYBRID_FEP_OMP_BIN): $(MPI_SRC) $(OMP_SRC) $(COMMON_SRC) $(MPI_HELP_SRC)
	srun bash -lc "module load mpi/openmpi-x86_64 && $(MPICXX) $(CXXFLAGS) $(OMPFLAGS) $^ -o $@ -DVARIANT_NAME='\"hybrid_fep_omp\"'"

# Hybrid PCA using MPI-Pthreads
$(HYBRID_PTHR_BIN): $(MPI_SRC) $(OMP_SRC) $(COMMON_SRC) $(MPI_HELP_SRC)
	$(MPICXX) $(CXXFLAGS) $(OMPFLAGS) $^ -o $@ -DVARIANT_NAME=\"hybrid_pthr\"

# Hybrid FEP MPI-Pthreads PCA
$(HYBRID_FEP_PTHR_BIN): $(MPI_SRC) $(PTHREADS_SRC) $(COMMON_SRC) $(MPI_HELP_SRC) $(PTHREADS_HELP_SRC)
	srun bash -lc "module load mpi/openmpi-x86_64 && $(MPICXX) $(CXXFLAGS) $(PTHREADSFLAGS) $^ -o $@ -DVARIANT_NAME='\"hybrid_fep_pthr\"'"

clean:
	rm -f $(SEQ_SPECTRA_BIN) $(OMP_BIN) $(MPI_BIN) $(HYBRID_FEP_OMP_BIN) $(HYBRID_OMP_BIN) $(MPI_FEP_BIN) $(SEQ_BIN) $(PTHREADS_BIN) output/*.ppm output_compressed/*

run_mpi_fep:
	sbatch -n$(REQ_PROC) --wrap='bash -lc "module load mpi/openmpi-x86_64 && OMPI_MCA_btl=self,vader,tcp OMPI_MCA_mtl=^ofi mpirun -np $$SLURM_NTASKS ./$(MPI_FEP_BIN) $(REQ_INPUT) $(REQ_COMP)"'

run_hyb_fep:
	sbatch -n$(REQ_PROC) --wrap='bash -lc "module load mpi/openmpi-x86_64 && OMPI_MCA_btl=self,vader,tcp OMPI_MCA_mtl=^ofi mpirun -np $$SLURM_NTASKS ./$(HYBRID_FEP_OMP_BIN) $(REQ_INPUT) $(REQ_COMP)"'

.PHONY: all clean run_mpi_fep run_hyb_fep
