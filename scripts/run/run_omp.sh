#!/bin/bash

cd ../../
(make clean && make omp_pca) > /dev/null 2>&1

INPUT_DIR="input"
RESULT_FILE="results/omp.csv"

mkdir -p results
rm -f "$RESULT_FILE"
touch "$RESULT_FILE"

echo "variant,image,width,height,time_comp,nof_components,num_threads,num_procs" >> $RESULT_FILE

NOF_COMPONENTS=(25 50 100)
NOF_THREADS=(2 4 8 16 32)

echo "Starting PCA processing with OpenMP..."

for img in $INPUT_DIR/test00_*.ppm; do
    img_name=$(basename "$img")

    for n in "${NOF_COMPONENTS[@]}"; do
        for nthreads in "${NOF_THREADS[@]}"; do
            echo "Running omp_pca on $img_name with $n components, $nthreads threads"
            sbatch --wait -n 1 --cpus-per-task=$nthreads --wrap="./omp_pca $INPUT_DIR/$img_name $n >> $RESULT_FILE"
        done
    done
done

rm slurm-*.out
cd -
