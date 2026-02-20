#!/bin/bash

cd ../../
(make clean && make seq_pca) > /dev/null 2>&1

INPUT_DIR="input"
RESULT_FILE="results/seq.csv"

mkdir -p results
rm -f "$RESULT_FILE"
touch "$RESULT_FILE"

echo "variant,image,width,height,time_comp,nof_components,num_threads,num_procs" >> $RESULT_FILE

NOF_COMPONENTS=(20 25 50 100)

echo "Starting PCA processing with Sequential implementation..."

for img in $INPUT_DIR/test00_*.ppm; do
    for n in "${NOF_COMPONENTS[@]}"; do
        echo "Running seq_pca on $(basename $img) with $n components"
        sbatch --wait -n 1 --cpus-per-task=1 --wrap="./seq_pca $INPUT_DIR/$(basename $img) $n >> $RESULT_FILE"
    done
done

rm slurm-*.out
cd -
