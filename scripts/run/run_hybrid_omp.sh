#!/bin/bash

cd ../../

(make clean && make hybrid_fep_omp_pca) > /dev/null 2>&1

INPUT_DIR="input"
RESULT_FILE="results/hybrid_omp.csv"

mkdir -p results
rm "$RESULT_FILE"

touch "$RESULT_FILE"
echo "variant,image,width,height,time_comp,nof_components,num_threads,num_procs" >> $RESULT_FILE

NOF_COMPONENTS=(10 20 30)
NOF_THREADS=(2 4 8)
NOF_PROCS=(4 9 16)

for img in $INPUT_DIR/test00_*.ppm; do
    img_name=$(basename "$img")

    for n in "${NOF_COMPONENTS[@]}"; do
        for nthreads in "${NOF_THREADS[@]}"; do
            for nprocs in "${NOF_PROCS[@]}"; do
                
                total_cores=$((nthreads * nprocs))

                if [ "$total_cores" -le 32 ]; then

                    sbatch --wait \
                        --ntasks=$nprocs \
                        --cpus-per-task=$nthreads \
                        --wrap="bash -lc '
                            module load mpi/openmpi-x86_64 && 
                            export OMP_NUM_THREADS=\$SLURM_CPUS_PER_TASK && 
                            OMPI_MCA_btl=self,vader,tcp OMPI_MCA_mtl=^ofi \
                            mpirun -np \$SLURM_NTASKS ./hybrid_fep_omp_pca $INPUT_DIR/$img_name $n >> $RESULT_FILE
                        '"
                fi  
            done
        done
    done
done

rm slurm-*.out
cd -
