#!/bin/bash

cd ../../
(make clean && make mpi_fep_pca) > /dev/null 2>&1

INPUT_DIR="input"
RESULT_FILE="results/mpi.csv"

mkdir -p results
rm -f "$RESULT_FILE"
touch "$RESULT_FILE"

echo "variant,image,width,height,time_comp,nof_components,num_threads,num_procs" >> $RESULT_FILE

NOF_COMPONENTS=(15 25 35)
NOF_PROCS=(4 9 16 25)

for img in $INPUT_DIR/test00_*.ppm; do
    img_name=$(basename "$img")

    for n in "${NOF_COMPONENTS[@]}"; do
        for nprocs in "${NOF_PROCS[@]}"; do
            echo "Running mpi_fep_pca on $img_name with $n components, 1 thread, $nprocs processes"
        
            sbatch --wait \
                --ntasks=$nprocs \
                --cpus-per-task=1 \
                --wrap="bash -lc '
                    module load mpi/openmpi-x86_64 && 
                    export OMP_NUM_THREADS=1 && 
                    OMPI_MCA_btl=self,vader,tcp OMPI_MCA_mtl=^ofi \
                    mpirun -np \$SLURM_NTASKS ./mpi_fep_pca $INPUT_DIR/$img_name $n >> $RESULT_FILE
                '"
        done
    done
done

rm slurm-*.out
cd -
